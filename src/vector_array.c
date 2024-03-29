#include "demogobbler/vector_array.h"
#include "demogobbler/utils.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

static size_t find_smallest_power_of_two_that_is_bigger(size_t value) {
  size_t rval = 1;
  while (value > rval)
    rval <<= 1;
  return rval;
}

dg_vector_array dg_va_create_(void *ptr, uint32_t ptr_bytes, uint32_t bytes_per_element,
                           uint32_t alignment) {
  dg_vector_array out;
  memset(&out, 0, sizeof(out));
  // We align the user provided memory if required, otherwise alignment information is not stored
  // malloc should return sufficiently aligned memory for any type
  size_t loss = alignment_loss(ptr_bytes, alignment);
  ptr_bytes -= loss;

  if (ptr_bytes > 0) {
    out.allocated_by_malloc = false;
    out.ptr = (uint8_t *)ptr + loss;
    out.ptr_bytes = ptr_bytes;
  }

  out.bytes_per_element = bytes_per_element;
  return out;
}

void *dg_va_push_back_empty(dg_vector_array *thisptr) {
  uint32_t new_size = (thisptr->count_elements + 1) * thisptr->bytes_per_element;
  if (new_size > thisptr->ptr_bytes) {
    uint32_t new_allocation_size = find_smallest_power_of_two_that_is_bigger(new_size);
    new_allocation_size = MAX(new_allocation_size, 64); // Allocate at least 64 bytes
    if (!thisptr->allocated_by_malloc) {
      void *new_array = malloc(new_allocation_size);

      if (!new_array) {
        return NULL;
      }
      thisptr->ptr_bytes = new_allocation_size;
      // Null pointer may be passed to create
      if (thisptr->ptr) {
        memcpy(new_array, thisptr->ptr,
               thisptr->count_elements * thisptr->bytes_per_element); // Copy the bytes in use
      }
      thisptr->ptr = new_array;
      thisptr->allocated_by_malloc = true;
    } else {
      void *new_array = realloc(thisptr->ptr, new_allocation_size);
      if (!new_array) {
        return NULL;
      }
      thisptr->ptr_bytes = new_allocation_size;
      thisptr->ptr = new_array;
    }
  }
  thisptr->count_elements = thisptr->count_elements + 1;
  return dg_va_indexptr(thisptr, thisptr->count_elements - 1);
}

bool dg_va_push_back(dg_vector_array *thisptr, void *src) {
  void *index_ptr = dg_va_push_back_empty(thisptr);
  memcpy(index_ptr, src, thisptr->bytes_per_element);

  return true;
}

void dg_va_clear(dg_vector_array *thisptr) { thisptr->count_elements = 0; }

void dg_va_free(dg_vector_array *thisptr) {
  if (thisptr->allocated_by_malloc) {
    free(thisptr->ptr);
    thisptr->ptr = NULL;
  }
}

void *dg_va_indexptr(dg_vector_array *thisptr, uint32_t index) {
  return (uint8_t *)thisptr->ptr + thisptr->bytes_per_element * index;
}
