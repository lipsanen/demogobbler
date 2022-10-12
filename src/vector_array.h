#pragma once

#include "alignof_wrapper.h"
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  void *ptr;                  // Pointer to array
  uint32_t ptr_bytes;         // How many bytes allocated
  uint32_t count_elements;    // How many elements currently in the vector
  uint32_t bytes_per_element; // How many bytes each element takes up
  bool allocated_by_malloc;   // If on the stack, we dont want to free ptr
} dg_vector_array;

dg_vector_array dg_va_create_(void *ptr, uint32_t ptr_bytes, uint32_t bytes_per_element,
                           uint32_t alignment);
void *dg_va_push_back_empty(dg_vector_array *thisptr);          // Add new element without copying
bool dg_va_push_back(dg_vector_array *thisptr, void *src);      // Add new element
void dg_va_clear(dg_vector_array *thisptr);                     // Clear but dont deallocate
void dg_va_free(dg_vector_array *thisptr);                      // Frees the memory used
void *dg_va_indexptr(dg_vector_array *thisptr, uint32_t index); // Get ptr to index

// Use for initializing with an array on the stack
#define dg_va_create(array, type) dg_va_create_(array, sizeof(array), sizeof(*array), alignof(type))
