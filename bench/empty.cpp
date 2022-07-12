#include "benchmark/benchmark.h"
#include "demogobbler.h"

static void null_pointer(benchmark::State& state)
{
    demogobbler_settings settings;
    demogobbler_settings_init(&settings);
    demogobbler_parser parser;
    demogobbler_parser_init(&parser, &settings);

    for(auto _ : state)
    {
        demogobbler_parser_parse(&parser, NULL);
    }
}

BENCHMARK(null_pointer);