#pragma once

// This memory arena provides storage for stuff that has the same lifetime
// Memory is freed all at once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

struct demogobbler_arena_block {
  uint32_t bytes_used; // How many bytes have been allocated
  uint32_t total_bytes; // How many bytes are in this arena (does not count the preamble)
  void* data; // Pointer to data
};

// The arena lazily allocates memory
typedef struct {
  struct demogobbler_arena_block* blocks;
  size_t block_count;
  size_t first_block_size;
  size_t current_block;
} arena;


#ifdef __cplusplus
}
#endif
