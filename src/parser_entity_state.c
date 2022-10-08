#include "parser_entity_state.h"
#include "alignof_wrapper.h"
#include "arena.h"
#include "hashtable.h"
#include "utils.h"
#include <string.h>

typedef struct {
  size_t max_props;
  prop_exclude_set excluded_props;
  hashtable dts_with_excludes;
  size_t dt_index;
  size_t serverclass_index;
  size_t flattenedprop_index;
  uint16_t baseclass_array[1024];
  size_t baseclass_count;
  size_t baseclass_index;
} propdata;

typedef struct {
  estate_init_args args;
  estate *entity_state;
  entity_parse_scrap* ent_scrap;
  const char *error_message;
  bool error;
} estate_init_state;

static void add_baseclass(propdata *data, size_t dt_index) {
  ++data->baseclass_count;

  if (data->baseclass_count > 1024) {
    abort();
  }

  memmove(data->baseclass_array + data->baseclass_index + 1,
          data->baseclass_array + data->baseclass_index,
          sizeof(size_t) * data->baseclass_count - data->baseclass_index);
  data->baseclass_array[data->baseclass_index] = dt_index;
}

static uint16_t get_baseclass_from_array(propdata *data, size_t index) {
  return data->baseclass_array[index];
}

// Returns true if either hashtable gets completely full
static bool add_exclude(propdata *data, demogobbler_sendprop *prop) {
  demogobbler_pes_insert(&data->excluded_props, prop);
  hashtable_entry entry;
  entry.str = prop->exclude_name;
  entry.value = 0;
  demogobbler_hashtable_insert(&data->dts_with_excludes, entry);
  return data->excluded_props.item_count != data->excluded_props.max_items &&
         data->dts_with_excludes.item_count != data->dts_with_excludes.max_items;
}

static bool does_datatable_have_excludes(propdata *data, demogobbler_sendtable *table) {
  hashtable_entry entry = demogobbler_hashtable_get(&data->dts_with_excludes, table->name);
  return entry.str != NULL;
}

static bool is_prop_excluded(propdata *data, demogobbler_sendtable *table,
                             demogobbler_sendprop *prop) {
  return demogobbler_pes_has(&data->excluded_props, table, prop);
}

static void create_dt_hashtable(estate_init_state *thisptr) {
  demogobbler_sendtable *sendtables = thisptr->entity_state->sendtables;
  const size_t sendtable_count = thisptr->entity_state->sendtable_count;
  const float FILL_RATE = 0.9f;
  const size_t buckets = (size_t)(sendtable_count / FILL_RATE);

  if (thisptr->ent_scrap->dt_hashtable.arr != NULL) {
    demogobbler_hashtable_clear(&thisptr->ent_scrap->dt_hashtable);
  } else {
    thisptr->ent_scrap->dt_hashtable = demogobbler_hashtable_create(buckets);
  }

  for (size_t i = 0; i < sendtable_count && !thisptr->error; ++i) {
    hashtable_entry entry;
    entry.str = sendtables[i].name;
    entry.value = i;

    if (!demogobbler_hashtable_insert(&thisptr->ent_scrap->dt_hashtable, entry)) {
      thisptr->error = true;
      thisptr->error_message = "Hashtable collision with datatable names or ran out of space";
    }
  }
}

static size_t get_baseclass(estate_init_state *thisptr, propdata *data,
                            demogobbler_sendprop *prop) {
  demogobbler_sendtable *sendtables = thisptr->entity_state->sendtables;
  if (prop->baseclass == NULL) {
    hashtable_entry entry =
        demogobbler_hashtable_get(&thisptr->ent_scrap->dt_hashtable, prop->dtname);

    if (entry.str == NULL) {
      // No entry found
      thisptr->error = true;
      thisptr->error_message = "Was unable to find datatable pointed to by sendprop";
      return 0;
    }
    prop->baseclass = sendtables + entry.value;
  }
  return prop->baseclass - sendtables;
}

