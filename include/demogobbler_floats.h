#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Because this library aims to be able to produce bit-for-bit equal demos
// with the original, encoded floats are stored in their encoded form.
// This header contains a bunch of types for that.

#include <stdbool.h>
#include <stdint.h>

struct demogobbler_bitangle_vector {
  uint32_t x;
  uint32_t y;
  uint32_t z;
  unsigned int bits : 6;
};

typedef struct demogobbler_bitangle_vector bitangle_vector;

struct demogobbler_bitcoord {
  bool exists : 1; // Used by vector
  bool has_int : 1;
  bool has_frac : 1;
  bool sign : 1;
  unsigned int int_value : 14; // 14 bits
  unsigned int frac_value : 5; // 5 bits
};

typedef struct demogobbler_bitcoord bitcoord;

struct demogobbler_bitcoord_vector {
  bitcoord x;
  bitcoord y;
  bitcoord z;
};

typedef struct demogobbler_bitcoord_vector bitcoord_vector;

#ifdef __cplusplus
}
#endif
