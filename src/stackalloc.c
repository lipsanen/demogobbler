#include "stackalloc.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

blk stackallocator_alloc(allocator* thisptr, size_t bytes)
{
  blk output;
  // Align the allocation to the next 8 byte boundary
  // We want to guarantee sufficient alignment for any allocation

  if((bytes & 0x7) != 0)
  {
    bytes = bytes + 8 - (bytes & 0x7);
  }

  if(bytes <= (thisptr->size - thisptr->allocated))
  {
    // Allocate from the stack, we have enough bytes
    output.address = (uint8_t*)thisptr->stack + thisptr->allocated;
    output.size = bytes;
    thisptr->allocated += bytes;
  }
  else
  {
    // Fall back on malloc
    output.address = malloc(bytes);
    output.size = bytes;
  }

  return output;
}

void stackallocator_init(allocator* thisptr, void* buffer, size_t size)
{
  thisptr->stack = buffer;
  thisptr->size = size;
  thisptr->allocated = 0;
}

void stackallocator_dealloc(allocator* thisptr, blk block)
{
  uint8_t* ptr = (uint8_t*)block.address;
  uint8_t* stack = (uint8_t*)thisptr->stack;

  // Check that the allocation came from the stack
  if(ptr >= stack && ptr < stack + thisptr->size)
  {
    // Can only free things from the top of the stack
    if(ptr + block.size == stack + thisptr->allocated)
    {
      thisptr->allocated -= block.size;
    }
    else
    {
#ifdef DEBUG
      fprintf(stderr, "Stack allocator is leaking memory, allocations and deallocations are not in the correct order.\n");
      abort();
#endif
    }
  }
  else
  {
    // Allocated by malloc
    free(block.address);
  }
}

void stackallocator_dealloc_all(allocator* thisptr)
{
  thisptr->allocated = 0;
}

void stackallocator_free(allocator* thisptr)
{
  thisptr->allocated = 0;
  thisptr->size = 0;
  free(thisptr->stack);
}

bool stackallocator_space_on_stack(allocator* thisptr, size_t bytes)
{
  if((bytes & 0x7) != 0)
  {
    bytes = bytes + 8 - (bytes & 0x7);
  }

  return bytes <= (thisptr->size - thisptr->allocated);
}

bool stackallocator_onstack(allocator* thisptr, blk block)
{
  uint8_t* ptr = (uint8_t*)block.address;
  uint8_t* stack = (uint8_t*)thisptr->stack;

  return ptr >= stack && ptr <= stack + thisptr->size;
}

blk stackallocator_realloc(allocator* thisptr, blk block, size_t bytes)
{
  bool was_stack = stackallocator_onstack(thisptr, block);

  if((!was_stack && stackallocator_space_on_stack(thisptr, bytes)) ||
  (was_stack && !stackallocator_space_on_stack(thisptr, bytes - block.size)))
  {
    blk new_block = stackallocator_alloc(thisptr, bytes);
    memcpy(new_block.address, block.address, block.size);
    stackallocator_dealloc(thisptr, block);

    return new_block;
  }

  stackallocator_dealloc(thisptr, block);
  return stackallocator_alloc(thisptr, bytes);
}
