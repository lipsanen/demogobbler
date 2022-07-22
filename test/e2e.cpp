#include "demogobbler.h"
#include "copy.hpp"
#include "gtest/gtest.h"
#include "test_demos.hpp"

TEST(E2E, copy_demos) {
  for(auto& demo : get_test_demos())
  {
    std::cout << "[----------] " << demo << std::endl;
    copy_demo_test(demo.c_str());
  }
}

static void packet_handler(void*, demogobbler_packet*) {
}

static void packet_net_message_handler(void* client_state, packet_net_message* message) {
}

TEST(E2E, buffer_stream_test) {
  
  auto demos = get_test_demos();

  if(demos.empty())
  {
    GTEST_SKIP();
  }

  auto demo = demos[0];
  std::cout << "[----------] " << demo << std::endl;
  FILE* stream = fopen(demo.c_str(), "rb");
  fseek(stream, 0, SEEK_END);
  std::size_t length = ftell(stream);

  void* buffer = malloc(length);

  fseek(stream, 0, SEEK_SET);
  fread(buffer, 1, length, stream);
  fclose(stream);

  demogobbler_parser parser;
  demogobbler_settings settings;
  demogobbler_settings_init(&settings);

  settings.packet_handler = packet_handler;
  settings.packet_net_message_handler = packet_net_message_handler;

  demogobbler_parser_init(&parser, &settings);
  demogobbler_parser_parse_buffer(&parser, buffer, length);
  demogobbler_parser_free(&parser);

  EXPECT_EQ(parser.error, false) << parser.error_message;

  free(buffer);
}
