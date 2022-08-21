#include "parser_entity_state.h"
#include "demogobbler_hashtable.h"
#include "dynamic_array.h"
#include "utils.h"
#include <string.h>

static uint32_t get_prop_value(uint16_t datatable_index, uint16_t prop_index) {
  uint32_t value = ((uint32_t)prop_index << 16) | datatable_index;
  return value;
}

static void set_add_prop(dynamic_array *set, uint16_t datatable_index, uint16_t prop_index) {
  uint32_t value = get_prop_value(datatable_index, prop_index);
  uint32_t *ptr = dynamic_array_add(set, 1);
  *ptr = value;
}

static bool has_prop(dynamic_array *set, uint16_t datatable_index, uint16_t prop_index) {
  if (set->count == 0)
    return false;

  uint32_t value = get_prop_value(datatable_index, prop_index);
  uint32_t *arr = set->ptr;

  for (size_t i = 0; i < set->count; ++i) {
    if (arr[i] == value)
      return true;
  }

  return false;
}

static void set_clear(dynamic_array *set) { set->count = 0; }

typedef struct {
  size_t max_props;
  dynamic_array exclude_props;
  hashtable dt_hashtable;
  size_t datatable_write_index;
  size_t flattenedprop_index;
} propdata;

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

static void gather_propdata(parser *thisptr, propdata *data,
                            demogobbler_datatables_parsed *datatables, size_t datatable_index) {
  demogobbler_sendtable *table = datatables->sendtables + datatable_index;

  for (size_t prop_index = 0; prop_index < table->prop_count; ++prop_index) {
    demogobbler_sendprop *prop = table->props + prop_index;
    if (prop->proptype == sendproptype_datatable) {
      size_t baseclass_index = get_baseclass(thisptr, data, datatables, prop);

      if(thisptr->error)
        return;

      gather_propdata(thisptr, data, datatables, baseclass_index);
    } else if (prop->flag_exclude) {
      set_add_prop(&data->exclude_props, datatable_index, prop_index);
    } else if (!prop->flag_insidearray) {
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

static void gather_props(parser *thisptr, propdata *data, demogobbler_datatables_parsed *datatables,
                         size_t datatable_index) {
  estate *entstate_ptr = &thisptr->state.entity_state;
  demogobbler_sendtable *table = datatables->sendtables + datatable_index;

  for (size_t prop_index = 0; prop_index < table->prop_count; ++prop_index) {
    demogobbler_sendprop *prop = table->props + prop_index;
    if (prop->proptype == sendproptype_datatable) {
      size_t baseclass_index = get_baseclass(thisptr, data, datatables, prop);

      if(thisptr->error)
        return;

      gather_props(thisptr, data, datatables, baseclass_index);
    } else if (!prop->flag_exclude && !prop->flag_insidearray) {
      bool prop_excluded = has_prop(&data->exclude_props, datatable_index, prop_index);
      if (!prop_excluded) {
        size_t flattenedprop_index =
            entstate_ptr->class_props[data->datatable_write_index].prop_count;
        demogobbler_sendprop *dest =
            entstate_ptr->class_props[data->datatable_write_index].props + flattenedprop_index;
        memcpy(dest, prop, sizeof(demogobbler_sendprop));

        if(datatable_index != data->datatable_write_index) {
          dest->baseclass = table; // specify the baseclasses in the flattened props
        }
        ++entstate_ptr->class_props[data->datatable_write_index].prop_count;
      }
    }
  }
}

void demogobbler_parser_init_estate(parser *thisptr, demogobbler_datatables_parsed *datatables) {
  estate *entstate_ptr = &thisptr->state.entity_state;
  demogobbler_parser_arena_check_init(thisptr);

  propdata data;
  memset(&data, 0, sizeof(propdata));
  create_dt_hashtable(thisptr, &data, datatables);

  if (thisptr->error)
    goto end;

  dynamic_array_init(&data.exclude_props, 32, sizeof(uint32_t));
  size_t array_size = sizeof(flattened_props) * datatables->sendtable_count;
  entstate_ptr->class_props =
      demogobbler_arena_allocate(&thisptr->memory_arena, array_size, alignof(flattened_props));
  memset(entstate_ptr->class_props, 0, array_size);
  entstate_ptr->classes_count = datatables->sendtable_count;

  for (size_t i = 0; i < datatables->sendtable_count; ++i) {
    data.datatable_write_index = i;
    data.flattenedprop_index = 0;
    data.max_props = 0;
    set_clear(&data.exclude_props);
    gather_propdata(thisptr, &data, datatables, i);

    entstate_ptr->class_props[i].props = demogobbler_arena_allocate(
        &thisptr->memory_arena, sizeof(demogobbler_sendprop) * data.max_props,
        alignof(demogobbler_sendprop));
    entstate_ptr->class_props[i].prop_count = 0;

    gather_props(thisptr, &data, datatables, i);
    sort_props(thisptr, entstate_ptr->class_props + i);

    if (thisptr->error)
      goto end;
  }

  if (thisptr->m_settings.entity_state_init_handler)
    thisptr->m_settings.entity_state_init_handler(&thisptr->state);

end:
  dynamic_array_free(&data.exclude_props);
  demogobbler_hashtable_free(&data.dt_hashtable);
}
