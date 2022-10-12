#include "benchmark/benchmark.h"
extern "C" {
#include "arena.h"
}
#include <cstdint>
#include <cstring>

const std::size_t SIZE_ALLOCATED = 1 << 20;
const std::size_t ITERATIONS = 50;
using test_type = std::uint32_t;

static void set_bytes(benchmark::State &state) {
  state.SetBytesProcessed(SIZE_ALLOCATED * ITERATIONS * state.iterations());
}

static void arena_allocation(benchmark::State &state) {
  const std::size_t ALLOCS_PER_IT = SIZE_ALLOCATED / sizeof(test_type);
  const std::size_t BLOCK_SIZE = 1 << 17;

  for (auto _ : state) {
    dg_arena a = dg_arena_create(BLOCK_SIZE);

    for (size_t i = 0; i < ITERATIONS; ++i) {
      dg_arena_clear(&a);
      for (size_t u = 0; u < ALLOCS_PER_IT; ++u) {
        benchmark::DoNotOptimize(dg_arena_allocate(&a, sizeof(test_type), alignof(test_type)));
      }
      benchmark::DoNotOptimize(a);
    }
    dg_arena_free(&a);
  }

  set_bytes(state);
}

BENCHMARK(arena_allocation);
