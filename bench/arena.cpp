#include "benchmark/benchmark.h"
#include "demogobbler_arena.h"
#include <cstring>
#include <cstdint>

using test_type = std::uint32_t;

static void bench_pointers_arena(benchmark::State& state) {
  for(auto _ : state) {
    arena a = demogobbler_arena_create(4096);
    const int COUNT = 100000;
    test_type* pointers[COUNT];

    for(int i=0; i < COUNT; ++i) {
      pointers[i] = (test_type*)demogobbler_arena_allocate(&a, sizeof(test_type), alignof(test_type));
      std::memset(pointers[i], i, sizeof(test_type));
      benchmark::DoNotOptimize(*pointers[i]);
    }

    demogobbler_arena_free(&a);
  }
}


static void bench_locality_arena(benchmark::State& state) {
  arena a = demogobbler_arena_create(4096);
  const int COUNT = 100000;
  test_type* pointers[COUNT];

  for(int i=0; i < COUNT; ++i) {
    pointers[i] = (test_type*)demogobbler_arena_allocate(&a, sizeof(test_type), alignof(test_type));
  }

  for(auto _ : state) {
    for(int i=0; i < COUNT; ++i) {
      std::memset(pointers[i], i, sizeof(test_type));
      benchmark::DoNotOptimize(*pointers[i]);
    }
  }

  demogobbler_arena_free(&a);
}

static void bench_locality_malloc(benchmark::State& state) {
  const int COUNT = 100000;
  test_type* pointers[COUNT];

  for(int i=0; i < COUNT; ++i) {
    pointers[i] = (test_type*)malloc(sizeof(test_type));
  }

  for(auto _ : state) {

    for(int i=0; i < COUNT; ++i) {
      std::memset(pointers[i], i, sizeof(test_type));
      benchmark::DoNotOptimize(*pointers[i]);
    }

  }

  for(int i=0; i < COUNT; ++i) {
    free(pointers[i]);
  }
}

static void bench_pointers_malloc(benchmark::State& state) {
  for(auto _ : state) {
    const int COUNT = 100000;
    test_type* pointers[COUNT];

    for(int i=0; i < COUNT; ++i) {
      pointers[i] = (test_type*)malloc(sizeof(test_type));
      std::memset(pointers[i], i, sizeof(test_type));
      benchmark::DoNotOptimize(*pointers[i]);
    }

    for(int i=0; i < COUNT; ++i) {
      free(pointers[i]);
    }
  }
}

BENCHMARK(bench_locality_arena);
BENCHMARK(bench_pointers_arena);
BENCHMARK(bench_locality_malloc);
BENCHMARK(bench_pointers_malloc);
