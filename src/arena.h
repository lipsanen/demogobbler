#pragma once

#include "demogobbler/arena.h"

dg_arena dg_arena_create(uint32_t first_block_size);
void dg_arena_clear(dg_arena *a);
void *dg_arena_allocate(dg_arena *a, uint32_t size, uint32_t alignment);
void *dg_arena_reallocate(dg_arena *a, void *ptr, uint32_t prev_size, uint32_t size,
                          uint32_t alignment);
void dg_arena_free(dg_arena *a);
// Add memory that is currently in use to the arena pool
void dg_arena_attach(dg_arena *a, void *ptr, uint32_t size);
