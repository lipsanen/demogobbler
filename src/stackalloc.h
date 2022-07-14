#pragma once

#include "stddef.h"
#include "stdint.h"

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
void stackallocator_init(allocator* thisptr, size_t bytes);
void stackallocator_dealloc(allocator* thisptr, blk block);
void stackallocator_dealloc_all(allocator* thisptr);
void stackallocator_free(allocator* thisptr);


#define allocator_alloc stackallocator_alloc
#define allocator_init stackallocator_init
#define allocator_dealloc stackallocator_dealloc
#define allocator_dealloc_all stackallocator_dealloc_all
#define allocator_free stackallocator_free
