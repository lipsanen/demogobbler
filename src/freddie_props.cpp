#include "demogobbler/freddie.hpp"
#include <cstdio>
#include <string.h>

using namespace freddie;

prop_status datatable_change_info::get_prop_status(dg_sendprop *prop) const {
  auto it = this->prop_map.find(prop);
  prop_status status;
  if (it == this->prop_map.end()) {
    status.exists = false;
  } else {
    status = it->second;
  }

  return status;
}

datatable_status datatable_change_info::get_datatable_status(uint32_t index) const {
  if (index >= this->datatables.size()) {
    datatable_status status;
    status.props_changed = true;
    status.exists = false;
    return status;
  } else {
    return this->datatables[index];
  }
}

void datatable_change_info::add_datatable(uint32_t new_index, bool props_changed, bool exists) {
  datatable_status status;
  status.index = new_index;
  status.props_changed = props_changed;
  status.exists = exists;
  this->datatables.push_back(status);
}

void datatable_change_info::add_prop(dg_sendprop *prop, prop_status status) {
  this->prop_map[prop] = status;
}

void datatable_change_info::print(bool should_print_props) {
  for (size_t i = 0; i < datatables.size(); ++i) {
    auto ptr = input_estate.class_datas + i;
    auto status = datatables[i];
    bool unchanged = status.exists && !status.props_changed && status.index == i;
    if (!unchanged) {
      std::printf("[%lu] Datatable %s", i, ptr->dt_name);
      if (!status.exists) {
        std::printf(" deleted\n");
        continue;
      }

      if (status.props_changed && status.exists) {
        std::printf(" changed");
      }

      if (status.index != i && status.exists) {
        std::printf(" \tindex changed %ld -> %u", i, status.index);
      }

      std::printf("\n");

      if (should_print_props) {
        this->print_props(i);
      }
    }
  }
}

void datatable_change_info::print_props(uint32_t datatable_id) {
  dg_serverclass_data *data = input_estate.class_datas + datatable_id;
  char name[64];
  for (size_t i = 0; i < data->prop_count; ++i) {
    dg_sendprop *prop = data->props + i;
    auto status = get_prop_status(prop);
    bool any_changes = status.flags_changed || !status.exists || status.index != i;

    if (any_changes) {
      dg_sendprop_name(name, sizeof(name), prop);
      std::printf("\t[%lu] Prop %s", i, name);
      if (!status.exists) {
        std::printf(" deleted");
      }
      if (status.flags_changed && status.exists) {
        std::printf(" flags changed");
      }
      if (status.index != i && status.exists) {
        std::printf(" index changed %lu -> %u", i, status.index);
      }
      std::printf("\n");
    }
  }
}


dg_parse_result datatable_change_info::convert_props(dg_ent_update* update) const {
  dg_parse_result result;
  memset(&result, 0, sizeof(result));
  dg_serverclass_data* data = input_estate.class_datas + update->datatable_id;
  for(size_t i=0; i < update->prop_value_array_size; ++i) {
    auto prop_ptr = update->prop_value_array + i;
    auto sendprop = data->props + prop_ptr->prop_index;
    auto status = get_prop_status(sendprop);

    if(!status.exists) {
      // Mark deleted props as max value
      prop_ptr->prop_index = UINT32_MAX;
      continue;
    }

    if(status.index != prop_ptr->prop_index) {
      prop_ptr->prop_index = status.index; // remap the index
    }
    // TODO: add conversion logic for props
    if(status.flags_changed) {
      memset(&prop_ptr->value, 0, sizeof(prop_ptr->value));
    }
  }

  // TODO: add explicit delete handling

  // Sort the props back into order if the indexing changes changed the ordering
  std::sort(update->prop_value_array, update->prop_value_array + update->prop_value_array_size, [](const prop_value& lhs, const prop_value& rhs) {
    return lhs.prop_index < rhs.prop_index;
  });

  // Leave the deleted props at the end but mark the array as shorter than it actually is
  while(update->prop_value_array_size > 1 && update->prop_value_array[update->prop_value_array_size - 1].prop_index == UINT32_MAX)
    --update->prop_value_array_size;
  
  return result;
}


