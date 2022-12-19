#include "demogobbler.h"
#include "test_demos.hpp"
#include "gtest/gtest.h"

struct baseline_state {
  dg_arena memory;
  dg_datatables_parsed datatables;
  dg_stringtables_parsed stringtables;
  dg_svc_create_stringtable instancebaselines;
  dg_demver_data demver_data;
  estate entity_state;

  baseline_state() {
    memory = dg_arena_create(1 << 15);
    memset(&datatables, 0, sizeof(datatables));
    memset(&stringtables, 0, sizeof(stringtables));
    memset(&instancebaselines, 0, sizeof(instancebaselines));
    memset(&entity_state, 0, sizeof(entity_state));
  }

  ~baseline_state() { 
    dg_arena_free(&memory);
    dg_estate_free(&entity_state);
  }
};

static void parse_baselines(baseline_state* state) {
  dg_instancebaseline_args args;
  dg_alloc_state allocator = dg_arena_create_allocator(&state->memory);
  args.permanent_allocator = args.allocator = &allocator;
  args.demver_data = &state->demver_data;
  args.estate_ptr = &state->entity_state;
  dg_sentry* sentry = &state->instancebaselines.stringtable;

  if(sentry->values == NULL)
  {
    std::printf("skipping write test, no baselines parsed for demo\n");
    return;
  }

  for(size_t i=0; i < sentry->values_length; ++i) {
    dg_sentry_value* value = sentry->values + i;
    dg_ent_update update;
    args.datatable_id = atoi(value->stored_string);
    args.output = &update;
    args.stream = &value->userdata;

    dg_parse_result result = dg_parse_instancebaseline(&args);
    EXPECT_EQ(result.error, false) << result.error_message;

    bitwriter writer;
    bitwriter_init(&writer, value->userdata_length);
#ifdef GROUND_TRUTH_CHECK
    writer.truth_data = value->userdata.data;
    writer.truth_data_offset = value->userdata.bitoffset;
    writer.truth_size_bits = value->userdata.bitsize;
#endif
    dg_bitwriter_write_props(&writer, &state->demver_data, &update);
    unsigned bits = dg_bitstream_bits_left(&value->userdata);
    EXPECT_LE(bits - writer.bitoffset, 7);
    bitwriter_free(&writer);
  }

  bitwriter writer;

  bitwriter_init(&writer, state->instancebaselines.data_length);
#ifdef GROUND_TRUTH_CHECK
  writer.truth_data = state->instancebaselines.data.data;
  writer.truth_data_offset = state->instancebaselines.data.bitoffset;
  writer.truth_size_bits = state->instancebaselines.data.bitsize;
#endif
  dg_sentry_write_args write_args;
  write_args.input = sentry;
  write_args.writer = &writer;

  auto result = dg_write_stringtable_entry(&write_args);
  EXPECT_EQ(result.error, false);
  EXPECT_EQ(dg_bitstream_bits_left(&state->instancebaselines.data), writer.bitoffset);
  bitwriter_free(&writer);
}

static void write_baselines(baseline_state* state) {

}

static void handle_dt(parser_state *_state, dg_datatables_parsed *value) {
  baseline_state *state = (baseline_state *)_state->client_state;
  state->datatables = *value;
}

static void handle_version(parser_state *_state, dg_demver_data demver_data) {
  baseline_state *state = (baseline_state *)_state->client_state;
  state->demver_data = demver_data;
}

static void handle_st(parser_state *_state, dg_stringtables_parsed *value) {
  baseline_state *state = (baseline_state *)_state->client_state;
  state->stringtables = *value;
}

static void handle_packet(parser_state *_state, packet_parsed *value) {
  baseline_state *state = (baseline_state *)_state->client_state;
  for (size_t i = 0; i < value->message_count; ++i) {
    packet_net_message *msg = value->messages + i;
    if (msg->mtype == svc_create_stringtable &&
        strcmp("instancebaseline", msg->message_svc_create_stringtable.name) == 0) {
      state->instancebaselines = msg->message_svc_create_stringtable;
    }
  }
}


static dg_parse_result get_baseline_state(const char *filepath, baseline_state *state) {
  dg_settings settings;
  dg_settings_init(&settings);
  settings.client_state = state;
  settings.datatables_parsed_handler = handle_dt;
  settings.stringtables_parsed_handler = handle_st;
  settings.packet_parsed_handler = handle_packet;
  settings.demo_version_handler = handle_version;
  settings.temp_alloc_state.allocator = &state->memory;
  settings.permanent_alloc_state.allocator = &state->memory;
  auto result = dg_parse_file(&settings, filepath);

  if (result.error)
    return result;

  estate_init_args args;
  dg_alloc_state allocator = dg_arena_create_allocator(&state->memory);
  args.allocator = &allocator;
  args.flatten_datatables = true;
  args.message = &state->datatables;
  args.should_store_props = false;
  args.version_data = &state->demver_data;
  result = dg_estate_init(&state->entity_state, args);

  if (result.error)
    return result;

  parse_baselines(state);

  return result;
}

TEST(baselines, parse_and_write) {
  for (auto &demo : get_test_demos()) {
    baseline_state base;
    std::cout << "[----------] " << demo << std::endl;
    auto result = get_baseline_state(demo.c_str(), &base);
    EXPECT_EQ(result.error, false) << result.error_message;
  }
}

