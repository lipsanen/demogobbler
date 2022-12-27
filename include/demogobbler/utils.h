#pragma once

#include <stdint.h>
#include <stdlib.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y))
#define CLAMP(min, value, max) MIN(MAX(min, value), max)

enum { COORD_INT_BITS_MP = 11 };
enum { FRAC_BITS_LP = 3 };
enum { FRAC_BITS = 5 };
enum { MAX_EDICTS = 2048 };
enum { MAX_EDICT_BITS = 11 };
enum { ENTITY_SENTINEL = 9999 };
enum { NORM_FRAC_BITS = 11 };
enum { NORM_DENOM = (1 << NORM_FRAC_BITS) - 1 };
enum { HANDLE_BITS = 10 };

#define COORD_INTEGER_BITS 14
#define COORD_FRACTIONAL_BITS 5
#define COORD_DENOMINATOR (1 << (COORD_FRACTIONAL_BITS))
#define COORD_RESOLUTION 1.0f / (1 << COORD_FRACTIONAL_BITS)
#define COORD_RESOLUTION_LP 1.0f / (1 << FRAC_BITS_LP)
#define NORM_RES (1.0f / NORM_DENOM)
#define ARRAYSIZE(a) ((sizeof(a) / sizeof(*(a))) / (size_t)(!(sizeof(a) % sizeof(*(a)))))

static inline size_t alignment_loss(size_t bytes_allocated, size_t alignment) {
  size_t offset = bytes_allocated & (alignment - 1);

  if (offset == 0) {
    return 0;
  } else {
    return alignment - offset;
  }
}

unsigned dg_bits_required(unsigned i);
unsigned int highest_bit_index(unsigned int number);
int Q_log2(int val);
