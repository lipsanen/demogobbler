#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct demogobbler_stackallocator
{
  void* stack;
  size_t allocated;
  size_t size;
};

struct demogobbler_blk
{
  void* address;
  size_t size;
};

typedef struct demogobbler_stackallocator allocator;
typedef struct demogobbler_blk blk;

blk stackallocator_alloc(allocator* thisptr, size_t bytes);
void stackallocator_dealloc(allocator* thisptr, blk block);
void stackallocator_dealloc_all(allocator* thisptr);
void stackallocator_init(allocator* thisptr, void* buffer, size_t bytes);
bool stackallocator_space_on_stack(allocator* thisptr, size_t bytes);
bool stackallocator_onstack(allocator* thisptr, blk block);
blk stackallocator_realloc(allocator* thisptr, blk block, size_t bytes);


#define allocator_alloc stackallocator_alloc
#define allocator_own stackallocator_owns
#define allocator_init stackallocator_init
#define allocator_dealloc stackallocator_dealloc
#define allocator_dealloc_all stackallocator_dealloc_all
#define allocator_realloc stackallocator_realloc
