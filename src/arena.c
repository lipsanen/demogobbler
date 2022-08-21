#include "demogobbler_arena.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

arena demogobbler_arena_create(uint32_t first_block_size) {
  arena out;
  memset(&out, 0, sizeof(arena));
  out.first_block_size = first_block_size;

  return out;
}

static size_t alignment_loss(struct demogobbler_arena_block* block, uint32_t alignment) {
  size_t offset = block->bytes_allocated & (alignment - 1);

  if(offset == 0) {
    return 0;
  }
  else {
    return alignment - offset;
  }
}

static size_t block_bytes_left(struct demogobbler_arena_block* block, uint32_t size, uint32_t alignment) {
  if(block == NULL) {
    return 0;
  }
  else {
    size_t inc = alignment_loss(block, alignment);
    return block->total_bytes - (block->bytes_allocated + inc);
  }
}

static struct demogobbler_arena_block* get_last_block(arena* a) {
  if(a->block_count == 0) {
    return NULL;
  }
  else {
    return &a->blocks[a->block_count - 1];
  }
}

static void allocate_new_block(arena* a, uint32_t requested_size) {
  size_t allocated_size;
  if(a->block_count == 0) {
    allocated_size = a->first_block_size;
  }
  else {
    struct demogobbler_arena_block* ptr = get_last_block(a);
    allocated_size = ptr->total_bytes * 2; // Double the size for the next block
  }

  // This shouldnt usually happen since allocations are supposed to be a fraction of the total size of the block
  // But make sure that we have enough space for the next allocation
  while(allocated_size < requested_size) {
    allocated_size *= 2;
  }

  // God help you if you allocate this much memory
  if(allocated_size > UINT32_MAX) {
    allocated_size = UINT32_MAX;
  }

  ++a->block_count;
  a->blocks = realloc(a->blocks, sizeof(struct demogobbler_arena_block) * a->block_count);
  struct demogobbler_arena_block* ptr = get_last_block(a);
  ptr->data = malloc(allocated_size);
  ptr->bytes_allocated = 0;
  ptr->total_bytes = allocated_size;
}

static void* allocate(struct demogobbler_arena_block* block, uint32_t size, uint32_t alignment) {
  size_t increment = alignment_loss(block, alignment);
  size_t allocation_size = size + increment; // size_t here in case overflow

  if(block->data == NULL || allocation_size > block->total_bytes - block->bytes_allocated) { 
    return NULL;
  }
  else {
    void* out = (uint8_t*)block->data + block->bytes_allocated + increment;
    block->bytes_allocated += size + increment;
    return out;
  }
}

void* demogobbler_arena_allocate(arena* a, uint32_t size, uint32_t alignment) {
  struct demogobbler_arena_block* ptr = get_last_block(a);
  size_t bytes_left = block_bytes_left(ptr, size, alignment);
  if(bytes_left < size) {
    allocate_new_block(a, size);
    ptr = get_last_block(a);
  }

  return allocate(ptr, size, alignment);
}

void demogobbler_arena_free(arena* a) {
  for(size_t i=0; i < a->block_count; ++i) {
    free(a->blocks[i].data);
  }
  free(a->blocks);
  memset(a, 0, sizeof(arena));
}
