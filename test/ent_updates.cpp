#include "demogobbler.h"
#include "gtest/gtest.h"
#include "test_demos.hpp"

extern "C" {
  #include "parser.h"
  #include "parser_packetentities.h"
}

void handle_packet_net_message(parser_state *state, packet_net_message *message) {
  parser* thisptr = (parser*)((size_t)(state) - offsetof(parser, state));
  if(message->mtype == svc_packet_entities) {
    demogobbler_parse_packetentities(thisptr, &message->message_svc_packet_entities);

    EXPECT_EQ(thisptr->error, false) << thisptr->error_message;
  }
}


TEST(E2E, test_ents_reading) {

  demogobbler_settings settings;
  demogobbler_settings_init(&settings);
  settings.packet_net_message_handler = handle_packet_net_message;
  settings.store_ents = true;

  for(auto& demo : get_test_demos())
  {
    std::cout << "[----------] " << demo << std::endl;
    auto output = demogobbler_parse_file(&settings, demo.c_str());
    ASSERT_EQ(output.error, false);
  }
}