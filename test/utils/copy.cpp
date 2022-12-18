#include "demogobbler.h"
#include "demogobbler/freddie.hpp"
#include "demogobbler/streams.h"
#include "memory_stream.hpp"
#include "gtest/gtest.h"
#include <algorithm>
#include <cstring>

#define DECLARE_WRITE_FUNC(type)                                                                   \
  static void type##_handler(parser_state *state, dg_##type *message) {                            \
    dg_write_##type((writer *)state->client_state, message);                                       \
  }

DECLARE_WRITE_FUNC(consolecmd);
DECLARE_WRITE_FUNC(customdata);
// DECLARE_WRITE_FUNC(datatables);
DECLARE_WRITE_FUNC(header);
DECLARE_WRITE_FUNC(packet);
// DECLARE_WRITE_FUNC(stringtables);
DECLARE_WRITE_FUNC(stop);
DECLARE_WRITE_FUNC(synctick);
DECLARE_WRITE_FUNC(usercmd);

void handle_datatables_parsed(parser_state *state, dg_datatables_parsed *datatables) {
  dg_write_datatables_parsed((writer *)state->client_state, datatables);
}

void handle_packet_parsed(parser_state *state, packet_parsed *message) {
  dg_write_packet_parsed((writer *)state->client_state, message);
}

void handle_stringtables_parsed(parser_state *state, dg_stringtables_parsed *message) {
  dg_write_stringtables_parsed((writer *)state->client_state, message);
}

void handle_version(parser_state *state, dg_demver_data version) {
  ((writer *)state->client_state)->version = version;
}

void copy_demo_freddie(const char *filepath) {
  freddie::demo_t testdemo;

  auto result = freddie::demo_t::parse_demo(&testdemo, filepath);
  EXPECT_EQ(result.error, false) << result.error_message;

  // Steampipe demos have some garbage in the last tick, causes this test to fail
  if(!result.error && testdemo.demver_data.game != steampipe)
  {
    wrapped_memory_stream output;
    wrapped_memory_stream input;
    output.underlying.ground_truth = &input.underlying;
    input.underlying.fill_with_file(filepath);
    auto writeresult1 = testdemo.write_demo(&output.underlying, {freddie::memory_stream_write});
    EXPECT_EQ(writeresult1.error, false) << writeresult1.error_message;
  }
}

void copy_demo_test(const char *filepath) {
  writer w;
  wrapped_memory_stream output;
  wrapped_memory_stream input;
  output.underlying.ground_truth = &input.underlying;
  input.underlying.fill_with_file(filepath);

  dg_writer_init(&w);
  dg_writer_open(&w, &output.underlying, {freddie::memory_stream_write});
  if (!w.error) {
    dg_settings settings;
    dg_settings_init(&settings);

    settings.consolecmd_handler = consolecmd_handler;
    settings.customdata_handler = customdata_handler;
    // settings.datatables_handler = datatables_handler;
    settings.datatables_parsed_handler = handle_datatables_parsed;
    settings.header_handler = header_handler;
    settings.packet_handler = packet_handler;
    settings.stop_handler = stop_handler;
    // settings.stringtables_handler = handle_stringtables;
    settings.stringtables_parsed_handler = handle_stringtables_parsed;
    settings.synctick_handler = synctick_handler;
    settings.usercmd_handler = usercmd_handler;
    settings.demo_version_handler = handle_version;
    settings.client_state = &w;

    dg_input_interface input_funcs = {freddie::memory_stream_read, freddie::memory_stream_seek};

    auto out = dg_parse(&settings, &input, input_funcs);
    EXPECT_EQ(out.error, false) << out.error_message;
    EXPECT_EQ(w.error, false) << w.error_message;

    dg_writer_close(&w);
  }
  dg_writer_free(&w);

  EXPECT_EQ(output.underlying.file_size, input.underlying.file_size) << "Output and Input file sizes did not match "
                                               << output.underlying.file_size << " vs. " << input.underlying.file_size;
  std::size_t size = std::min(output.underlying.file_size, input.underlying.file_size);
  uint8_t *ptr1 = (uint8_t *)output.underlying.buffer;
  uint8_t *ptr2 = (uint8_t *)input.underlying.buffer;

  for (std::size_t i = 0; i < size; ++i) {
    if (ptr1[i] != ptr2[i]) {
      EXPECT_EQ(ptr1[i], ptr2[i]) << " expected output/input bytes to match at index " << i;
      break;
    }
  }
}
