#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "demogobbler_vector.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct demogobbler_bitstream {
  void* data;
  int64_t bitsize;
  int64_t bitoffset;
  bool overflow;
};

typedef struct demogobbler_bitstream bitstream;

bitstream demogobbler_bitstream_create(void* data, size_t size);
void demogobbler_bitstream_advance(bitstream* thisptr, unsigned int bits);
bitstream demogobbler_bitstream_fork_and_advance(bitstream* stream, unsigned int bits);
uint64_t demogobbler_bitstream_read_uint(bitstream* thisptr, unsigned int bits);
static inline uint32_t demogobbler_bitstream_read_uint32(bitstream* thisptr) { return demogobbler_bitstream_read_uint(thisptr, 32); }
int64_t demogobbler_bitstream_read_sint(bitstream* thisptr, unsigned int bits);
static inline int32_t demogobbler_bitstream_read_sint32(bitstream* thisptr) { return demogobbler_bitstream_read_sint(thisptr, 32); }
float demogobbler_bitstream_read_float(bitstream* thisptr);
void demogobbler_bitstream_read_bits(bitstream* thisptr, void* dest, unsigned bits);
size_t demogobbler_bitstream_read_cstring(bitstream* thisptr, char* dest, size_t max_bytes);
vector demogobbler_bitstream_read_bitvector(bitstream* thisptr, unsigned int bits);

#define bitstream_create demogobbler_bitstream_create
#define bitstream_advance demogobbler_bitstream_advance
#define bitstream_fork_and_advance demogobbler_bitstream_fork_and_advance
#define bitstream_read_uint demogobbler_bitstream_read_uint
#define bitstream_read_uint32 demogobbler_bitstream_read_uint32
#define bitstream_read_sint demogobbler_bitstream_read_sint
#define bitstream_read_sint32 demogobbler_bitstream_read_sint32
#define bitstream_read_bits demogobbler_bitstream_read_bits
#define bitstream_read_cstring demogobbler_bitstream_read_cstring
#define bitstream_read_float demogobbler_bitstream_read_float
#define bitstream_read_bitvector demogobbler_bitstream_read_bitvector

static inline int64_t demogobbler_bitstream_bits_left(bitstream* thisptr) {
  if(thisptr->bitsize >= thisptr->bitoffset) {
    return thisptr->bitsize - thisptr->bitoffset;
  }
  else {
    return 0;
  }
}

#ifdef __cplusplus
}
#endif