static void gather_excludes(estate_init_state *thisptr, propdata *data, size_t datatable_index) {
  demogobbler_sendtable *table = thisptr->entity_state->sendtables + datatable_index;

  for (size_t prop_index = 0; prop_index < table->prop_count; ++prop_index) {
    demogobbler_sendprop *prop = table->props + prop_index;

    if (prop->proptype == sendproptype_datatable) {
      size_t baseclass_index = get_baseclass(thisptr, data, prop);

      if (thisptr->error)
        return;

      gather_excludes(thisptr, data, baseclass_index);
    } else if (prop->flag_exclude) {
      if (!add_exclude(data, prop)) {
        thisptr->error = true;
        thisptr->error_message = "Was unable to add exclude";
      }
    }
  }
}

static void gather_propdata(estate_init_state *thisptr, propdata *data, size_t datatable_index) {
  demogobbler_sendtable *sendtables = thisptr->entity_state->sendtables;
  demogobbler_sendtable *table = sendtables + datatable_index;
  bool table_has_excludes = does_datatable_have_excludes(data, table);

  for (size_t prop_index = 0; prop_index < table->prop_count; ++prop_index) {
    demogobbler_sendprop *prop = table->props + prop_index;
    bool prop_excluded = table_has_excludes && is_prop_excluded(data, table, prop);
    if (prop_excluded)
      continue;

    if (prop->proptype == sendproptype_datatable) {
      size_t baseclass_index = get_baseclass(thisptr, data, prop);

      if (thisptr->error)
        return;

      if (!prop->flag_collapsible) {
        add_baseclass(data, baseclass_index);
        gather_propdata(thisptr, data, baseclass_index);
        ++data->baseclass_index;
      } else {
        gather_propdata(thisptr, data, baseclass_index);
      }
    } else if (!prop->flag_insidearray && !prop->flag_exclude) {
      ++data->max_props;
    }
  }
}

static uint8_t get_priority_protocol4(demogobbler_sendprop *prop) {
  if (prop->priority >= 64 && prop->flag_changesoften)
    return 64;
  else
    return prop->priority;
}

static void sort_props(estate_init_state *thisptr, serverclass_data *class_data) {
  if (thisptr->args.version_data->demo_protocol >= 4 && thisptr->args.version_data->game != l4d) {
    bool priorities[256];
    memset(priorities, 0, sizeof(priorities));

    for (int i = 0; i < class_data->prop_count; ++i) {
      size_t prio = get_priority_protocol4(class_data->props + i);
      priorities[prio] = true;
    }

    size_t start = 0;

    for (size_t current_prio = 0; current_prio < 256; ++current_prio) {
      if (priorities[current_prio] == false)
        continue;

      for (size_t i = start; i < class_data->prop_count; ++i) {
        demogobbler_sendprop *prop = class_data->props + i;
        uint8_t prio = get_priority_protocol4(prop);

        if (prio == current_prio) {
          demogobbler_sendprop *dest = class_data->props + start;
          demogobbler_sendprop temp = *dest;
          *dest = *prop;
          *prop = temp;
          ++start;
        }
      }
    }
  } else {
    size_t start = 0;

    for (size_t i = start; i < class_data->prop_count; ++i) {
      demogobbler_sendprop *prop = class_data->props + i;
      if (prop->flag_changesoften) {
        demogobbler_sendprop *dest = class_data->props + start;
        demogobbler_sendprop temp = *dest;
        *dest = *prop;
        *prop = temp;
        ++start;
      }
    }
  }
}

static void iterate_props(estate_init_state *thisptr, propdata *data,
                          demogobbler_sendtable *table) {
  bool table_has_excludes = does_datatable_have_excludes(data, table);
  for (size_t prop_index = 0; prop_index < table->prop_count; ++prop_index) {
    demogobbler_sendprop *prop = table->props + prop_index;
    if (prop->proptype == sendproptype_datatable) {
      if (prop->flag_collapsible)
        iterate_props(thisptr, data, prop->baseclass);
    } else if (!prop->flag_exclude && !prop->flag_insidearray) {
      bool prop_excluded = table_has_excludes && is_prop_excluded(data, table, prop);
      if (!prop_excluded) {
        size_t flattenedprop_index =
            thisptr->entity_state->class_datas[data->serverclass_index].prop_count;
        demogobbler_sendprop *dest =
            thisptr->entity_state->class_datas[data->serverclass_index].props + flattenedprop_index;
        memcpy(dest, prop, sizeof(demogobbler_sendprop));
        ++thisptr->entity_state->class_datas[data->serverclass_index].prop_count;
      }
    }
  }
}

