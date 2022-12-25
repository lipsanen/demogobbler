#include "demogobbler/freddie.hpp"
#include <cstdio>
#include <set>
#include <string.h>

using namespace freddie;

prop_status datatable_change_info::get_prop_status(dg_sendprop *prop) {
  auto it = this->prop_map.find(prop);
  prop_status status;
  if (it == this->prop_map.end()) {
    status.exists = false;
  } else {
    status = it->second;
  }

  return status;
}

datatable_status datatable_change_info::get_datatable_status(uint32_t index) {
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

static void init_value(const dg_sendprop *prop, dg_prop_value_inner *value, dg_alloc_state *arena) {
  memset(value, 0, sizeof(dg_prop_value_inner));
  value->prop_numbits = prop->prop_numbits;
  value->type = dg_sendprop_type(prop);
  value->proptype = prop->proptype;

  if (value->proptype == sendproptype_array) {
    value->arr_val =
        (dg_array_value *)dg_alloc_allocate(arena, sizeof(dg_array_value), alignof(dg_array_value));
    memset(value->arr_val, 0, sizeof(dg_array_value));
    size_t memory_size = prop->array_num_elements * sizeof(dg_prop_value_inner);
    value->arr_val->array_size = prop->array_num_elements;
    value->array_num_elements = prop->array_num_elements;
    value->arr_val->values =
        (dg_prop_value_inner *)dg_alloc_allocate(arena, memory_size, alignof(dg_prop_value_inner));
    memset(value->arr_val->values, 0, memory_size);

    for (size_t array_index = 0; array_index < prop->array_num_elements; ++array_index) {
      init_value(prop->array_prop, value->arr_val->values + array_index, arena);
    }

  } else if (value->proptype == sendproptype_vector2) {
    value->v2_val = (dg_vector2_value *)dg_alloc_allocate(arena, sizeof(dg_vector2_value),
                                                          alignof(dg_vector2_value));
    memset(value->v2_val, 0, sizeof(dg_vector2_value));
    dg_sendprop innerprop = *prop;
    innerprop.proptype = sendproptype_float;
    init_value(&innerprop, &value->v2_val->x, arena);
    init_value(&innerprop, &value->v2_val->y, arena);
  } else if (value->proptype == sendproptype_vector3) {
    value->v3_val = (dg_vector3_value *)dg_alloc_allocate(arena, sizeof(dg_vector3_value),
                                                          alignof(dg_vector3_value));
    memset(value->v3_val, 0, sizeof(dg_vector3_value));
    dg_sendprop innerprop = *prop;
    innerprop.proptype = sendproptype_float;
    init_value(&innerprop, &value->v3_val->x, arena);
    init_value(&innerprop, &value->v3_val->y, arena);
    init_value(&innerprop, &value->v3_val->z, arena);
  
    if(prop->flag_normal) {
      value->v3_val->_sign = dg_vector3_sign_pos;
    } else {
      value->v3_val->_sign = dg_vector3_sign_no;
    }
  } else if (value->proptype == sendproptype_string) {
    value->str_val = (dg_string_value *)dg_alloc_allocate(arena, sizeof(dg_string_value),
                                                          alignof(dg_string_value));
    memset(value->str_val, 0, sizeof(dg_string_value));
    value->str_val->str = (char *)dg_alloc_allocate(arena, 1, 1);
    value->str_val->len = 1;
    value->str_val->str[0] = '\0';
  }
}

dg_parse_result datatable_change_info::convert_props(dg_ent_update *update,
                                                     uint32_t new_datatable_id) {
  dg_parse_result result;
  memset(&result, 0, sizeof(result));
  dg_serverclass_data *data = this->input_estate.class_datas + update->datatable_id;
  dg_serverclass_data *target_data = this->target_estate.class_datas + new_datatable_id;
  for (size_t i = 0; i < update->prop_value_array_size; ++i) {
    auto prop_ptr = update->prop_value_array + i;
    auto sendprop = data->props + prop_ptr->prop_index;
    auto status = get_prop_status(sendprop);

    if (!status.exists) {
      // Mark deleted props as max value
      prop_ptr->prop_index = UINT32_MAX;
      continue;
    }

    auto newprop = target_data->props + status.index;
    prop_ptr->prop_index = status.index; // remap the index
    // TODO: add conversion logic for props
    if (status.flags_changed) {
      init_value(newprop, &prop_ptr->value, &this->allocator);
    }
  }

  // TODO: add explicit delete handling

  // Sort the props back into order if the indexing changes changed the ordering
  std::sort(
      update->prop_value_array, update->prop_value_array + update->prop_value_array_size,
      [](const prop_value &lhs, const prop_value &rhs) { return lhs.prop_index < rhs.prop_index; });

  // Leave the deleted props at the end but mark the array as shorter than it actually is
  while (update->prop_value_array_size > 0 &&
         update->prop_value_array[update->prop_value_array_size - 1].prop_index == UINT32_MAX)
    --update->prop_value_array_size;

  return result;
}

dg_parse_result datatable_change_info::convert_updates(dg_packetentities_data *data) {
  dg_parse_result result;
  memset(&result, 0, sizeof(result));
  for (size_t i = 0; i < data->ent_updates_count && !result.error; ++i) {
    dg_ent_update *update = data->ent_updates + i;
    // We keep track of entity state here in case some entities get deleted
    if (update->update_type == 2) {
      // enter pvs
      auto state = get_datatable_status(update->datatable_id);
      if (state.exists) {
        if (state.props_changed) {
          convert_props(update, state.index);
        }
        update->datatable_id = state.index;
      } else {
        result.error = true;
        result.error_message = "entity has deleted datatable, cannot be converted";
      }
    } else if (update->update_type == 0) {
      // Delta
      auto state = get_datatable_status(update->datatable_id);
      if (state.exists) {
        if (state.props_changed) {
          convert_props(update, state.index);
        }
        update->datatable_id = state.index;
      } else {
        result.error = true;
        result.error_message = "entity has deleted datatable, cannot be converted";
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
  COMPARE_ELEMENT(prop_numbits);

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
      status.flags_changed = true;
      status.exists = false;
      status.index = 0;
      status.target = nullptr;
    } else {
      status.flags_changed = !prop_attributes_equal(first_prop, prop);
      status.exists = true;
      status.index = index;
      status.target = prop;
    }

    changes = status.flags_changed || status.index != i || changes;
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
      info->add_datatable((uint32_t)dt, changes, true);
    }
  }
}

static void get_datatable_indices_from_packetentities(const dg_packetentities_data *data,
                                                      int *datatable_indices) {
  for (size_t i = 0; i < data->ent_updates_count; ++i) {
    datatable_indices[data->ent_updates[i].datatable_id] = 1;
  }
}

static void get_datatable_indices_from_packet(const packet_parsed *packet,
                                              int *datatable_indices) {
  for (size_t i = 0; i < packet->message_count; ++i) {
    auto msg = packet->messages + i;
    if (msg->mtype == svc_packet_entities) {
      get_datatable_indices_from_packetentities(&msg->message_svc_packet_entities.parsed->data,
                                                datatable_indices);
    }
  }
}

// TODO: This is supposed to be a C function, move it out of freddie
void dg_init_baseline(dg_ent_update *baseline, const dg_serverclass_data *target_datatable,
                          dg_alloc_state* allocator) {
  size_t props_size = sizeof(prop_value) * target_datatable->prop_count;
  baseline->prop_value_array =
      (prop_value *)dg_alloc_allocate(allocator, props_size, alignof(prop_value));
  baseline->prop_value_array_size = target_datatable->prop_count;
  baseline->new_way = false;
  for (size_t i = 0; i < target_datatable->prop_count; ++i) {
    dg_sendprop *prop = target_datatable->props + i;
    prop_value *value = baseline->prop_value_array + i;
    value->prop_index = i;

    init_value(prop, &value->value, allocator);
  }
}

static constexpr size_t MAX_INDICES = 512;

static size_t count_datatable_indices(int* datatable_indices) {
  size_t count = 0;
  for(size_t i=0; i < MAX_INDICES; ++i) {
    if(datatable_indices[i] == 1)
      ++count;
  }

  return count;
}

static void convert_create_stringtable(freddie::mallocator *mallocator,
                                       const dg_demver_data *demver_data,
                                       dg_svc_create_stringtable *table,
                                       const freddie::datatable_change_info *info) {
  size_t memory_size = sizeof(dg_sentry_value) * info->baselines_count;
  char BUFFER[4];

  dg_sentry sentry;
  memset(&sentry, 0, sizeof(dg_sentry));
  sentry.max_entries = table->max_entries;
  sentry.values = (dg_sentry_value *)mallocator->alloc(memory_size);
  sentry.values_length = table->num_entries = info->baselines_count;
  memset(sentry.values, 0, memory_size);

  for (size_t i = 0; i < info->baselines_count; ++i) {
    dg_ent_update *update = info->baselines + i;
    //printf("update %d\n", update->ent_index);
    dg_sentry_value *value = sentry.values + i;
    snprintf(BUFFER, sizeof(BUFFER), "%d", update->datatable_id);
    size_t len = strlen(BUFFER);

    value->has_name = true;
    value->has_user_data = true;
    value->entry_bit = true;
    value->stored_string = (char *)mallocator->alloc(len + 1);
    memcpy(value->stored_string, BUFFER, len + 1);

    dg_bitwriter bitwriter;
    bitwriter_init(&bitwriter, 1024);

#if 0
    bitwriter.truth_data = value->userdata.data;
    bitwriter.truth_data_offset = value->userdata.bitoffset;
    bitwriter.truth_size_bits = value->userdata.bitsize;
#endif

    value->userdata = get_start_state(&bitwriter);
    dg_bitwriter_write_props(&bitwriter, demver_data, update);
    unsigned bits = bitwriter.bitoffset;

    while (bits % 8 != 0) {
      bitwriter_write_bit(&bitwriter, 1);
      bits += 1;
    }
    value->userdata_length = bits / 8;

    finalize_stream(&value->userdata, &bitwriter);
    mallocator->attach(bitwriter.ptr);
  }

  dg_bitwriter final_writer;
  bitwriter_init(&final_writer, 1024);
#if 0
  final_writer.truth_data = table->data.data;
  final_writer.truth_data_offset = table->data.bitoffset;
  final_writer.truth_size_bits = table->data.bitsize;
#endif

  dg_sentry_write_args args;
  args.input = &sentry;
  args.writer = &final_writer;

  table->data = get_start_state(&final_writer);
  dg_write_stringtable_entry(&args);
  finalize_stream(&table->data, &final_writer);
  table->flags = 0;
  mallocator->attach(final_writer.ptr);
}

static void convert_baselines(freddie::demo_t *demo, const freddie::datatable_change_info *info) {
  for (size_t i = 0; i < demo->packets.size(); ++i) {
    packet_parsed *packet_ptr = std::get_if<packet_parsed>(&demo->packets[i]->packet);
    if (packet_ptr) {
      for (size_t msg_index = 0; msg_index < packet_ptr->message_count; ++msg_index) {
        packet_net_message *msg = packet_ptr->messages + msg_index;
        if (msg->mtype == svc_create_stringtable) {
          dg_svc_create_stringtable *table = &msg->message_svc_create_stringtable;
          if (strcmp(table->name, "instancebaseline") == 0) {
            convert_create_stringtable(&demo->packets[i]->memory, &demo->demver_data, table, info);
          }
        } else if (msg->mtype == svc_update_stringtable) {
          if (msg->message_svc_update_stringtable.table_id == 5) {
            size_t memory_size =
                (packet_ptr->message_count - msg_index - 1) * sizeof(packet_net_message);
            memmove(packet_ptr->messages + msg_index, packet_ptr->messages + msg_index + 1,
                    memory_size);
            --msg_index;
          }
        }
      }
    }
  }
}

dg_parse_result datatable_change_info::convert_demo(freddie::demo_t *input) {
  dg_parse_result result;
  memset(&result, 0, sizeof(result));

  for (size_t i = 0; i < input->packets.size() && !result.error; ++i) {
    packet_parsed *packet_ptr = std::get_if<packet_parsed>(&input->packets[i]->packet);
    dg_datatables_parsed *dt_ptr = std::get_if<dg_datatables_parsed>(&input->packets[i]->packet);
    if (packet_ptr) {
      for (size_t msg_index = 0; msg_index < packet_ptr->message_count; ++msg_index) {
        auto *netmsg = packet_ptr->messages + msg_index;
        if (netmsg->mtype == svc_packet_entities) {
          result = convert_updates(&netmsg->message_svc_packet_entities.parsed->data);
          break;
        } else if(netmsg->mtype == svc_create_stringtable && 
        strcmp("instancebaseline", netmsg->message_svc_create_stringtable.name) == 0) {
          auto msg = &netmsg->message_svc_create_stringtable;
          result = convert_instancebaselines(&msg->stringtable, &msg->data);
        } else if (netmsg->mtype == svc_update_stringtable && 
        netmsg->message_svc_update_stringtable.table_id == 5) {
          auto msg = &netmsg->message_svc_update_stringtable;
          result = convert_instancebaselines(&msg->parsed_sentry, &msg->data);
        }
      }
    } else if (dt_ptr) {
      input->packets[i]->packet = this->target_datatable;
    }
  }

  return result;
}

datatable_change_info::datatable_change_info(dg_alloc_state allocator) {
  this->allocator = allocator;
  memset(&input_estate, 0, sizeof(input_estate));
  memset(&target_estate, 0, sizeof(target_estate));
  baselines = NULL;
  baselines_count = 0;
  memset(&target_datatable, 0, sizeof(target_datatable));
}

datatable_change_info::~datatable_change_info() {
  dg_estate_free(&input_estate);
  dg_estate_free(&target_estate);
}

dg_parse_result datatable_change_info::init(freddie::demo_t *input, const freddie::demo_t *target) {
  dg_parse_result result;
  memset(&result, 0, sizeof(result));
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
  this->target_datatable = *datatable2; // TODO: memory management outta wazoo
  this->target_demver = target->demver_data;

end:
  return result;
}

struct baseline_conversion_args {
  dg_alloc_state* allocator;
  estate* input_estate;
  const dg_demver_data* target_demver;
  dg_sentry_value* value;
};

static dg_parse_result convert_baseline(baseline_conversion_args* args, datatable_change_info* info) {
  dg_parse_result result;

  memset(&result, 0, sizeof(result));
  dg_ent_update update;

  dg_instancebaseline_args parse_args;
  parse_args.permanent_allocator = parse_args.allocator = args->allocator;
  parse_args.datatable_id = std::atoi(args->value->stored_string);
  parse_args.demver_data = args->target_demver;
  parse_args.estate_ptr = args->input_estate;
  parse_args.output = &update;
  parse_args.stream = &args->value->userdata;

  dg_parse_instancebaseline(&parse_args);

  auto status = info->get_datatable_status(parse_args.datatable_id);
  auto data = dg_estate_serverclass_data(&info->target_estate, &info->target_demver, args->allocator, status.index);

  if(status.index != parse_args.datatable_id) {
    char BUFFER[4];
    auto len = snprintf(BUFFER, sizeof(BUFFER), "%d", status.index) + 1;
    args->value->stored_string = (char*)dg_alloc_allocate(args->allocator, len, 1);
    memcpy(args->value->stored_string, BUFFER, len);
  }

  bitwriter writer;
  bitwriter_init(&writer, 1024);
#if 0
  writer.truth_data = args->value->userdata.data;
  writer.truth_data_offset = args->value->userdata.bitoffset;
  writer.truth_size_bits = args->value->userdata.bitsize;
#endif
  dg_bitwriter_write_props(&writer, args->target_demver, &update);
  dg_alloc_attach(args->allocator, writer.ptr, writer.bitsize / 8);

end:
  return result;
}

dg_parse_result datatable_change_info::convert_instancebaselines(dg_sentry* stringtable, dg_bitstream* data) {
  dg_parse_result result;
  memset(&result, 0, sizeof(result));

  if(stringtable->values == NULL) {
    result.error = true;
    result.error_message = "svc_create_stringtable format not supported, cannot convert";
    return result;
  }

  baseline_conversion_args args;
  args.allocator = &this->allocator;
  args.target_demver = &target_demver;
  args.input_estate = &input_estate;

  for(size_t i=0; i < stringtable->values_length && !result.error; ++i) {
    args.value = stringtable->values + i;
    result = convert_baseline(&args, this);
  }

  if(result.error) {
    return result;
  }

  bitwriter writer;
  bitwriter_init(&writer, 1 << 15);
#if 0
  writer.truth_data = msg->data.data;
  writer.truth_data_offset = msg->data.bitoffset;
  writer.truth_size_bits = msg->data.bitsize;
#endif
  dg_sentry_write_args write_args;
  write_args.input = stringtable;
  write_args.writer = &writer;

  result = dg_write_stringtable_entry(&write_args);

  if(result.error) {
    bitwriter_free(&writer);
    return result;
  }

  unsigned bits = writer.bitoffset;
  unsigned original_message_bits = dg_bitstream_bits_left(data);

  if(writer.error) {
    result.error = true;
    result.error_message = writer.error_message;
  }

  *data = dg_bitstream_create(writer.ptr, writer.bitoffset);
  dg_alloc_attach(&this->allocator, writer.ptr, writer.bitsize / 8);

  return result;
}