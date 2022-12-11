#include "demogobbler/allocator.h"
#include "utils.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Turn this macro on for more readable profiler output
#define FUN_ATTRIBUTE //__attribute__((noinline))

dg_arena FUN_ATTRIBUTE dg_arena_create(uint32_t first_block_size) {
  dg_arena out;
  memset(&out, 0, sizeof(dg_arena));
  out.first_block_size = first_block_size;

  return out;
}

void FUN_ATTRIBUTE dg_arena_clear(dg_arena *a) {
  for (size_t i = 0; i < a->block_count; ++i) {
    a->blocks[i].bytes_used = 0;
  }
  a->current_block = 0;
}

static size_t FUN_ATTRIBUTE block_bytes_left(struct dg_arena_block block, uint32_t size,
                                             uint32_t alignment) {
  size_t inc = alignment_loss(block.bytes_used, alignment);
  return block.total_bytes - (block.bytes_used + inc);
}

static struct dg_arena_block *FUN_ATTRIBUTE get_last_block(dg_arena *a) {
  if (a->block_count == 0) {
    return NULL;
  } else {
    return &a->blocks[a->block_count - 1];
  }
}

static void FUN_ATTRIBUTE allocate_new_block(dg_arena *a, uint32_t requested_size) {
  size_t allocated_size;
  if (a->block_count == 0) {
    allocated_size = a->first_block_size;
  } else {
    struct dg_arena_block *ptr = get_last_block(a);
    allocated_size = ptr->total_bytes; // Double the size for the next block
  }

  allocated_size = MAX(1, allocated_size); // Allocate at least 1 byte

  // This shouldnt usually happen since allocations are supposed to be a fraction of the total size
  // of the block But make sure that we have enough space for the next allocation
  while (allocated_size < requested_size) {
    allocated_size *= 2;
  }

  // God help you if you allocate this much memory
  if (allocated_size > UINT32_MAX) {
    allocated_size = UINT32_MAX;
  }

  ++a->block_count;
  a->blocks = realloc(a->blocks, sizeof(struct dg_arena_block) * a->block_count);
  struct dg_arena_block *ptr = get_last_block(a);
  ptr->data = malloc(allocated_size);
  ptr->bytes_used = 0;
  ptr->total_bytes = allocated_size;
}

static struct dg_arena_block *FUN_ATTRIBUTE find_block_with_memory(dg_arena *a,
                                                                   uint32_t requested_size,
                                                                   uint32_t alignment) {
  if (a->block_count == 0) {
    allocate_new_block(a, requested_size);
  }

  while (block_bytes_left(a->blocks[a->current_block], requested_size, alignment) <
         requested_size) {
    ++a->current_block;
    if (a->current_block >= a->block_count) {
      allocate_new_block(a, requested_size);
      break;
    }
  }

  return &a->blocks[a->current_block];
}

static void *FUN_ATTRIBUTE allocate(struct dg_arena_block *block, uint32_t size,
                                    uint32_t alignment) {
  size_t increment = alignment_loss(block->bytes_used, alignment);
  void *out = (uint8_t *)block->data + block->bytes_used + increment;
  block->bytes_used += size + increment;
  return out;
}

void *FUN_ATTRIBUTE dg_arena_allocate(dg_arena *a, uint32_t size, uint32_t alignment) {
  if (size == 0) {
    return NULL;
  }

  struct dg_arena_block *ptr = find_block_with_memory(a, size, alignment);
  return allocate(ptr, size, alignment);
}

void *dg_arena_reallocate(dg_arena *a, void *_ptr, uint32_t prev_size, uint32_t size,
                          uint32_t alignment) {
  if (_ptr == NULL || a->blocks == NULL) {
    return dg_arena_allocate(a, size, alignment);
  }

  struct dg_arena_block *blk_ptr = &a->blocks[a->current_block];
  uint8_t *previous = (uint8_t *)blk_ptr->data + blk_ptr->bytes_used - prev_size;
  uint8_t *ptr = _ptr;
  uint32_t bytes_left_in_block = blk_ptr->total_bytes - blk_ptr->bytes_used + prev_size;

  if (previous == ptr && bytes_left_in_block >= size) {
    blk_ptr->bytes_used -= prev_size;
    void *new_arr = dg_arena_allocate(a, size, alignment);
    assert(new_arr == _ptr); // in-place allocation
    return new_arr;
  } else {
    void *new_arr = dg_arena_allocate(a, size, alignment);
    assert(new_arr != _ptr);
    memcpy(new_arr, _ptr, prev_size); // Not in-place
    return new_arr;
  }
}

void FUN_ATTRIBUTE dg_arena_free(dg_arena *a) {
  for (size_t i = 0; i < a->block_count; ++i) {
    free(a->blocks[i].data);
  }
  free(a->blocks);
  memset(a, 0, sizeof(dg_arena));
}

void dg_arena_attach(dg_arena *a, void *data, uint32_t size) {
  ++a->block_count;
  a->blocks = realloc(a->blocks, sizeof(struct dg_arena_block) * a->block_count);
  struct dg_arena_block *ptr = get_last_block(a);
  ptr->data = data;
  ptr->bytes_used = size;
  ptr->total_bytes = size;
}
