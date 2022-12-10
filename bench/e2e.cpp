#include "benchmark/benchmark.h"
#include "demogobbler/freddie.hpp"
#include "demogobbler/streams.h"
#include "demogobbler.h"
#include "test_demos.hpp"
#include <filesystem>
#include <iostream>

static void header_handler(parser_state *, dg_header *header) {}
static void consolecmd_handler(parser_state *, dg_consolecmd *message) {}
static void customdata_handler(parser_state *, dg_customdata *message) {}
static void datatables_handler(parser_state *, dg_datatables *message) {}
static void packet_handler(parser_state *, dg_packet *packet) {}
static void stringtables_handler(parser_state *, dg_stringtables *message) {}
static void stop_handler(parser_state *, dg_stop *message) {}
static void synctick_handler(parser_state *, dg_synctick *message) {}
static void usercmd_handler(parser_state *, dg_usercmd *message) {}

static void get_bytes(benchmark::State &state) {
  size_t bytes = 0;

  for (auto &demo : get_test_demos()) {
    bytes += std::filesystem::file_size(demo);
  }

  state.SetBytesProcessed(bytes * state.iterations());
}

static void testdemos_packet_only(benchmark::State &state) {
  // Benchmarks only the parsing portion
  dg_settings settings;
  dg_settings_init(&settings);

  settings.packet_handler = packet_handler;

  auto demos = get_test_demos();

  for (auto _ : state) {
    for (auto &demo : demos) {
      dg_parse_file(&settings, demo.c_str());
    }
  }

  get_bytes(state);
}

static void packet_parsed_handler(parser_state *, packet_parsed *message) {}

static void testdemos_parse_everything(benchmark::State &state) {
  dg_settings settings;
  dg_settings_init(&settings);

  settings.consolecmd_handler = consolecmd_handler;
  settings.customdata_handler = customdata_handler;
  settings.datatables_handler = datatables_handler;
  settings.header_handler = header_handler;
  settings.packet_handler = packet_handler;
  settings.stop_handler = stop_handler;
  settings.stringtables_handler = stringtables_handler;
  settings.synctick_handler = synctick_handler;
  settings.usercmd_handler = usercmd_handler;
  settings.packet_parsed_handler = packet_parsed_handler;
  settings.store_ents = true;
  settings.store_props = true;
  auto demos = get_test_demos();

  for (auto _ : state) {
    for (auto &demo : demos) {
      dg_parse_file(&settings, demo.c_str());
    }
  }

  get_bytes(state);
}

static void parse_only(dg_settings *settings, const std::vector<std::string> &demos) {
  for (auto &file : demos) {
    dg_parse_file(settings, file.c_str());
  }
}

static void testdemos_parse_only(benchmark::State &state) {
  dg_settings settings;
  dg_settings_init(&settings);

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

  get_bytes(state);
}

static void testdemos_header_only(benchmark::State &state) {
  // Only register header handler, opportunity to optimize the main loop out
  dg_settings settings;
  dg_settings_init(&settings);
  settings.header_handler = header_handler;
  auto demos = get_test_demos();

  for (auto _ : state) {
    for (auto &demo : demos) {
      dg_parse_file(&settings, demo.c_str());
    }
  }

  get_bytes(state);
}

static void testdemos_freddie_parse(benchmark::State &state) {
  auto demos = get_test_demos();

  for (auto _ : state) {
    for (auto &file : demos) {
      freddie::demo_t demo;
      freddie::demo_t::parse_demo(&demo, file.c_str());
    }
  }

  get_bytes(state);
}

static void testdemos_freddie_write(benchmark::State &state) {
  auto demos = get_test_demos();
  std::vector<std::shared_ptr<freddie::demo_t>> demo_vec;
  for (auto &file : demos) {
    std::shared_ptr<freddie::demo_t> ptr = std::make_shared<freddie::demo_t>();
    freddie::demo_t::parse_demo(ptr.get(), file.c_str());
    demo_vec.emplace_back(ptr);
  }

  for (auto _ : state) {
    for (auto demo : demo_vec) {
      freddie::memory_stream output;
      auto result = demo->write_demo(&output, {freddie::memory_stream_write});
      if(result.error)
        abort();
    }
  }

  get_bytes(state);
}

static void testdemos_freddie_convert(benchmark::State &state) {
  auto demos = get_test_demos();
  std::vector<std::shared_ptr<freddie::demo_t>> demo_vec;
  for (auto &file : demos) {
    std::shared_ptr<freddie::demo_t> ptr = std::make_shared<freddie::demo_t>();
    freddie::demo_t::parse_demo(ptr.get(), file.c_str());
    demo_vec.emplace_back(ptr);
  }

  for (auto _ : state) {
    for (auto demo : demo_vec) {
      freddie::convert_demo(demo.get(), demo.get());
    }
  }

  get_bytes(state);
}

BENCHMARK(testdemos_parse_only);
BENCHMARK(testdemos_packet_only);
BENCHMARK(testdemos_header_only);
BENCHMARK(testdemos_parse_everything);
BENCHMARK(testdemos_freddie_parse);
BENCHMARK(testdemos_freddie_write);
BENCHMARK(testdemos_freddie_convert);
