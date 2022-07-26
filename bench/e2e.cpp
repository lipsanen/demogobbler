#include "benchmark/benchmark.h"
#include "demogobbler.h"
#include "test_demos.hpp"
#include <iostream>
#include <filesystem>

static void header_handler(parser_state*, demogobbler_header *header) {}
static void consolecmd_handler(parser_state*, demogobbler_consolecmd *message) {}
static void customdata_handler(parser_state*, demogobbler_customdata *message) {}
static void datatables_handler(parser_state*, demogobbler_datatables *message) {}
static void packet_handler(parser_state*, demogobbler_packet *packet) {}
static void stringtables_handler(parser_state*, demogobbler_stringtables *message) {}
static void stop_handler(parser_state*, demogobbler_stop *message) {}
static void synctick_handler(parser_state*, demogobbler_synctick *message) {}
static void usercmd_handler(parser_state*, demogobbler_usercmd *message) {}

static void testdemos_packet_only(benchmark::State &state) {
  // Benchmarks only the parsing portion
  demogobbler_settings settings;
  demogobbler_settings_init(&settings);

  settings.packet_handler = packet_handler;

  auto demos = get_test_demos();

  for (auto _ : state) {
    for(auto& demo : demos)
    {
      demogobbler_parse_file(&settings, demo.c_str());
    }
  }

  state.SetItemsProcessed(state.iterations() * demos.size());
}

static void packet_netmessage_handler(parser_state*, demogobbler_packet_net_message* message) {}

static void testdemos_netmessages(benchmark::State &state) {
  // Benchmarks only the parsing portion
  demogobbler_settings settings;
  demogobbler_settings_init(&settings);

  settings.packet_net_message_handler = packet_netmessage_handler;
  auto demos = get_test_demos();

  for (auto _ : state) {
    for(auto& demo : demos)
    {
      demogobbler_parse_file(&settings, demo.c_str());
    }
  }

  state.SetItemsProcessed(state.iterations() * demos.size());
}

static void parse_only(demogobbler_settings* settings, const std::vector<std::string>& demos)
{
  for(auto& file : demos)
  {
    demogobbler_parse_file(settings, file.c_str());
  }
}

static void testdemos_parse_only(benchmark::State &state) {
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

  auto demos = get_test_demos();

  for (auto _ : state) {
    parse_only(&settings, demos);
  }

  state.SetItemsProcessed(state.iterations() * demos.size());

}

static void testdemos_header_only(benchmark::State &state) {
  // Only register header handler, opportunity to optimize the main loop out
  demogobbler_settings settings;
  demogobbler_settings_init(&settings);
  settings.header_handler = header_handler;
  auto demos = get_test_demos();

  for (auto _ : state) {
    for(auto& demo : demos)
    {
      demogobbler_parse_file(&settings, demo.c_str());
    }
  }

  state.SetItemsProcessed(state.iterations() * demos.size());
}

BENCHMARK(testdemos_parse_only);
BENCHMARK(testdemos_packet_only);
BENCHMARK(testdemos_header_only);
BENCHMARK(testdemos_netmessages);