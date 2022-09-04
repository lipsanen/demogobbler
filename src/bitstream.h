#pragma once

#include "demogobbler_bitstream.h"
#include "demogobbler_floats.h"

#ifdef BITSTREAM_INLINED
#define BITSTREAM_PREAMBLE static inline
#else
#define BITSTREAM_PREAMBLE
#endif

struct demogobbler_sendprop;

BITSTREAM_PREAMBLE bitstream demogobbler_bitstream_create(void *data, size_t size);
BITSTREAM_PREAMBLE void demogobbler_bitstream_advance(bitstream *thisptr, unsigned int bits);
BITSTREAM_PREAMBLE bitstream demogobbler_bitstream_fork_and_advance(bitstream *stream, unsigned int bits);
BITSTREAM_PREAMBLE bool demogobbler_bitstream_read_bit(bitstream* thisptr);
BITSTREAM_PREAMBLE uint64_t demogobbler_bitstream_read_uint(bitstream *thisptr, unsigned int bits);
BITSTREAM_PREAMBLE int64_t demogobbler_bitstream_read_sint(bitstream *thisptr, unsigned int bits);
BITSTREAM_PREAMBLE float demogobbler_bitstream_read_float(bitstream *thisptr);
BITSTREAM_PREAMBLE void demogobbler_bitstream_read_fixed_string(bitstream *thisptr, void *dest, size_t max_bytes);
BITSTREAM_PREAMBLE size_t demogobbler_bitstream_read_cstring(bitstream *thisptr, char *dest, size_t max_bytes);
BITSTREAM_PREAMBLE bitangle_vector demogobbler_bitstream_read_bitvector(bitstream *thisptr, unsigned int bits);
BITSTREAM_PREAMBLE bitcoord_vector demogobbler_bitstream_read_coordvector(bitstream *thisptr);
BITSTREAM_PREAMBLE bitcoord demogobbler_bitstream_read_bitcoord(bitstream* thisptr);
BITSTREAM_PREAMBLE uint32_t demogobbler_bitstream_read_varuint32(bitstream *thisptr);
BITSTREAM_PREAMBLE uint32_t demogobbler_bitstream_read_uint32(bitstream *thisptr);
BITSTREAM_PREAMBLE uint32_t demogobbler_bitstream_read_ubitvar(bitstream* thisptr);
BITSTREAM_PREAMBLE int32_t demogobbler_bitstream_read_sint32(bitstream *thisptr);
BITSTREAM_PREAMBLE uint32_t demogobbler_bitstream_read_ubitint(bitstream* thisptr);
BITSTREAM_PREAMBLE uint32_t demogobbler_bitstream_read_ubitvar(bitstream* thisptr);
BITSTREAM_PREAMBLE demogobbler_bitcellcoord demogobbler_bitstream_read_bitcellcoord(bitstream* thisptr, bool is_int, bool lp, unsigned bits);
BITSTREAM_PREAMBLE demogobbler_bitcoordmp demogobbler_bitstream_read_bitcoordmp(bitstream* thisptr, bool is_int, bool lp);
BITSTREAM_PREAMBLE demogobbler_bitnormal demogobbler_bitstream_read_bitnormal(bitstream* thisptr);
BITSTREAM_PREAMBLE int32_t demogobbler_bitstream_read_field_index(bitstream* thisptr, int32_t last_index, bool new_way);

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

#ifdef BITSTREAM_INLINED
#include "bitstream_impl.h"
#endif
