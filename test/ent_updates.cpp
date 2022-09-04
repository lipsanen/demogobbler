#include "demogobbler.h"
#include "demogobbler_bitwriter.h"
#include "gtest/gtest.h"
#include "test_demos.hpp"

extern "C" {
  #include "parser.h"
  #include "parser_packetentities.h"
}

void handle_packetentities(parser_state *state, svc_packetentities_parsed* parsed) {
  parser* thisptr = (parser*)((size_t)(state) - offsetof(parser, state)); // dont try this at home
  bitwriter writer;
  bitwriter_init(&writer, parsed->orig->data_length);
  write_packetentities_args args;
  args.data = parsed->data;
  args.entity_state = &state->entity_state;
  args.is_delta = parsed->orig->is_delta;
  args.version = &thisptr->demo_version;
#ifdef GROUND_TRUTH_CHECK
  writer.truth_data = parsed->orig->data.data;
  writer.truth_data_offset = parsed->orig->data.bitoffset;
  writer.truth_size_bits = parsed->orig->data_length;
#endif

  demogobbler_bitwriter_write_packetentities(&writer, args);
  if(writer.error || writer.bitoffset != parsed->orig->data_length) {
    thisptr->error = true;
    thisptr->error_message = writer.error_message;
  }
  bitwriter_free(&writer);
}


TEST(E2E, test_ents_reading) {

  demogobbler_settings settings;
  demogobbler_settings_init(&settings);
  settings.packetentities_parsed_handler = handle_packetentities;

  for(auto& demo : get_test_demos())
  {
    std::cout << "[----------] " << demo << std::endl;
    auto output = demogobbler_parse_file(&settings, demo.c_str());
    EXPECT_EQ(output.error, false) << output.error_message;
  }
}