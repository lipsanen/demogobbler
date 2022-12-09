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


dg_arena dg_arena_create(uint32_t first_block_size);
void dg_arena_clear(dg_arena *a);
void *dg_arena_allocate(dg_arena *a, uint32_t size, uint32_t alignment);
void *dg_arena_reallocate(dg_arena *a, void *ptr, uint32_t prev_size, uint32_t size,
                          uint32_t alignment);
void dg_arena_free(dg_arena *a);
// Add memory that is currently in use to the arena pool
void dg_arena_attach(dg_arena *a, void *ptr, uint32_t size);

#ifdef __cplusplus
}
#endif
