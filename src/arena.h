#pragma once

#include "demogobbler_arena.h"

arena demogobbler_arena_create(uint32_t first_block_size);
void demogobbler_arena_clear(arena* a);
void* demogobbler_arena_allocate(arena* a, uint32_t size, uint32_t alignment);
void demogobbler_arena_free(arena* a);