dg_parse_result datatable_change_info::convert_demo(freddie::demo_t* input) const {
  dg_parse_result result;
  memset(&result, 0, sizeof(result));

  for (size_t i = 0; i < input->packets.size(); ++i) {
    packet_parsed *packet_ptr = std::get_if<packet_parsed>(&input->packets[i]->packet);
    if(packet_ptr) {
      for(size_t i=0; i < packet_ptr->message_count; ++i) {
        auto* netmsg = packet_ptr->messages + i;
        if(netmsg->mtype == svc_packet_entities) {
          convert_updates(&netmsg->message_svc_packet_entities.parsed->data);
          break;
        }
      }
    }
  }
  
  return result;
}

dg_parse_result datatable_change_info::convert_updates(dg_packetentities_data *data) const {
  dg_parse_result result;
  bool deletes = false;
  memset(&result, 0, sizeof(result));
  for (size_t i = 0; i < data->ent_updates_count && !result.error; ++i) {
    bool should_delete = false;
    dg_ent_update *update = data->ent_updates + i;
    // We keep track of entity state here in case some entities get deleted
    if (update->update_type == 2) {
      // enter pvs
      auto state = get_datatable_status(update->datatable_id);
      if (state.exists) {
        update->datatable_id = state.index;
        if(state.props_changed) {
          convert_props(update);
        }
      } else {
        result.error = true;
        result.error_message = "entity has deleted datatable, cannot be converted";
      }
    } else if (update->update_type == 0) {
      // Delta
      auto state = get_datatable_status(update->datatable_id);
      if(state.props_changed) {
        convert_props(update);
      }
    }
  }

  return result;
}

static int32_t get_dt(const estate *state, const char *name, int32_t initial_guess) {
  if ((int32_t)state->serverclass_count > initial_guess &&
      strcmp(state->class_datas[initial_guess].dt_name, name) == 0) {
    return initial_guess;
  } else {
    for (int32_t i = 0; i < (int32_t)state->serverclass_count; ++i) {
      if (strcmp(name, state->class_datas[i].dt_name) == 0) {
        return i;
      }
    }
    return -1;
  }
}

static bool prop_equal(const dg_sendprop *prop1, const char *prop_name) {
  char prop1_name[64];
  dg_sendprop_name(prop1_name, sizeof(prop1_name), prop1);

  return strcmp(prop1_name, prop_name) == 0;
}

static bool prop_attributes_equal(const dg_sendprop *prop1, const dg_sendprop *prop2) {
  bool equal = true;
#define COMPARE_ELEMENT(x)                                                                         \
  if (prop1->x != prop2->x) {                                                                      \
    equal = false;                                                                                 \
  }

  COMPARE_ELEMENT(array_num_elements);
  COMPARE_ELEMENT(flag_cellcoord);
  COMPARE_ELEMENT(flag_cellcoordint);
  COMPARE_ELEMENT(flag_cellcoordlp);
  COMPARE_ELEMENT(flag_changesoften);
  COMPARE_ELEMENT(flag_collapsible);
  COMPARE_ELEMENT(flag_coord);
  COMPARE_ELEMENT(flag_coordmp);
  COMPARE_ELEMENT(flag_coordmplp);
  COMPARE_ELEMENT(flag_coordmpint);
  COMPARE_ELEMENT(flag_coord);
  COMPARE_ELEMENT(flag_insidearray);
  COMPARE_ELEMENT(flag_normal);
  COMPARE_ELEMENT(flag_noscale);
  COMPARE_ELEMENT(flag_proxyalwaysyes);
  COMPARE_ELEMENT(flag_rounddown);
  COMPARE_ELEMENT(flag_roundup);
  COMPARE_ELEMENT(flag_unsigned);
  COMPARE_ELEMENT(flag_xyze);

  return equal;
}