static void gather_props(estate_init_state *thisptr, propdata *data) {
  demogobbler_sendtable *sendtables = thisptr->entity_state->sendtables;
  for (size_t i = 0; i < data->baseclass_count; ++i) {
    uint16_t baseclass_index = get_baseclass_from_array(data, i);
    demogobbler_sendtable *table = sendtables + baseclass_index;
    iterate_props(thisptr, data, table);
  }

  iterate_props(thisptr, data, sendtables + data->dt_index);
}

#define CHECK_ERR()                                                                                \
  if (thisptr->error)                                                                              \
  goto end

static void parse_serverclass(estate_init_state *thisptr, size_t i) {
  // Reset iteration state
  propdata data;
  memset(&data, 0, sizeof(propdata));
  data.excluded_props = thisptr->ent_scrap->excluded_props;
  data.dts_with_excludes = thisptr->ent_scrap->dts_with_excludes;
  data.serverclass_index = i;

  demogobbler_serverclass *cls = thisptr->entity_state->serverclasses + i;
  hashtable_entry entry =
      demogobbler_hashtable_get(&thisptr->ent_scrap->dt_hashtable, cls->datatable_name);
  data.dt_index = entry.value;

  if (entry.str == NULL) {
    thisptr->error = true;
    thisptr->error_message = "No datatable found for serverclass";
    goto end;
  }

  data.dts_with_excludes.item_count = 0;
  demogobbler_pes_clear(&data.excluded_props);
  gather_excludes(thisptr, &data, data.dt_index);
  CHECK_ERR();
  gather_propdata(thisptr, &data, data.dt_index);
  CHECK_ERR();

  thisptr->entity_state->class_datas[i].props = demogobbler_arena_allocate(
      &thisptr->entity_state->memory_arena, sizeof(demogobbler_sendprop) * data.max_props,
      alignof(demogobbler_sendprop));
  thisptr->entity_state->class_datas[i].prop_count = 0;
  thisptr->entity_state->class_datas[i].dt_name =
      (thisptr->entity_state->sendtables + data.dt_index)->name;

  gather_props(thisptr, &data);
  CHECK_ERR();
  sort_props(thisptr, thisptr->entity_state->class_datas + i);
  CHECK_ERR();
end:;
}

demogobbler_parse_result demogobbler_estate_init(estate *thisptr, entity_parse_scrap* scrap, estate_init_args args) {
  demogobbler_parse_result result;
  if(thisptr->edicts != NULL) {
    result.error = true;
    result.error_message = "Entity state already initialized";
    return result;
  }

  thisptr->should_store_props = args.should_store_props;
  thisptr->sendtables = args.message->sendtables;
  thisptr->serverclasses = args.message->serverclasses;
  thisptr->serverclass_count = args.message->serverclass_count;
  thisptr->sendtable_count = args.message->sendtable_count;
  if(thisptr->memory_arena.first_block_size == 0) {
    thisptr->memory_arena = demogobbler_arena_create(1 << 15);
  }
  thisptr->edicts =
      demogobbler_arena_allocate(&thisptr->memory_arena, sizeof(edict) * MAX_EDICTS, alignof(edict));
  memset(thisptr->edicts, 0, sizeof(edict) * MAX_EDICTS);

  estate_init_state state;
  memset(&state, 0, sizeof(state));
  state.args = args;
  state.entity_state = thisptr;
  state.ent_scrap = scrap;

  create_dt_hashtable(&state);

  if (!state.error) {
    if (scrap->excluded_props.arr == NULL) {
      scrap->excluded_props = demogobbler_pes_create(256);
    }
    if (scrap->dts_with_excludes.arr == NULL) {
      scrap->dts_with_excludes = demogobbler_hashtable_create(256);
    }
    size_t array_size = sizeof(serverclass_data) * thisptr->serverclass_count;
    thisptr->class_datas =
        demogobbler_arena_allocate(&thisptr->memory_arena, array_size, alignof(serverclass_data));
    memset(thisptr->class_datas, 0, array_size);

    if (args.flatten_datatables) {
      for (size_t i = 0; i < thisptr->serverclass_count; ++i) {
        parse_serverclass(&state, i);
      }
    }
  }

  result.error = state.error;
  result.error_message = state.error_message;

  return result;
}

