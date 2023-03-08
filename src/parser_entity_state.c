#include "parser_entity_state.h"
#include "demogobbler/alignof_wrapper.h"
#include "demogobbler/allocator.h"
#include "demogobbler.h"
#include "demogobbler/hashtable.h"
#include "demogobbler/utils.h"
#include <string.h>

static void free_inner_value(dg_prop_value_inner *value, dg_sendprop *prop);

dg_eproparr dg_eproparr_init(uint16_t prop_count) {
  dg_eproparr output;
  memset(&output, 0, sizeof(output));
  output.prop_count = prop_count;

  return output;
}

dg_prop_value_inner *dg_eproparr_get(dg_eproparr *thisptr, uint16_t index, bool *new_prop) {
  if (!thisptr->next_prop_indices) {
    thisptr->next_prop_indices = malloc(sizeof(uint16_t) * (thisptr->prop_count + 1));
    thisptr->values = malloc(sizeof(dg_prop_value_inner) * thisptr->prop_count);
    thisptr->next_prop_indices[0] = thisptr->prop_count;
    memset(thisptr->next_prop_indices + 1, 0, sizeof(uint16_t) * thisptr->prop_count);
    memset(thisptr->values, 0, sizeof(dg_prop_value_inner) * thisptr->prop_count);
  }

  uint16_t *prop_index_ptr = thisptr->next_prop_indices + index + 1;
  if (*prop_index_ptr == 0) {
    uint16_t *prev_ptr = prop_index_ptr;
    // First index always either points to first element
    // or is the end-of-array index so no need to bounds check
    while (*prev_ptr == 0) {
      --prev_ptr;
    }
    *prop_index_ptr = *prev_ptr;
    *prev_ptr = index;
    *new_prop = true;
  } else {
    *new_prop = false;
  }

  return thisptr->values + index;
}

void dg_eproparr_free(dg_eproparr *thisptr) {
  free(thisptr->next_prop_indices);
  free(thisptr->values);
}

dg_prop_value_inner *dg_eproparr_next(const dg_eproparr *thisptr, dg_prop_value_inner *current) {
  if (!thisptr->next_prop_indices) {
    return NULL;
  }

  size_t index;
  if (current == NULL) {
    index = 0;
  } else {
    index = (current - thisptr->values) + 1;
  }
  uint16_t next_index = thisptr->next_prop_indices[index];
  if (next_index == thisptr->prop_count) {
    return NULL;
  } else {
    return thisptr->values + next_index;
  }
}

dg_eproplist dg_eproplist_init(void) {
  dg_eproplist list;
  memset(&list, 0, sizeof(dg_eproplist));
  list.head = malloc(sizeof(dg_epropnode));
  memset(list.head, 0, sizeof(dg_epropnode));
  list.head->index = -1;
  return list;
}

dg_epropnode *dg_eproplist_get(dg_eproplist *thisptr, dg_epropnode *initial_guess, uint16_t index,
                               bool *new_prop) {
  dg_epropnode *current = initial_guess;

  if (!current) {
    current = thisptr->head;
  }

  dg_epropnode *rval;

  // Walk the linked list of props until we arrive at the insertion point
  while (current->next && current->next->index < index) {
    current = current->next;
  }

  if (current->next && current->next->index == index) {
    rval = current->next;
    *new_prop = false;
  } else {
    // Did not match, insert a new node either in the middle or back of the list
    rval = malloc(sizeof(dg_epropnode));
    memset(rval, 0, sizeof(dg_epropnode));
    rval->next = current->next;
    current->next = rval;
    *new_prop = true;
  }

  rval->index = index;
  return rval;
}

dg_epropnode *dg_eproplist_next(const dg_eproplist *thisptr, dg_epropnode *current) {
  return current->next;
}

