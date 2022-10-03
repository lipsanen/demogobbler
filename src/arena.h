#pragma once

#include "demogobbler_arena.h"

arena demogobbler_arena_create(uint32_t first_block_size);
void demogobbler_arena_clear(arena* a);
void* demogobbler_arena_allocate(arena* a, uint32_t size, uint32_t alignment);
void* demogobbler_arena_reallocate(arena* a, void* ptr, uint32_t prev_size, uint32_t size, uint32_t alignment);
void demogobbler_arena_free(arena* a);
// Add memory that is currently in use to the arena pool
void demogobbler_arena_attach(arena* a, void* ptr, uint32_t size);