struct baselinegen_state {
  dg_arena memory;
  dg_datatables_parsed datatables;
  dg_demver_data demver_data;
  estate entity_state;

  baselinegen_state() {
    memory = dg_arena_create(1 << 15);
    memset(&datatables, 0, sizeof(datatables));
    memset(&entity_state, 0, sizeof(entity_state));
  }

  ~baselinegen_state() { 
    dg_arena_free(&memory);
    dg_estate_free(&entity_state);
  }
};


static void handle_dt_gen(parser_state *_state, dg_datatables_parsed *value) {
  baselinegen_state *state = (baselinegen_state *)_state->client_state;
  state->datatables = *value;
}

static void handle_version_gen(parser_state *_state, dg_demver_data demver_data) {
  baselinegen_state *state = (baselinegen_state *)_state->client_state;
  state->demver_data = demver_data;
}

static bool test_baseline(const char* type, dg_ent_update* update, dg_ent_update* orig, const dg_serverclass_data* data) {
  if(update->prop_value_array_size == data->prop_count) {
    return true;
  } else {
    for(size_t i=0; i < update->prop_value_array_size; ++i) {
      prop_value* prev_value = update->prop_value_array + i - 1;
      prop_value* prev_value_orig = NULL;
      dg_sendprop* prev_prop = data->props + i - 1;
      if(orig)
        prev_value_orig = orig->prop_value_array + i - 1;
      prop_value* value = update->prop_value_array + i;
      prop_value* value_orig = NULL;
      dg_sendprop* prop = data->props + i;
      if(orig)
        value_orig = orig->prop_value_array + i;
      if(i != value->prop_index) {
        std::printf("%s : %u index on datatable [%lu] diverges\n", type, i, update->datatable_id);
        return false;
      }
    }

    std::printf("%s : %u  props missing with [%lu] / [%lu]\n", type, update->datatable_id, update->prop_value_array_size, data->prop_count);

    return false;
  }
}

bool test_baseline(uint32_t datatable_id, const dg_serverclass_data *data, estate* estate, const dg_demver_data* demver_data, dg_arena* arena) {
  if(data->props == NULL)
    return true;
  
  dg_ent_update update;
  update.datatable_id = datatable_id;
  dg_init_baseline(&update, data, arena);

  bool baseline_is_good = test_baseline("baseline", &update, NULL, data);
  EXPECT_EQ(baseline_is_good, true);
  if(!baseline_is_good)
    return baseline_is_good;

  bitwriter writer;
  bitwriter_init(&writer, 1024);
  dg_bitwriter_write_props(&writer, demver_data, &update);

  dg_instancebaseline_args args;
  dg_ent_update output;
  output.datatable_id = datatable_id;

  auto alligator = dg_arena_create_allocator(arena);
  auto stream = bitstream_create(writer.ptr, writer.bitoffset);
  args.permanent_allocator = args.allocator = &alligator;
  args.datatable_id = datatable_id;
  args.demver_data = demver_data;
  args.estate_ptr = estate;
  args.output = &output;
  args.stream = &stream;

  auto result = dg_parse_instancebaseline(&args);
  EXPECT_EQ(result.error, false);
  bool output_is_good = test_baseline("output", &output, &update, data);
  EXPECT_EQ(output_is_good, true);
  bitwriter_free(&writer);

  dg_init_baseline(&update, data, arena);
  bitwriter_init(&writer, 1024);
  dg_bitwriter_write_props(&writer, demver_data, &update);
  
  return output_is_good;
}

static dg_parse_result get_baselinegen_state(const char *filepath, baselinegen_state *state) {
  dg_settings settings;
  dg_settings_init(&settings);
  settings.client_state = state;
  settings.datatables_parsed_handler = handle_dt_gen;
  settings.demo_version_handler = handle_version_gen;
  settings.temp_alloc_state.allocator = &state->memory;
  settings.permanent_alloc_state.allocator = &state->memory;

  auto result = dg_parse_file(&settings, filepath);

  if (result.error)
    return result;

  estate_init_args args;
  dg_alloc_state allocator = dg_arena_create_allocator(&state->memory);
  args.allocator = &allocator;
  args.flatten_datatables = true;
  args.message = &state->datatables;
  args.should_store_props = false;
  args.version_data = &state->demver_data;
  result = dg_estate_init(&state->entity_state, args);

  if (result.error)
    return result;

  for(size_t i=0; i < state->entity_state.serverclass_count; ++i) {
    bool value = test_baseline(i, state->entity_state.class_datas + i, &state->entity_state, &state->demver_data, &state->memory);
    if(!value) {
      result.error = true;
      result.error_message = "baseline wrong";
      break;
    }
  }

  return result;
}


TEST(baselines, generate_and_verify) {
  for (auto &demo : get_test_demos()) {
    baselinegen_state base;
    std::cout << "[----------] " << demo << std::endl;
    if(strcmp(demo.c_str(), "./test_demos/l4d1 37 v1005.dem") == 0) {
      int i = 0;
    }
    auto result = get_baselinegen_state(demo.c_str(), &base);
    EXPECT_EQ(result.error, false);
  }
}
