#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Because this library aims to be able to produce bit-for-bit equal demos
// with the original, encoded floats are stored in their encoded form.
// This header contains a bunch of types for that.

#include <stdbool.h>
#include <stdint.h>

struct dg_bitangle_vector {
  uint32_t x;
  uint32_t y;
  uint32_t z;
  unsigned int bits : 6;
};

typedef struct dg_bitangle_vector dg_bitangle_vector;

struct dg_bitcoord {
  unsigned exists : 1; // Used by vector
  unsigned has_int : 1;
  unsigned has_frac : 1;
  unsigned sign : 1;
  unsigned int_value : 14;
  unsigned frac_value : 5;
};

typedef struct dg_bitcoord dg_bitcoord;

struct dg_bitcoord_vector {
  dg_bitcoord x;
  dg_bitcoord y;
  dg_bitcoord z;
};

typedef struct dg_bitcoord_vector dg_bitcoord_vector;

typedef struct {
  unsigned frac;
  bool sign;
} dg_bitnormal;

typedef struct {
  unsigned int_val;
  unsigned fract_val;
} dg_bitcellcoord;

typedef struct {
  unsigned int_val;
  unsigned frac_val;
  bool inbounds;
  bool int_has_val;
  bool sign;
} dg_bitcoordmp;

#ifdef __cplusplus
}
#endif
