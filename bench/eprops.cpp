#include "benchmark/benchmark.h"
#include <cstdlib>
extern "C" {
#include "parser_entity_state.h"
}

static void eproplist_insert(benchmark::State &state) {
  bool newprop;
  for (auto _ : state) {
    eproplist list = demogobbler_eproplist_init();
    epropnode *node = nullptr;
    for (uint16_t i = 0; i < 20; ++i) {
      node = demogobbler_eproplist_get(&list, node, i, &newprop);
    }
    demogobbler_eproplist_free(&list);
  }
}

static void eproparr_insert(benchmark::State &state) {
  bool newprop;
  for (auto _ : state) {
    eproparr arr = demogobbler_eproparr_init(400);
    for (uint16_t i = 0; i < 20; ++i) {
      demogobbler_eproparr_get(&arr, i * 10, &newprop);
    }
    demogobbler_eproparr_free(&arr);
  }
}

std::vector<std::vector<uint16_t>> get_demosim() {
  std::vector<std::vector<uint16_t>> updates;
  updates.reserve(1000);
  std::srand(0);

  for (size_t i = 0; i < 1000; ++i) {
    std::vector<uint16_t> update;
    update.reserve(20);
    for (uint16_t i = 0; i < 20; ++i) {
      uint16_t index = i * 10;
      double rng = std::rand() / ((double)RAND_MAX);
      if (rng < 0.8) {
        update.push_back(index);
      }
    }
    updates.push_back(std::move(update));
  }

  return updates;
}

static void eproparr_demosim(benchmark::State &state) {
  bool newprop;
  auto updates = get_demosim();

  for (auto _ : state) {
    eproparr arr = demogobbler_eproparr_init(400);
    for (size_t i = 0; i < updates.size(); ++i) {
      for (size_t u = 0; u < updates[i].size(); ++u) {
        demogobbler_eproparr_get(&arr, updates[i][u], &newprop);
      }
    }
    demogobbler_eproparr_free(&arr);
  }
}

static void eproplist_demosim(benchmark::State &state) {
  bool newprop;
  auto updates = get_demosim();

  for (auto _ : state) {
    eproplist list = demogobbler_eproplist_init();
    for (size_t i = 0; i < updates.size(); ++i) {
      epropnode *node = nullptr;
      for (size_t u = 0; u < updates[i].size(); ++u) {
        uint16_t value = updates[i][u];
        node = demogobbler_eproplist_get(&list, node, value, &newprop);
      }
    }
    demogobbler_eproplist_free(&list);
  }
}

BENCHMARK(eproparr_insert);
BENCHMARK(eproplist_insert);
BENCHMARK(eproparr_demosim);
BENCHMARK(eproplist_demosim);
