#include "stackalloc.h"
#include "stdio.h"
#include "stdlib.h"

blk stackallocator_alloc(allocator* thisptr, size_t bytes)
{
  blk output;
  // Align the allocation to the next 8 byte boundary
  // We want to guarantee sufficient alignment for any allocation

  if((bytes & 0x7) != 0)
  {
    bytes = bytes + 8 - (bytes & 0x7);
  }

  // Stack only allocated when memory requested
  if(!thisptr->stack)
  {
    thisptr->stack = malloc(thisptr->size);
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

void stackallocator_init(allocator* thisptr, size_t size)
{
  thisptr->stack = NULL;
  thisptr->size = size;
  thisptr->allocated = 0;
}

void stackallocator_dealloc(allocator* thisptr, blk block)
{
  uint8_t* ptr = (uint8_t*)block.address;
  uint8_t* stack = (uint8_t*)thisptr->stack;

  // Check that the allocation came from the stack
  if(ptr >= stack && ptr <= stack + thisptr->size)
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
