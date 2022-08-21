#pragma once

// This memory arena provides storage for stuff that has the same lifetime
// Memory is freed all at once

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

struct demogobbler_arena_block {
  uint32_t bytes_allocated; // How many bytes have been allocated
  uint32_t total_bytes; // How many bytes are in this arena (does not count the preamble)
  void* data; // Pointer to data
};

// The arena always allocates mem
struct demogobbler_arena {
  struct demogobbler_arena_block* blocks;
  size_t block_count;
  size_t first_block_size;
};

typedef struct demogobbler_arena arena;

arena demogobbler_arena_create(uint32_t first_block_size);
void* demogobbler_arena_allocate(arena* a, uint32_t size, uint32_t alignment);
void demogobbler_arena_free(arena* a);

#ifdef __cplusplus
}
#endif
