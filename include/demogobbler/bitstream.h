#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "demogobbler/floats.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct dg_bitstream {
  void *data;
  uint32_t bitsize;
  uint32_t bitoffset;
  uint64_t buffered;
  uint8_t *buffered_address;
  uint32_t buffered_bytes_read;
  bool overflow;
};

typedef struct dg_bitstream dg_bitstream;

dg_bitstream dg_bitstream_create(void *data, size_t size);
void dg_bitstream_advance(dg_bitstream *thisptr, unsigned int bits);
dg_bitstream dg_bitstream_fork_and_advance(dg_bitstream *stream, unsigned int bits);
bool dg_bitstream_read_bit(dg_bitstream *thisptr);
uint64_t dg_bitstream_read_uint(dg_bitstream *thisptr, unsigned int bits);
int64_t dg_bitstream_read_sint(dg_bitstream *thisptr, unsigned int bits);
float dg_bitstream_read_float(dg_bitstream *thisptr);
void dg_bitstream_read_fixed_string(dg_bitstream *thisptr, void *dest, size_t max_bytes);
size_t dg_bitstream_read_cstring(dg_bitstream *thisptr, char *dest, size_t max_bytes);
dg_bitangle_vector dg_bitstream_read_bitvector(dg_bitstream *thisptr, unsigned int bits);
dg_bitcoord_vector dg_bitstream_read_coordvector(dg_bitstream *thisptr);
dg_bitcoord dg_bitstream_read_bitcoord(dg_bitstream *thisptr);
uint32_t dg_bitstream_read_varuint32(dg_bitstream *thisptr);
uint32_t dg_bitstream_read_uint32(dg_bitstream *thisptr);
uint32_t dg_bitstream_read_ubitvar(dg_bitstream *thisptr);
int32_t dg_bitstream_read_sint32(dg_bitstream *thisptr);
uint32_t dg_bitstream_read_ubitint(dg_bitstream *thisptr);
dg_bitcellcoord dg_bitstream_read_bitcellcoord(dg_bitstream *thisptr, bool is_int, bool lp,
                                               unsigned bits);
dg_bitcoordmp dg_bitstream_read_bitcoordmp(dg_bitstream *thisptr, bool is_int, bool lp);
dg_bitnormal dg_bitstream_read_bitnormal(dg_bitstream *thisptr);
int32_t dg_bitstream_read_field_index(dg_bitstream *thisptr, int32_t last_index, bool new_way);

#define bitstream_create dg_bitstream_create
#define bitstream_advance dg_bitstream_advance
#define bitstream_fork_and_advance dg_bitstream_fork_and_advance
#define bitstream_read_bit dg_bitstream_read_bit
#define bitstream_read_uint dg_bitstream_read_uint
#define bitstream_read_ubitint dg_bitstream_read_ubitint
#define bitstream_read_ubitvar dg_bitstream_read_ubitvar
#define bitstream_read_uint32 dg_bitstream_read_uint32
#define bitstream_read_varuint32 dg_bitstream_read_varuint32
#define bitstream_read_sint dg_bitstream_read_sint
#define bitstream_read_sint32 dg_bitstream_read_sint32
#define bitstream_read_fixed_string dg_bitstream_read_fixed_string
#define bitstream_read_cstring dg_bitstream_read_cstring
#define bitstream_read_float dg_bitstream_read_float
#define bitstream_read_field_index dg_bitstream_read_field_index
#define bitstream_read_bitvector dg_bitstream_read_bitvector
#define bitstream_read_coordvector dg_bitstream_read_coordvector

static inline int64_t dg_bitstream_bits_left(dg_bitstream *thisptr) {
  if (thisptr->bitsize >= thisptr->bitoffset && !thisptr->overflow) {
    return thisptr->bitsize - thisptr->bitoffset;
  } else {
    return 0;
  }
}

#ifdef __cplusplus
}
#endif
