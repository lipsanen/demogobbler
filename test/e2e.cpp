#include "demogobbler.h"
#include "gtest/gtest.h"

static int packet_tick = 0;

static void packet_handler(demogobbler_packet* packet)
{
  if(packet->preamble.tick >= 0)
  {
    packet_tick = packet->preamble.tick;
  }
}

TEST(E2E, realcreative_14) {
  demogobbler_parser parser;
  demogobbler_settings settings;
  demogobbler_settings_init(&settings);
  settings.packet_handler = packet_handler;
  demogobbler_parser_init(&parser, &settings);
  demogobbler_parser_parse(&parser, "./test_demos/realcreative-14.dem");
  demogobbler_parser_free(&parser);
  EXPECT_EQ(packet_tick, 1061);
}
