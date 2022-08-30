#pragma once

#include <stdint.h>
#include <stdlib.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define CLAMP(min, value, max) MIN(MAX(min, value), max)

#define COORD_INTEGER_BITS 14
#define COORD_FRACTIONAL_BITS 5
#define COORD_DENOMINATOR (1 << (COORD_FRACTIONAL_BITS))
#define COORD_RESOLUTION 1.0f / (1 << COORD_FRACTIONAL_BITS)

enum { COORD_INT_BITS_MP = 11 };
enum { FRAC_BITS_LP = 3 };
enum { FRAC_BITS = 5 };
enum { MAX_EDICTS = 2048 };
enum { MAX_EDICT_BITS = 11 };
enum { ENTITY_SENTINEL = 9999 };

#define ARRAYSIZE(a) ((sizeof(a) / sizeof(*(a))) / (size_t)(!(sizeof(a) % sizeof(*(a)))))

static inline size_t alignment_loss(size_t bytes_allocated, size_t alignment) {
  if (alignment > bytes_allocated) {
    return bytes_allocated;
  }

  size_t offset = bytes_allocated & (alignment - 1);

  if (offset == 0) {
    return 0;
  } else {
    return alignment - offset;
  }
}

#define dynamic_array_add demogobbler_dynamic_array_add
#define dynamic_array_free demogobbler_dynamic_array_free
#define dynamic_array_init demogobbler_dynamic_array_init
#define dynamic_array_get demogobbler_dynamic_array_get
#define dynamic_array_offset demogobbler_dynamic_array_offset

unsigned demogobbler_bits_required(unsigned i);
unsigned int highest_bit_index(unsigned int number);
int Q_log2(int val);