static dg_sendprop *find_prop(const dg_serverclass_data *data, const char *name,
                              size_t initial_index) {
  if (initial_index < data->prop_count) {
    if (prop_equal(data->props + initial_index, name)) {
      return data->props + initial_index;
    }
  }

  for (size_t i = 0; i < data->prop_count; ++i) {
    if (prop_equal(data->props + i, name))
      return data->props + i;
  }

  return nullptr;
}

static bool compare_sendtable_props(freddie::datatable_change_info *info,
                                    const dg_serverclass_data *data1,
                                    const dg_serverclass_data *data2) {
  char buffer[64];
  bool changes = false;
  for (size_t i = 0; i < data1->prop_count; ++i) {
    dg_sendprop *first_prop = data1->props + i;
    dg_sendprop_name(buffer, sizeof(buffer), first_prop);
    dg_sendprop *prop = find_prop(data2, buffer, i);
    freddie::prop_status status;
    size_t index = prop ? prop - data2->props : 0;

    if (!prop) {
      status.flags_changed = false;
      status.exists = false;
      status.index = 0;
      status.target = nullptr;
    } else {
      status.flags_changed = !prop_attributes_equal(first_prop, prop);
      status.exists = true;
      status.index = index;
      status.target = prop;
    }

    changes = status.flags_changed || changes;
    info->add_prop(first_prop, status);
  }

  return changes;
}

static void compare_sendtables(freddie::datatable_change_info *info, const estate *input_state,
                               const estate *target_state) {
  for (int32_t i = 0; i < (int32_t)input_state->serverclass_count; ++i) {
    const char *name = input_state->class_datas[i].dt_name;
    auto dt = get_dt(target_state, name, i);
    if (dt == -1) {
      info->add_datatable(0, true, false);
    } else {
      bool changes = compare_sendtable_props(info, input_state->class_datas + i,
                                             target_state->class_datas + dt);
      info->add_datatable((uint32_t)i, changes, true);
    }
  }
}

datatable_change_info::datatable_change_info() {
  arena = dg_arena_create(1 << 10);
  memset(&input_estate, 0, sizeof(input_estate));
  memset(&target_estate, 0, sizeof(target_estate));
}

datatable_change_info::~datatable_change_info() {
  dg_arena_free(&arena);
  dg_estate_free(&input_estate);
  dg_estate_free(&target_estate);
}

dg_parse_result datatable_change_info::init(const freddie::demo_t *input,
                                            const freddie::demo_t *target) {
  if (arena.block_count > 0)
    abort(); // trying to init state again leaks memory
  dg_parse_result result;
  memset(&result, 0, sizeof(result));

  dg_alloc_state allocator = {&arena, (func_dg_alloc)dg_arena_allocate,
                              (func_dg_clear)dg_arena_clear, (func_dg_realloc)dg_arena_reallocate,
                              (func_dg_attach)dg_arena_attach};
  dg_datatables_parsed *datatable1 = input->get_datatables();
  dg_datatables_parsed *datatable2 = target->get_datatables();

  if (datatable1 == nullptr) {
    result.error = true;
    result.error_message = "missing datatable";
    goto end;
  }

  if (datatable2 == nullptr) {
    result.error = true;
    result.error_message = "missing datatable";
    goto end;
  }

  estate_init_args args1;
  estate_init_args args2;
  args2.allocator = args1.allocator = &allocator;
  args2.flatten_datatables = args1.flatten_datatables = true;
  args2.should_store_props = args1.should_store_props = false;
  args1.message = datatable1;
  args1.version_data = &input->demver_data;
  args2.message = datatable2;
  args2.version_data = &target->demver_data;

  dg_estate_init(&input_estate, args1);
  dg_estate_init(&target_estate, args2);

  compare_sendtables(this, &input_estate, &target_estate);

end:
  return result;
}
