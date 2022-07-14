#include "benchmark/benchmark.h"
#include "demogobbler.h"
#include <iostream>

static void header_handler(demogobbler_header *header) {}
static void consolecmd_handler(demogobbler_consolecmd *message) {}
static void customdata_handler(demogobbler_customdata *message) {}
static void datatables_handler(demogobbler_datatables *message) {}
static void packet_handler(demogobbler_packet *packet) {}
static void stringtables_handler(demogobbler_stringtables *message) {}
static void stop_handler(demogobbler_stop *message) {}
static void synctick_handler(demogobbler_synctick *message) {}
static void usercmd_handler(demogobbler_usercmd *message) {}

static void realcreative_14_setup_and_teardown(benchmark::State &state) {
  // Benchmarks the whole process
  for (auto _ : state) {
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
  }
}

static void realcreative_14_packet_only(benchmark::State &state) {
  // Benchmarks only the parsing portion
  demogobbler_parser parser;
  demogobbler_settings settings;
  demogobbler_settings_init(&settings);

  settings.packet_handler = packet_handler;

  demogobbler_parser_init(&parser, &settings);

  for (auto _ : state) {
    demogobbler_parser_parse_file(&parser, "./test_demos/realcreative-14.dem");
  }

  demogobbler_parser_free(&parser);
}

static void realcreative_14_parse_only(benchmark::State &state) {
  // Benchmarks only the parsing portion
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

  for (auto _ : state) {
    demogobbler_parser_parse_file(&parser, "./test_demos/realcreative-14.dem");
  }

  demogobbler_parser_free(&parser);
}

static void realcreative_14_header_only(benchmark::State &state) {
  // Only register header handler, opportunity to optimize the main loop out
  demogobbler_parser parser;
  demogobbler_settings settings;
  demogobbler_settings_init(&settings);
  settings.header_handler = header_handler;
  demogobbler_parser_init(&parser, &settings);
  for (auto _ : state) {
    demogobbler_parser_parse_file(&parser, "./test_demos/realcreative-14.dem");
  }

  demogobbler_parser_free(&parser);
}

BENCHMARK(realcreative_14_setup_and_teardown);
BENCHMARK(realcreative_14_parse_only);
BENCHMARK(realcreative_14_packet_only);
BENCHMARK(realcreative_14_header_only);