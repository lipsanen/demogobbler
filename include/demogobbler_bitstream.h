#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "demogobbler_floats.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct demogobbler_bitstream {
  void *data;
  int64_t bitsize;
  int64_t bitoffset;
  bool overflow;
  uint64_t buffered;
  unsigned int buffered_bits;
  uint8_t* buffered_address;
  uint64_t buffered_bytes_read;
};

typedef struct demogobbler_bitstream bitstream;
struct demogobbler_sendprop;

bitstream demogobbler_bitstream_create(void *data, size_t size);
void demogobbler_bitstream_advance(bitstream *thisptr, unsigned int bits);
bitstream demogobbler_bitstream_fork_and_advance(bitstream *stream, unsigned int bits);
bool demogobbler_bitstream_read_bit(bitstream* thisptr);
uint64_t demogobbler_bitstream_read_uint(bitstream *thisptr, unsigned int bits);
int64_t demogobbler_bitstream_read_sint(bitstream *thisptr, unsigned int bits);
float demogobbler_bitstream_read_float(bitstream *thisptr);
void demogobbler_bitstream_read_fixed_string(bitstream *thisptr, void *dest, size_t max_bytes);
size_t demogobbler_bitstream_read_cstring(bitstream *thisptr, char *dest, size_t max_bytes);
bitangle_vector demogobbler_bitstream_read_bitvector(bitstream *thisptr, unsigned int bits);
bitcoord_vector demogobbler_bitstream_read_coordvector(bitstream *thisptr);
bitcoord demogobbler_bitstream_read_bitcoord(bitstream* thisptr);
uint32_t demogobbler_bitstream_read_varuint32(bitstream *thisptr);
uint32_t demogobbler_bitstream_read_uint32(bitstream *thisptr);
uint32_t demogobbler_bitstream_read_ubitvar(bitstream* thisptr);
int32_t demogobbler_bitstream_read_sint32(bitstream *thisptr);
uint32_t demogobbler_bitstream_read_ubitint(bitstream* thisptr);
uint32_t demogobbler_bitstream_read_ubitvar(bitstream* thisptr);
demogobbler_bitcellcoord demogobbler_bitstream_read_bitcellcoord(bitstream* thisptr, bool is_int, bool lp, unsigned bits);
demogobbler_bitcoordmp demogobbler_bitstream_read_bitcoordmp(bitstream* thisptr, bool is_int, bool lp);
demogobbler_bitnormal demogobbler_bitstream_read_bitnormal(bitstream* thisptr);
int32_t demogobbler_bitstream_read_field_index(bitstream* thisptr, int32_t last_index, bool new_way);

#define bitstream_create demogobbler_bitstream_create
#define bitstream_advance demogobbler_bitstream_advance
#define bitstream_fork_and_advance demogobbler_bitstream_fork_and_advance
#define bitstream_read_bit demogobbler_bitstream_read_bit
#define bitstream_read_uint demogobbler_bitstream_read_uint
#define bitstream_read_ubitint demogobbler_bitstream_read_ubitint
#define bitstream_read_ubitvar demogobbler_bitstream_read_ubitvar
#define bitstream_read_uint32 demogobbler_bitstream_read_uint32
#define bitstream_read_varuint32 demogobbler_bitstream_read_varuint32
#define bitstream_read_sint demogobbler_bitstream_read_sint
#define bitstream_read_sint32 demogobbler_bitstream_read_sint32
#define bitstream_read_fixed_string demogobbler_bitstream_read_fixed_string
#define bitstream_read_cstring demogobbler_bitstream_read_cstring
#define bitstream_read_float demogobbler_bitstream_read_float
#define bitstream_read_field_index demogobbler_bitstream_read_field_index
#define bitstream_read_bitvector demogobbler_bitstream_read_bitvector
#define bitstream_read_coordvector demogobbler_bitstream_read_coordvector

static inline int64_t demogobbler_bitstream_bits_left(bitstream *thisptr) {
  if (thisptr->bitsize >= thisptr->bitoffset) {
    return thisptr->bitsize - thisptr->bitoffset;
  } else {
    return 0;
  }
}

#ifdef __cplusplus
}
#endif