static void free_inner_value(prop_value_inner* value, demogobbler_sendproptype prop_type) {
  if(prop_type == sendproptype_vector3) {
    free(value->v3_val);
  } else if(prop_type == sendproptype_vector2) {
    free(value->v2_val);
  } else if(prop_type == sendproptype_string) {
    free(value->str_val->str);
    free(value->str_val);
  }
}

static void free_props(edict* ent) {
  ent_prop* current = ent->values;
  while(current) {
    ent_prop* temp = current->next;
    if(current->value.prop->proptype == sendproptype_vector3) {
      free_inner_value(&current->value.value, current->value.prop->proptype);
    } else if(current->value.prop->proptype == sendproptype_vector2) {
      free_inner_value(&current->value.value, current->value.prop->proptype);
    } else if(current->value.prop->proptype == sendproptype_string) {
      free_inner_value(&current->value.value, current->value.prop->proptype);
    } else if(current->value.prop->proptype == sendproptype_array) {
      for(size_t i=0; i < current->value.prop->array_num_elements; ++i) {
        free_inner_value(current->value.value.arr_val->values + i, current->value.prop->array_prop->proptype);
      }
      free(current->value.value.arr_val->values);
      free(current->value.value.arr_val);
    }
    free(current);
    current = temp;
  }
}

void demogobbler_estate_free(estate* thisptr) {
  if(thisptr->should_store_props) {
    for(size_t i=0; i < MAX_EDICTS; ++i) {
      free_props(thisptr->edicts + i);
    }
  }
  demogobbler_arena_free(&thisptr->memory_arena);
}

void demogobbler_parser_init_estate(parser *thisptr, demogobbler_datatables_parsed *message) {
  estate_init_args args;
  args.should_store_props = thisptr->m_settings.store_props;
  args.flatten_datatables = thisptr->m_settings.flattened_props_handler != NULL;
  args.message = message;
  args.version_data = &thisptr->demo_version;

  demogobbler_parse_result result = demogobbler_estate_init(&thisptr->state.entity_state, &thisptr->ent_scrap, args);

  if (result.error) {
    thisptr->error = true;
    thisptr->error_message = result.error_message;
  }

  if (!thisptr->error && thisptr->m_settings.flattened_props_handler) {
    thisptr->m_settings.flattened_props_handler(&thisptr->state);
  }
}

serverclass_data *demogobbler_estate_serverclass_data(parser *thisptr, size_t index) {
  estate_init_state state;
  memset(&state, 0, sizeof(state));
  state.args.version_data = &thisptr->demo_version;
  state.ent_scrap = &thisptr->ent_scrap;

  state.entity_state = &thisptr->state.entity_state;

  if (state.entity_state->class_datas[index].dt_name == NULL) {
    parse_serverclass(&state, index);
  }

  return state.entity_state->class_datas + index;
}

static void copy_into_inner_value(prop_value_inner* dest, const prop_value_inner* src, demogobbler_sendproptype prop_type) {
  if(prop_type == sendproptype_vector3) {
    memcpy(dest->v3_val, src->v3_val, sizeof(vector3_value));
  } else if(prop_type == sendproptype_vector2) {
    memcpy(dest->v2_val, src->v2_val, sizeof(vector2_value));
  } else if (prop_type == sendproptype_string) {
    size_t current_len = dest->str_val->len;
    size_t value_len = src->str_val->len;

    if(value_len == 0) {
      free(dest->str_val->str);
      memset(dest->str_val, 0, sizeof(string_value));
    } else {
      if(current_len < value_len) {
        // If new value too large, realloc the string
        // Also works in the null pointer case
        dest->str_val->str = realloc(dest->str_val->str, value_len);
        dest->str_val->len = value_len;
        current_len = value_len;
      }
      strncpy(dest->str_val->str, src->str_val->str, current_len);
    }
  } else {
    memcpy(dest, src, sizeof(prop_value_inner));
  }
}

static void copy_into_prop(ent_prop* prop, const prop_value* value) {
  demogobbler_sendproptype prop_type = value->prop->proptype;

  if(prop_type != sendproptype_array) {
    copy_into_inner_value(&prop->value.value, &value->value, prop_type);
  } else {
    demogobbler_sendproptype array_prop_type = value->prop->array_prop->proptype;
    for(size_t i=0; i < value->value.arr_val->array_size; ++i) {
      copy_into_inner_value(prop->value.value.arr_val->values + i, value->value.arr_val->values + i, array_prop_type);
    }
  }
}

