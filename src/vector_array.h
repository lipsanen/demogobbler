#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  void *ptr;                  // Pointer to array
  uint32_t ptr_bytes;         // How many bytes allocated
  uint32_t count_elements;    // How many elements currently in the vector
  uint32_t bytes_per_element; // How many bytes each element takes up
  bool allocated_by_malloc;   // If on the stack, we dont want to free ptr
} vector_array;

vector_array demogobbler_va_create_(void *ptr, uint32_t ptr_bytes, uint32_t bytes_per_element,
                                    uint32_t alignment);
void* demogobbler_va_push_back_empty(vector_array *thisptr); // Add new element without copying
bool demogobbler_va_push_back(vector_array *thisptr, void *src); // Add new element
void demogobbler_va_clear(vector_array *thisptr);                // Clear but dont deallocate
void demogobbler_va_free(vector_array *thisptr);                 // Frees the memory used
void* demogobbler_va_indexptr(vector_array* thisptr, uint32_t index); // Get ptr to index

// Use for initializing with an array on the stack
#define demogobbler_va_create(array)                                                               \
  demogobbler_va_create_(array, sizeof(array), sizeof(*array), alignof(*array))
