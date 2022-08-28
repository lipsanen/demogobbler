#include "parser_entity_state.h"
#include "demogobbler_hashtable.h"
#include "dynamic_array.h"
#include "utils.h"
#include <string.h>

typedef struct {
  size_t max_props;
  hashtable exclude_props_hashtable;
  hashtable dt_hashtable;
  size_t datatable_write_index;
  size_t flattenedprop_index;
  size_t baseclass_array[1024];
  size_t baseclass_count;
  size_t baseclass_index;
} propdata;

static void add_baseclass(propdata *data, size_t dt_index) {
  // TODO: Add bounds checking
  memmove(data->baseclass_array + data->baseclass_index + 1,
          data->baseclass_array + data->baseclass_index,
          sizeof(size_t) * data->baseclass_count - data->baseclass_index);
  data->baseclass_array[data->baseclass_index] = dt_index;
  ++data->baseclass_count;
}

static size_t get_baseclass_from_array(propdata *data, size_t index) {
  return data->baseclass_array[index];
}

static bool set_add_prop(propdata *data, demogobbler_sendprop *prop) {
  hashtable_entry entry;
  entry.str = prop->name;
  entry.value = 0;

  return demogobbler_hashtable_insert(&data->exclude_props_hashtable, entry);
}

static bool is_prop_excluded(propdata *data, demogobbler_sendprop *prop) {
  hashtable_entry entry = demogobbler_hashtable_get(&data->exclude_props_hashtable, prop->name);

  return entry.str != NULL;
}

static void create_dt_hashtable(parser *thisptr, propdata *data,
                                demogobbler_datatables_parsed *datatables) {
  const float FILL_RATE = 0.9f;
  const size_t buckets = (size_t)(datatables->sendtable_count / FILL_RATE);
  data->dt_hashtable = demogobbler_hashtable_create(buckets);
  for (size_t i = 0; i < datatables->sendtable_count && !thisptr->error; ++i) {
    hashtable_entry entry;
    entry.str = datatables->sendtables[i].name;
    entry.value = i;

    if (!demogobbler_hashtable_insert(&data->dt_hashtable, entry)) {
      thisptr->error = true;
      thisptr->error_message = "Hashtable collision with datatable names or ran out of space";
    }
  }
}

static size_t get_baseclass(parser *thisptr, propdata *data,
                            demogobbler_datatables_parsed *datatables, demogobbler_sendprop *prop) {
  if (prop->baseclass == NULL) {
    hashtable_entry entry = demogobbler_hashtable_get(&data->dt_hashtable, prop->dtname);

    if (entry.str == NULL) {
      // No entry found
      thisptr->error = true;
      thisptr->error_message = "Was unable to find datatable pointed to by sendprop";
      return 0;
    }
    prop->baseclass = datatables->sendtables + entry.value;
  }
  return prop->baseclass - datatables->sendtables;
}

static void gather_excludes(parser* thisptr, propdata *data,
                            demogobbler_datatables_parsed *datatables, size_t datatable_index) {
  demogobbler_sendtable *table = datatables->sendtables + datatable_index;

  for (size_t prop_index = 0; prop_index < table->prop_count; ++prop_index) {
    demogobbler_sendprop *prop = table->props + prop_index;

    if (prop->proptype == sendproptype_datatable) {
      size_t baseclass_index = get_baseclass(thisptr, data, datatables, prop);

      if (thisptr->error)
        return;

      gather_excludes(thisptr, data, datatables, baseclass_index);
    } else if (prop->flag_exclude) {
      set_add_prop(data, prop);
    }
  }
}


