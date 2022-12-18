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

static void handle_version(parser_state *_state, dg_demver_data demver_data) {
  baseline_state *state = (baseline_state *)_state->client_state;
  state->demver_data = demver_data;
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