#ifdef DEMOGOBBLER_USE_LINKED_LIST_PROPS
static void dg_eproplist_freeprops(dg_eproplist *thisptr, dg_serverclass_data *data) {
  dg_epropnode *node = thisptr->head;
  // Free the sentinel node from the beginning
  dg_epropnode *temp = node->next;
  free(node);
  node = temp;

  while (node) {
    temp = node->next;
    dg_sendprop *prop = data->props + node->index;
    free_inner_value(&node->value, prop);
    free(node);
    node = temp;
  }
}
#else
static void dg_eproparr_freeprops(dg_eproparr *thisptr, dg_serverclass_data *data) {
  dg_prop_value_inner *value = dg_eproparr_next(thisptr, NULL);

  while (value) {
    size_t index = value - thisptr->values;
    dg_sendprop *prop = data->props + index;
    dg_prop_value_inner *next = dg_eproparr_next(thisptr, value);
    free_inner_value(value, prop);
    value = next;
  }
}
#endif

void dg_eproplist_free(dg_eproplist *thisptr) {
  dg_epropnode *node = thisptr->head;

  while (node) {
    dg_epropnode *temp = node->next;
    free(node);
    node = temp;
  }
}

typedef struct {
  size_t max_props;
  dg_pes excluded_props;
  dg_hashtable dts_with_excludes;
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
  entity_parse_scrap *ent_scrap;
  dg_alloc_state* allocator;
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

// Returns true if either dg_hashtable gets completely full
static bool add_exclude(propdata *data, dg_sendprop *prop) {
  dg_pes_insert(&data->excluded_props, prop);
  dg_hashtable_entry entry;
  entry.str = prop->exclude_name;
  entry.value = 0;
  dg_hashtable_insert(&data->dts_with_excludes, entry);
  return data->excluded_props.item_count != data->excluded_props.max_items &&
         data->dts_with_excludes.item_count != data->dts_with_excludes.max_items;
}

static bool does_datatable_have_excludes(propdata *data, dg_sendtable *table) {
  dg_hashtable_entry entry = dg_hashtable_get(&data->dts_with_excludes, table->name);
  return entry.str != NULL;
}

static bool is_prop_excluded(propdata *data, dg_sendtable *table, dg_sendprop *prop) {
  return dg_pes_has(&data->excluded_props, table, prop);
}

static void create_dt_hashtable(estate_init_state *thisptr) {
  dg_sendtable *sendtables = thisptr->entity_state->sendtables;
  const size_t sendtable_count = thisptr->entity_state->sendtable_count;
  const float FILL_RATE = 0.9f;
  const size_t buckets = (size_t)(sendtable_count / FILL_RATE);

  if (thisptr->ent_scrap->dt_hashtable.arr != NULL) {
    dg_hashtable_clear(&thisptr->ent_scrap->dt_hashtable);
  } else {
    thisptr->ent_scrap->dt_hashtable = dg_hashtable_create(buckets);
  }

  for (size_t i = 0; i < sendtable_count && !thisptr->error; ++i) {
    dg_hashtable_entry entry;
    entry.str = sendtables[i].name;
    entry.value = i;

    if (!dg_hashtable_insert(&thisptr->ent_scrap->dt_hashtable, entry)) {
      thisptr->error = true;
      thisptr->error_message = "Hashtable collision with datatable names or ran out of space";
    }
  }
}

static size_t get_baseclass(estate_init_state *thisptr, propdata *data, dg_sendprop *prop) {
  dg_sendtable *sendtables = thisptr->entity_state->sendtables;
  if (prop->baseclass == NULL) {
    dg_hashtable_entry entry = dg_hashtable_get(&thisptr->ent_scrap->dt_hashtable, prop->dtname);

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
  dg_sendtable *table = thisptr->entity_state->sendtables + datatable_index;

  for (size_t prop_index = 0; prop_index < table->prop_count; ++prop_index) {
    dg_sendprop *prop = table->props + prop_index;

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
  dg_sendtable *sendtables = thisptr->entity_state->sendtables;
  dg_sendtable *table = sendtables + datatable_index;
  bool table_has_excludes = does_datatable_have_excludes(data, table);

  for (size_t prop_index = 0; prop_index < table->prop_count; ++prop_index) {
    dg_sendprop *prop = table->props + prop_index;
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

static uint8_t get_priority_protocol4(dg_sendprop *prop) {
  if (prop->priority >= 64 && prop->flag_changesoften)
    return 64;
  else
    return prop->priority;
}

static void sort_props(estate_init_state *thisptr, dg_serverclass_data *class_data) {
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
        dg_sendprop *prop = class_data->props + i;
        uint8_t prio = get_priority_protocol4(prop);

        if (prio == current_prio) {
          dg_sendprop *dest = class_data->props + start;
          dg_sendprop temp = *dest;
          *dest = *prop;
          *prop = temp;
          ++start;
        }
      }
    }
  } else {
    size_t start = 0;

    for (size_t i = start; i < class_data->prop_count; ++i) {
      dg_sendprop *prop = class_data->props + i;
      if (prop->flag_changesoften) {
        dg_sendprop *dest = class_data->props + start;
        dg_sendprop temp = *dest;
        *dest = *prop;
        *prop = temp;
        ++start;
      }
    }
  }
}

static void iterate_props(estate_init_state *thisptr, propdata *data, dg_sendtable *table) {
  bool table_has_excludes = does_datatable_have_excludes(data, table);
  for (size_t prop_index = 0; prop_index < table->prop_count; ++prop_index) {
    dg_sendprop *prop = table->props + prop_index;
    if (prop->proptype == sendproptype_datatable) {
      if (prop->flag_collapsible)
        iterate_props(thisptr, data, prop->baseclass);
    } else if (!prop->flag_exclude && !prop->flag_insidearray) {
      bool prop_excluded = table_has_excludes && is_prop_excluded(data, table, prop);
      if (!prop_excluded) {
        size_t flattenedprop_index =
            thisptr->entity_state->class_datas[data->serverclass_index].prop_count;
        dg_sendprop *dest =
            thisptr->entity_state->class_datas[data->serverclass_index].props + flattenedprop_index;
        memcpy(dest, prop, sizeof(dg_sendprop));
        ++thisptr->entity_state->class_datas[data->serverclass_index].prop_count;
      }
    }
  }
}

static void gather_props(estate_init_state *thisptr, propdata *data) {
  dg_sendtable *sendtables = thisptr->entity_state->sendtables;
  for (size_t i = 0; i < data->baseclass_count; ++i) {
    uint16_t baseclass_index = get_baseclass_from_array(data, i);
    dg_sendtable *table = sendtables + baseclass_index;
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

  dg_serverclass *cls = thisptr->entity_state->serverclasses + i;
  dg_hashtable_entry entry =
      dg_hashtable_get(&thisptr->ent_scrap->dt_hashtable, cls->datatable_name);
  data.dt_index = entry.value;

  if (entry.str == NULL) {
    thisptr->error = true;
    thisptr->error_message = "No datatable found for serverclass";
    goto end;
  }

  data.dts_with_excludes.item_count = 0;
  dg_pes_clear(&data.excluded_props);
  gather_excludes(thisptr, &data, data.dt_index);
  CHECK_ERR();
  gather_propdata(thisptr, &data, data.dt_index);
  CHECK_ERR();

  thisptr->entity_state->class_datas[i].props =
      dg_alloc_allocate(thisptr->allocator, sizeof(dg_sendprop) * data.max_props,
                        alignof(dg_sendprop));
  thisptr->entity_state->class_datas[i].prop_count = 0;
  thisptr->entity_state->class_datas[i].dt_name =
      (thisptr->entity_state->sendtables + data.dt_index)->name;

  gather_props(thisptr, &data);
  CHECK_ERR();
  sort_props(thisptr, thisptr->entity_state->class_datas + i);
  CHECK_ERR();
end:;
}

dg_parse_result dg_estate_init(estate *thisptr, estate_init_args args) {
  dg_parse_result result;
  if (thisptr->edicts != NULL) {
    result.error = true;
    result.error_message = "Entity state already initialized";
    return result;
  }

  memset(thisptr, 0, sizeof(*thisptr));
  thisptr->should_store_props = args.should_store_props;
  thisptr->sendtables = args.message->sendtables;
  thisptr->serverclasses = args.message->serverclasses;
  thisptr->serverclass_count = args.message->serverclass_count;
  thisptr->sendtable_count = args.message->sendtable_count;
  thisptr->edicts =
      dg_alloc_allocate(args.allocator, sizeof(dg_edict) * MAX_EDICTS, alignof(dg_edict));
  memset(thisptr->edicts, 0, sizeof(dg_edict) * MAX_EDICTS);

  estate_init_state state;
  memset(&state, 0, sizeof(state));
  state.args = args;
  state.entity_state = thisptr;
  state.ent_scrap = &thisptr->scrap;
  state.allocator = args.allocator;

  create_dt_hashtable(&state);

  if (!state.error) {
    if (thisptr->scrap.excluded_props.arr == NULL) {
      thisptr->scrap.excluded_props = dg_pes_create(256);
    }
    if (thisptr->scrap.dts_with_excludes.arr == NULL) {
      thisptr->scrap.dts_with_excludes = dg_hashtable_create(256);
    }
    size_t array_size = sizeof(dg_serverclass_data) * thisptr->serverclass_count;
    thisptr->class_datas =
        dg_alloc_allocate(args.allocator, array_size, alignof(dg_serverclass_data));
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

static void free_inner_value(dg_prop_value_inner *value, dg_sendprop *prop) {
  dg_sendproptype prop_type = prop->proptype;
  if (prop_type == sendproptype_vector3) {
    free(value->v3_val);
  } else if (prop_type == sendproptype_vector2) {
    free(value->v2_val);
  } else if (prop_type == sendproptype_string) {
    free(value->str_val->str);
    free(value->str_val);
  } else if (prop_type == sendproptype_array) {
    for (size_t i = 0; i < prop->array_num_elements; ++i) {
      free_inner_value(value->arr_val->values + i, prop->array_prop);
    }
    free(value->arr_val->values);
    free(value->arr_val);
  }
}

#ifdef DEMOGOBBLER_USE_LINKED_LIST_PROPS
static void free_props(dg_edict *ent, dg_serverclass_data *data) {
  if (ent->exists) {
    dg_eproplist_freeprops(&ent->props, data);
  }
}
#else
static void free_props(dg_edict *ent, dg_serverclass_data *data) {
  if (ent->exists) {
    dg_eproparr_freeprops(&ent->props, data);
    dg_eproparr_free(&ent->props);
  }
}
#endif

void dg_estate_free(estate *thisptr) {
  if (thisptr->should_store_props) {
    for (size_t i = 0; i < MAX_EDICTS; ++i) {
      dg_edict *ent = thisptr->edicts + i;
      free_props(ent, thisptr->class_datas + ent->datatable_id);
    }
    memset(thisptr->edicts, 0, sizeof(dg_edict) * MAX_EDICTS);
  }
  dg_hashtable_free(&thisptr->scrap.dt_hashtable);
  dg_hashtable_free(&thisptr->scrap.dts_with_excludes);
  dg_pes_free(&thisptr->scrap.excluded_props);
}

void dg_parser_init_estate(dg_parser *thisptr, dg_datatables_parsed *message) {
  estate_init_args args;
  args.should_store_props = false;
  args.flatten_datatables = thisptr->m_settings.flattened_props_handler != NULL;
  args.message = message;
  args.version_data = &thisptr->demo_version;
  args.allocator = dg_parser_perm_allocator(thisptr);

  dg_parse_result result = dg_estate_init(&thisptr->state.entity_state, args);

  if (result.error) {
    thisptr->error = true;
    thisptr->error_message = result.error_message;
  }

  if (!thisptr->error && thisptr->m_settings.flattened_props_handler) {
    thisptr->m_settings.flattened_props_handler(&thisptr->state);
  }
}

dg_serverclass_data *dg_estate_serverclass_data(estate *thisptr, const dg_demver_data* demver_data, dg_alloc_state* allocator, size_t index) {
  estate_init_state state;
  memset(&state, 0, sizeof(state));
  state.args.version_data = demver_data;
  state.ent_scrap = &thisptr->scrap;
  state.allocator = allocator;
  state.entity_state = thisptr;

  if (state.entity_state->class_datas[index].dt_name == NULL) {
    parse_serverclass(&state, index);
  }

  return state.entity_state->class_datas + index;
}

static void copy_into_inner_value(dg_prop_value_inner *dest, const dg_prop_value_inner *src,
                                  dg_sendproptype prop_type) {
  if (prop_type == sendproptype_vector3) {
    memcpy(dest->v3_val, src->v3_val, sizeof(dg_vector3_value));
  } else if (prop_type == sendproptype_vector2) {
    memcpy(dest->v2_val, src->v2_val, sizeof(dg_vector2_value));
  } else if (prop_type == sendproptype_string) {
    size_t current_len = dest->str_val->len;
    size_t value_len = src->str_val->len;

    if (value_len == 0) {
      free(dest->str_val->str);
      memset(dest->str_val, 0, sizeof(dg_string_value));
    } else {
      if (current_len < value_len) {
        // If new value too large, realloc the string
        // Also works in the null pointer case
        dest->str_val->str = realloc(dest->str_val->str, value_len);
        dest->str_val->len = value_len;
        current_len = value_len;
      }
      strncpy(dest->str_val->str, src->str_val->str, current_len);
    }
  } else {
    memcpy(dest, src, sizeof(dg_prop_value_inner));
  }
}

static void copy_into_prop(dg_prop_value_inner *dest, const prop_value *value, const dg_sendprop* prop) {
  dg_sendproptype prop_type = prop->proptype;
  if (prop_type != sendproptype_array) {
    copy_into_inner_value(dest, &value->value, prop_type);
  } else {
    dg_sendproptype array_prop_type = prop->array_prop->proptype;
    for (size_t i = 0; i < value->value.arr_val->array_size; ++i) {
      copy_into_inner_value(dest->arr_val->values + i, value->value.arr_val->values + i,
                            array_prop_type);
    }
  }
}

static void alloc_inner_value(dg_prop_value_inner *dest, dg_sendprop *prop) {
  if (prop->proptype == sendproptype_vector3) {
    dest->v3_val = malloc(sizeof(dg_vector3_value));
    memset(dest->v3_val, 0, sizeof(dg_vector3_value));
  } else if (prop->proptype == sendproptype_vector2) {
    dest->v2_val = malloc(sizeof(dg_vector2_value));
    memset(dest->v2_val, 0, sizeof(dg_vector2_value));
  } else if (prop->proptype == sendproptype_string) {
    dest->str_val = malloc(sizeof(dg_string_value));
    memset(dest->str_val, 0, sizeof(dg_string_value));
  } else if (prop->proptype == sendproptype_array) {
    dest->arr_val = malloc(sizeof(dg_array_value));
    memset(dest->arr_val, 0, sizeof(dg_array_value));
    dest->arr_val->values = malloc(sizeof(dg_prop_value_inner) * prop->array_num_elements);
    dest->arr_val->array_size = prop->array_num_elements;
    memset(dest->arr_val->values, 0, sizeof(dg_prop_value_inner) * prop->array_num_elements);
    for (size_t i = 0; i < prop->array_num_elements; ++i) {
      alloc_inner_value(dest->arr_val->values + i, prop->array_prop);
    }
  }
}

size_t number_of_props(dg_eproplist *thisptr) {
  dg_epropnode *node = thisptr->head;
  size_t i;
  for (i = 0; node != NULL; ++i) {
    node = dg_eproplist_next(thisptr, node);
  }
  return i;
}

#ifdef DEMOGOBBLER_USE_LINKED_LIST_PROPS
static void update_props(dg_edict *ent, const dg_ent_update *update, dg_serverclass_data *data) {
  dg_epropnode *node = NULL;
  for (size_t i = 0; i < update->prop_value_array_size; ++i) {
    const prop_value *value = update->prop_value_array + i;
    size_t index = value->prop - data->props;
    bool newprop;
    size_t before = number_of_props(&ent->props);
    node = dg_eproplist_get(&ent->props, node, index, &newprop);
    size_t after = number_of_props(&ent->props);

    if (newprop) {
      alloc_inner_value(&node->value, value->prop);
    }

    if (strcmp(value->prop->name, "m_vecOrigin") == 0 && node->value.v3_val == NULL) {
      int temp = 0;
    }
    copy_into_prop(&node->value, value);
    if (strcmp(value->prop->name, "m_vecOrigin") == 0 && node->value.v3_val == NULL) {
      int temp = 0;
    }
  }
}
#else
static dg_prop_value_inner *getinsert_prop(dg_edict *ent, uint16_t index, dg_sendprop *prop) {
  bool newprop;
  dg_prop_value_inner *value = dg_eproparr_get(&ent->props, index, &newprop);

  if (newprop) {
    alloc_inner_value(value, prop);
  }

  return value;
}

static void update_props(dg_edict *ent, const dg_ent_update *update, dg_serverclass_data *data) {
  for (size_t i = 0; i < update->prop_value_array_size; ++i) {
    const prop_value *value = update->prop_value_array + i;
    dg_sendprop* prop = data->props + value->prop_index;
    dg_prop_value_inner *dest = getinsert_prop(ent, prop - data->props, prop);
    copy_into_prop(dest, value, prop);
  }
}
#endif

dg_parse_result dg_estate_update(estate *entity_state, const dg_packetentities_data *data) {
  dg_parse_result result = {0};
  bool should_store_props = entity_state->should_store_props;

  for (size_t i = 0; i < data->ent_updates_count; ++i) {
    const dg_ent_update *update = data->ent_updates + i;

    if(update->ent_index < 0 || update->ent_index >= MAX_EDICTS) {
      result.error = true;
      result.error_message = "update index was out of bounds";
      goto end;
    }

    dg_edict *ent = entity_state->edicts + update->ent_index;
    if (update->update_type == 2) {
      dg_serverclass_data *data = entity_state->class_datas + update->datatable_id;
      if (should_store_props) {
        bool init_props = false;
        if (ent->exists && ent->datatable_id != update->datatable_id) {
          // game pulled a fast one, existing entity enters pvs with a new datatable???
          free_props(ent, entity_state->class_datas + ent->datatable_id);
          memset(ent, 0, sizeof(dg_edict));
          init_props = true;
        } else if (!ent->exists) {
          init_props = true;
        }

        if (init_props) {
#ifdef DEMOGOBBLER_USE_LINKED_LIST_PROPS
          ent->props = dg_eproplist_init();
#else
          ent->props = dg_eproparr_init(data->prop_count);
#endif
        }
      }

      // Enter pvs
      ent->explicitly_deleted = false;
      ent->exists = true;
      ent->datatable_id = update->datatable_id;
      ent->handle = update->handle;
      ent->in_pvs = true;

      if (should_store_props) {
        update_props(ent, update, data);
      }
    } else if (update->update_type == 0) {
      // Delta
      if (should_store_props) {
        update_props(ent, update, entity_state->class_datas + ent->datatable_id);
      }
    } else if (update->update_type == 1) {
      // Leave PVS
      ent->in_pvs = false;
    } else if (update->update_type == 3) {
      if (should_store_props) {
        free_props(ent, entity_state->class_datas + ent->datatable_id);
      }
      memset(ent, 0, sizeof(dg_edict));
    }
  }

  for (size_t i = 0; i < data->explicit_deletes_count; ++i) {
    dg_edict *ent = entity_state->edicts + data->explicit_deletes[i];
    dg_serverclass_data *data = entity_state->class_datas + ent->datatable_id;
    if (should_store_props) {
      free_props(ent, data);
    }
    memset(ent, 0, sizeof(dg_edict));
    ent->explicitly_deleted = true;
  }

end:
  return result;
}
