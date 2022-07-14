#include "demogobbler.h"
#include "copy.hpp"
#include "gtest/gtest.h"

static int packet_tick = 0;


static void header_handler(demogobbler_header *header) {}
static void consolecmd_handler(demogobbler_consolecmd *message) {}
static void customdata_handler(demogobbler_customdata *message) {}
static void datatables_handler(demogobbler_datatables *message) {}
static void stringtables_handler(demogobbler_stringtables *message) {}
static void stop_handler(demogobbler_stop *message) {}
static void synctick_handler(demogobbler_synctick *message) {}
static void usercmd_handler(demogobbler_usercmd *message) {}

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

  settings.consolecmd_handler = consolecmd_handler;
  settings.customdata_handler = customdata_handler;
  settings.datatables_handler = datatables_handler;
  settings.header_handler = header_handler;
  settings.packet_handler = packet_handler;
  settings.stop_handler = stop_handler;
  settings.stringtables_handler = stringtables_handler;
  settings.synctick_handler = synctick_handler;
  settings.usercmd_handler = usercmd_handler;

  demogobbler_parser_init(&parser, &settings);
  demogobbler_parser_parse_file(&parser, "./test_demos/realcreative-14.dem");
  demogobbler_parser_free(&parser);
  EXPECT_EQ(packet_tick, 1061);
}

TEST(E2E, realcreative_14_copy) {
  copy_demo_test("./test_demos/realcreative-14.dem");
}
