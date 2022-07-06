#include "benchmark/benchmark.h"
#include "demogoblin.h"

static void null_pointer(benchmark::State& state)
{
    demogoblin_settings settings;
    for(auto _ : state)
    {
        demogoblin_parse(nullptr, settings);
    }
}

BENCHMARK(null_pointer);