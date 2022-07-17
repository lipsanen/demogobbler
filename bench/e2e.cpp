#include "benchmark/benchmark.h"
#include "demogobbler.h"
#include "test_demos.hpp"
#include <iostream>
#include <filesystem>

static void header_handler(void*, demogobbler_header *header) {}
static void consolecmd_handler(void*, demogobbler_consolecmd *message) {}
static void customdata_handler(void*, demogobbler_customdata *message) {}
static void datatables_handler(void*, demogobbler_datatables *message) {}
static void packet_handler(void*, demogobbler_packet *packet) {}
static void stringtables_handler(void*, demogobbler_stringtables *message) {}
static void stop_handler(void*, demogobbler_stop *message) {}
static void synctick_handler(void*, demogobbler_synctick *message) {}
static void usercmd_handler(void*, demogobbler_usercmd *message) {}

static void test_demos_setup_and_teardown(benchmark::State &state) {
  auto demos = get_test_demos();

  // Benchmarks the whole process
  for (auto _ : state) {
    for(auto& demo : demos)
    {
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
      demogobbler_parser_parse_file(&parser, demo.c_str());
      demogobbler_parser_free(&parser);
    }
  }

  state.SetItemsProcessed(state.iterations() * demos.size());
}

static void testdemos_packet_only(benchmark::State &state) {
  // Benchmarks only the parsing portion
  demogobbler_parser parser;
  demogobbler_settings settings;
  demogobbler_settings_init(&settings);

  settings.packet_handler = packet_handler;

  demogobbler_parser_init(&parser, &settings);
  auto demos = get_test_demos();

  for (auto _ : state) {
    for(auto& demo : demos)
    {
      demogobbler_parser_parse_file(&parser, demo.c_str());
    }
  }

  state.SetItemsProcessed(state.iterations() * demos.size());

  demogobbler_parser_free(&parser);
}

static void parse_only(demogobbler_parser* parser, const std::vector<std::string>& demos)
{
  for(auto& file : demos)
  {
    demogobbler_parser_parse_file(parser, file.c_str());
  }
}

static void testdemos_parse_only(benchmark::State &state) {
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
  auto demos = get_test_demos();

  for (auto _ : state) {
    parse_only(&parser, demos);
  }

  state.SetItemsProcessed(state.iterations() * demos.size());

  demogobbler_parser_free(&parser);
}

static void testdemos_header_only(benchmark::State &state) {
  // Only register header handler, opportunity to optimize the main loop out
  demogobbler_parser parser;
  demogobbler_settings settings;
  demogobbler_settings_init(&settings);
  settings.header_handler = header_handler;
  demogobbler_parser_init(&parser, &settings);
  auto demos = get_test_demos();

  for (auto _ : state) {
    for(auto& demo : demos)
    {
      demogobbler_parser_parse_file(&parser, demo.c_str());
    }
  }

  state.SetItemsProcessed(state.iterations() * demos.size());

  demogobbler_parser_free(&parser);
}

BENCHMARK(test_demos_setup_and_teardown);
BENCHMARK(testdemos_parse_only);
BENCHMARK(testdemos_packet_only);
BENCHMARK(testdemos_header_only);