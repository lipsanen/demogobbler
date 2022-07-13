#include "benchmark/benchmark.h"
#include "demogobbler.h"
#include <iostream>

static void packet_handler(demogobbler_packet *packet) {}

static void realcreative_14_setup_and_teardown(benchmark::State &state) {
  // Benchmarks the whole process
  for (auto _ : state) {
    demogobbler_parser parser;
    demogobbler_settings settings;
    demogobbler_settings_init(&settings);
    settings.packet_handler =
        packet_handler; // Add empty packet handler to avoid the parser optimizing the main loop out
    demogobbler_parser_init(&parser, &settings);
    demogobbler_parser_parse(&parser, "./test_demos/realcreative-14.dem");
    demogobbler_parser_free(&parser);
  }
}

static void realcreative_14_parse_only(benchmark::State &state) {
  // Benchmarks only the parsing portion
  demogobbler_parser parser;
  demogobbler_settings settings;
  demogobbler_settings_init(&settings);
  settings.packet_handler =
      packet_handler; // Add empty packet handler to avoid the parser optimizing the main loop out
  demogobbler_parser_init(&parser, &settings);
  for (auto _ : state) {
    demogobbler_parser_parse(&parser, "./test_demos/realcreative-14.dem");
  }

  demogobbler_parser_free(&parser);
}

BENCHMARK(realcreative_14_setup_and_teardown);
BENCHMARK(realcreative_14_parse_only);