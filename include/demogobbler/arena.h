#pragma once

// This memory arena provides storage for stuff that has the same lifetime
// Memory is freed all at once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

struct dg_arena_block {
  uint32_t bytes_used;  // How many bytes have been allocated
  uint32_t total_bytes; // How many bytes are in this arena (does not count the preamble)
  void *data;           // Pointer to data
};

// The arena lazily allocates memory
typedef struct {
  struct dg_arena_block *blocks;
  uint32_t block_count;
  uint32_t first_block_size;
  uint32_t current_block;
} dg_arena;

#ifdef __cplusplus
}
#endif