static void gather_propdata(parser *thisptr, propdata *data,
                            demogobbler_datatables_parsed *datatables, size_t datatable_index) {
  demogobbler_sendtable *table = datatables->sendtables + datatable_index;

  for (size_t prop_index = 0; prop_index < table->prop_count; ++prop_index) {
    demogobbler_sendprop *prop = table->props + prop_index;
    bool prop_excluded = is_prop_excluded(data, prop);
    if(prop_excluded)
      continue;

    if (prop->proptype == sendproptype_datatable) {
      size_t baseclass_index = get_baseclass(thisptr, data, datatables, prop);

      if (thisptr->error)
        return;

      if (!prop->flag_collapsible) {
        add_baseclass(data, baseclass_index);
        gather_propdata(thisptr, data, datatables, baseclass_index);
        ++data->baseclass_index;
      } else {
        gather_propdata(thisptr, data, datatables, baseclass_index);
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

static void sort_props(parser *thisptr, flattened_props *class_data) {
  if (thisptr->demo_version.demo_protocol >= 4 && thisptr->demo_version.game != l4d) {
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

static void iterate_props(parser *thisptr, propdata *data,
                          demogobbler_datatables_parsed *datatables, demogobbler_sendtable *table) {
  estate *entstate_ptr = &thisptr->state.entity_state;
  for (size_t prop_index = 0; prop_index < table->prop_count; ++prop_index) {
    demogobbler_sendprop *prop = table->props + prop_index;
    if (prop->proptype == sendproptype_datatable) {
      if (prop->flag_collapsible)
        iterate_props(thisptr, data, datatables, prop->baseclass);
    } else if (!prop->flag_exclude && !prop->flag_insidearray) {
      bool prop_excluded = is_prop_excluded(data, prop);
      if (!prop_excluded) {
        size_t flattenedprop_index =
            entstate_ptr->class_props[data->datatable_write_index].prop_count;
        demogobbler_sendprop *dest =
            entstate_ptr->class_props[data->datatable_write_index].props + flattenedprop_index;
        memcpy(dest, prop, sizeof(demogobbler_sendprop));
        ++entstate_ptr->class_props[data->datatable_write_index].prop_count;
      }
    }
  }
}

static void gather_props(parser *thisptr, propdata *data,
                         demogobbler_datatables_parsed *datatables) {
  for (size_t i = 0; i < data->baseclass_count; ++i) {
    size_t index = get_baseclass_from_array(data, i);
    demogobbler_sendtable *table = datatables->sendtables + index;
    iterate_props(thisptr, data, datatables, table);
  }

  iterate_props(thisptr, data, datatables, datatables->sendtables + data->datatable_write_index);
}

void demogobbler_parser_init_estate(parser *thisptr, demogobbler_datatables_parsed *datatables) {
  estate *entstate_ptr = &thisptr->state.entity_state;
  entstate_ptr->edicts = demogobbler_arena_allocate(&thisptr->memory_arena,
                                                    sizeof(edict) * MAX_EDICTS, alignof(edict));
  memset(entstate_ptr->edicts, 0, sizeof(edict) * MAX_EDICTS);

  propdata data;
  memset(&data, 0, sizeof(propdata));
  create_dt_hashtable(thisptr, &data, datatables);

  if (thisptr->error)
    goto end;

  data.exclude_props_hashtable = demogobbler_hashtable_create(256);
  size_t array_size = sizeof(flattened_props) * datatables->sendtable_count;
  entstate_ptr->class_props =
      demogobbler_arena_allocate(&thisptr->memory_arena, array_size, alignof(flattened_props));
  memset(entstate_ptr->class_props, 0, array_size);

  for (size_t i = 0; i < datatables->sendtable_count; ++i) {
    data.datatable_write_index = i;
    data.flattenedprop_index = 0;
    data.max_props = 0;
    data.baseclass_count = data.baseclass_index = 0;
    memset(data.exclude_props_hashtable.arr, 0,
           data.exclude_props_hashtable.max_items * sizeof(hashtable_entry));
    gather_excludes(thisptr, &data, datatables, i);
    gather_propdata(thisptr, &data, datatables, i);

    entstate_ptr->class_props[i].props = demogobbler_arena_allocate(
        &thisptr->memory_arena, sizeof(demogobbler_sendprop) * data.max_props,
        alignof(demogobbler_sendprop));
    entstate_ptr->class_props[i].prop_count = 0;

    gather_props(thisptr, &data, datatables);
    sort_props(thisptr, entstate_ptr->class_props + i);

    if (thisptr->error)
      goto end;
  }

  if (thisptr->m_settings.entity_state_init_handler)
    thisptr->m_settings.entity_state_init_handler(&thisptr->state);

end:
  demogobbler_hashtable_free(&data.exclude_props_hashtable);
  demogobbler_hashtable_free(&data.dt_hashtable);
}
