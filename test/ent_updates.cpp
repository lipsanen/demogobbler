#include "demogobbler.h"
#include "demogobbler/bitwriter.h"
#include "utils/test_demos.hpp"
#include "gtest/gtest.h"

extern "C" {
#include "demogobbler/parser.h"
}

void handle_packetentities(parser_state *state, dg_svc_packetentities_parsed *parsed) {
  dg_parser *thisptr = (dg_parser *)((size_t)(state)-offsetof(dg_parser, state)); // dont try this at home
  dg_bitwriter writer;
  uint32_t bits = parsed->orig->data.bitsize - parsed->orig->data.bitoffset;
  dg_bitwriter_init(&writer, 1024);
  write_packetentities_args args;
  args.data = &parsed->data;
  args.is_delta = parsed->orig->is_delta;
  args.version = &thisptr->demo_version;
#ifdef GROUND_TRUTH_CHECK
  writer.truth_data = parsed->orig->data.data;
  writer.truth_data_offset = parsed->orig->data.bitoffset;
  writer.truth_size_bits = bits;
#endif

  dg_bitwriter_write_packetentities(&writer, args);
  if (writer.error || writer.bitoffset != bits) {
    thisptr->error = true;
    thisptr->error_message = writer.error_message;
  }
  dg_bitwriter_free(&writer);
}

TEST(E2E, test_ents_reading) {

  dg_settings settings;
  dg_settings_init(&settings);
  settings.packetentities_parsed_handler = handle_packetentities;

  for (auto &demo : get_test_demos()) {
    std::cout << "[----------] " << demo << std::endl;
    auto output = dg_parse_file(&settings, demo.c_str());
    EXPECT_EQ(output.error, false) << output.error_message;
  }
}