static void alloc_inner_value(prop_value_inner* dest, demogobbler_sendprop* prop) {
  if(prop->proptype == sendproptype_vector3) {
    dest->v3_val = malloc(sizeof(vector3_value));
    memset(dest->v3_val, 0, sizeof(vector3_value));
  } else if (prop->proptype == sendproptype_vector2) {
    dest->v2_val = malloc(sizeof(vector2_value));
    memset(dest->v2_val, 0, sizeof(vector2_value));
  } else if (prop->proptype == sendproptype_string) {
    dest->str_val = malloc(sizeof(string_value));
    memset(dest->str_val, 0, sizeof(string_value));
  } 
}

static ent_prop* alloc_prop(demogobbler_sendprop* prop) {
  ent_prop* rval = malloc(sizeof(ent_prop));
  memset(rval, 0, sizeof(ent_prop));
  rval->value.prop = prop;
  if(prop->proptype == sendproptype_array) {
    rval->value.value.arr_val = malloc(sizeof(array_value));
    memset(rval->value.value.arr_val, 0, sizeof(array_value));
    rval->value.value.arr_val->array_size = prop->array_num_elements;
    rval->value.value.arr_val->values = malloc(sizeof(prop_value_inner) * prop->array_num_elements);
    memset(rval->value.value.arr_val->values, 0, sizeof(prop_value_inner) * prop->array_num_elements);

    for(size_t i=0; i < prop->array_num_elements; ++i) {
      alloc_inner_value(rval->value.value.arr_val->values + i, prop->array_prop);
    }
  } else if(prop->proptype != sendproptype_int || prop->proptype != sendproptype_float) {
    alloc_inner_value(&rval->value.value, prop);
  }

  return rval;
}

static void update_props(edict* ent, const ent_update* update) {
  ent_prop* current = ent->values;

  for(size_t i=0; i < update->prop_value_array_size; ++i) {
    const prop_value* value = update->prop_value_array + i;
    ent_prop* prop;

    // Walk the linked list of props until we arrive at the insertion point
    if(current) {
      while(current->next && current->next->value.prop < value->prop) {
        current = current->next;
      }
    }

    if(current == NULL || current->value.prop > value->prop) {
      // We are at the front of the list, change the head pointer
      prop = alloc_prop(value->prop);
      prop->next = ent->values;
      ent->values = prop;
    } else if (current->value.prop == value->prop) {
      prop = current;
    } else {
      // Did not match, insert a new node either in the middle or back of the list
      prop = alloc_prop(value->prop);
      prop->next = current->next;
      current->next = prop;
    }
    copy_into_prop(prop, value);
    current = prop;
  }
}

demogobbler_parse_result demogobbler_estate_update(estate* entity_state, const packetentities_data* data) {
  demogobbler_parse_result result = {0};
  bool should_store_props = entity_state->should_store_props;

  for(size_t i=0; i < data->ent_updates_count; ++i) {
    const ent_update* update = data->ent_updates + i;
    edict* ent = entity_state->edicts + update->ent_index;
    if(update->update_type == 2) {
      // Enter pvs
      ent->explicitly_deleted = false;
      ent->exists = true;
      ent->datatable_id = update->datatable_id;
      ent->handle = update->handle;
      ent->in_pvs = true;
      
      if(should_store_props) {
        update_props(ent, update);
      }
    } else if( update->update_type == 0) {
      // Delta
      if(should_store_props) {
        update_props(ent, update);
      }
    } else if (update->update_type == 1) {
      // Leave PVS
      ent->in_pvs = false;
    } else if (update->update_type == 3) {
      if(should_store_props) {
        free_props(ent);
      }
      memset(ent, 0, sizeof(edict));
    }
  }


  for(size_t i=0; i < data->explicit_deletes_count; ++i) {
    edict* ent = entity_state->edicts + data->explicit_deletes[i];
    if(should_store_props) {
      free_props(ent);
    }
    memset(ent, 0, sizeof(edict));
    ent->explicitly_deleted = true;
  }

  return result;
}
