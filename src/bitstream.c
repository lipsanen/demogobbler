#include "demogobbler/bitstream.h"
#include "arena.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// Turn this macro on for more readable profiler output
#define FUN_ATTRIBUTE //__attribute__((noinline))

#ifdef _MSC_VER
#define NO_ASAN
#else
#define NO_ASAN __attribute__((no_sanitize("address")))
#endif

// This rounds up
static inline uint32_t FUN_ATTRIBUTE get_size_in_bytes(uint32_t bits) {
  if (bits & 0x7) {
    return bits / 8 + 1;
  } else {
    return bits / 8;
  }
}

static inline unsigned int FUN_ATTRIBUTE buffered_bits(dg_bitstream *thisptr) {
  uint8_t *cur_address = (uint8_t *)thisptr->data + thisptr->bitoffset / 8;
  uint32_t difference = (cur_address - thisptr->buffered_address);

  if (cur_address < thisptr->buffered_address || difference >= 8) {
    return 0;
  } else {
    return (thisptr->buffered_bytes_read - difference) * 8 - (thisptr->bitoffset & 0x7);
  }
}

static inline void NO_ASAN FUN_ATTRIBUTE fetch_ubit(dg_bitstream *thisptr) {
  if (thisptr->bitoffset >= thisptr->bitsize || thisptr->overflow) {
    thisptr->buffered = 0;
    thisptr->buffered_address = NULL;
    return;
  }

  uint64_t val = 0;
  thisptr->buffered_address = (uint8_t *)thisptr->data + (thisptr->bitoffset >> 3);
  uint32_t end_byte = get_size_in_bytes(thisptr->bitsize);
  thisptr->buffered_bytes_read = end_byte - (thisptr->bitoffset >> 3);
  thisptr->buffered_bytes_read = MIN(thisptr->buffered_bytes_read, 8);

  for (size_t i = 0; i < thisptr->buffered_bytes_read; ++i) {
    val |= ((uint64_t)thisptr->buffered_address[i]) << (8 * i);
  }

  thisptr->buffered = val;

  uint32_t byte_offset = thisptr->bitoffset & 0x7;

  if (byte_offset != 0) {
    thisptr->buffered >>= byte_offset;
  }
}

dg_bitstream FUN_ATTRIBUTE dg_bitstream_create(void *data, size_t size) {
  dg_bitstream stream;
  memset(&stream, 0, sizeof(dg_bitstream));
  stream.data = data;
  stream.bitsize = size;
  stream.bitoffset = 0;
  stream.overflow = false;
  return stream;
}

void FUN_ATTRIBUTE dg_bitstream_advance(dg_bitstream *thisptr, unsigned int bits) {
  thisptr->bitoffset += bits;
  thisptr->buffered >>= bits;

  if (thisptr->bitoffset > thisptr->bitsize) {
    thisptr->bitoffset = thisptr->bitsize;
    thisptr->overflow = true;
  }
}

dg_bitstream FUN_ATTRIBUTE dg_bitstream_fork_and_advance(dg_bitstream *stream, unsigned int bits) {
  dg_bitstream output;

  memset(&output, 0, sizeof(output));
  output.bitoffset = stream->bitoffset;
  output.bitsize = stream->bitoffset + bits;
  output.data = stream->data;
  output.overflow = stream->overflow;
  dg_bitstream_advance(stream, bits);

  return output;
}

static inline uint64_t FUN_ATTRIBUTE read_ubit_slow(dg_bitstream *thisptr,
                                                    unsigned requested_bits) {
  if (thisptr->overflow) {
    return 0;
  }

  uint64_t rval;
  unsigned int bits_left = requested_bits;

  if (buffered_bits(thisptr) == 0) {
    fetch_ubit(thisptr);
  }

  if (buffered_bits(thisptr) >= bits_left) {
    rval = thisptr->buffered << (64 - bits_left);
    rval >>= (64 - bits_left);
    thisptr->buffered >>= bits_left;
    thisptr->bitoffset += bits_left;
  } else {
    unsigned int first_read = buffered_bits(thisptr);
    rval = thisptr->buffered;
    thisptr->bitoffset += first_read;
    bits_left -= first_read;

    fetch_ubit(thisptr);

    uint64_t temp = thisptr->buffered << (64 - bits_left);
    temp >>= (64 - bits_left - first_read);

    rval |= temp;

    thisptr->bitoffset += bits_left;
    thisptr->buffered >>= bits_left;
  }

  if (thisptr->bitoffset > thisptr->bitsize) {
    thisptr->bitoffset = thisptr->bitsize;
    thisptr->overflow = true;
  }

  return rval;
}

static inline uint64_t FUN_ATTRIBUTE NO_ASAN read_ubit(dg_bitstream *thisptr,
                                                       unsigned requested_bits) {
  if (requested_bits > 56 || thisptr->overflow) {
    return read_ubit_slow(thisptr, requested_bits);
  }

  if (buffered_bits(thisptr) < requested_bits) {
    fetch_ubit(thisptr);
  }

  uint64_t rval = thisptr->buffered << (64 - requested_bits);
  rval >>= (64 - requested_bits);

  thisptr->bitoffset += requested_bits;
  thisptr->buffered >>= requested_bits;

  if (thisptr->bitoffset <= thisptr->bitsize) {
    return rval;
  } else {
    thisptr->overflow = true;
    return 0;
  }
}

void FUN_ATTRIBUTE dg_bitstream_read_fixed_string(dg_bitstream *thisptr, void *_dest,
                                                  size_t bytes) {
  uint8_t *dest = (uint8_t *)_dest;

  for (size_t i = 0; i < bytes; ++i) {
    uint8_t val = read_ubit(thisptr, 8);
    if (dest)
      dest[i] = val;
  }
}

bool FUN_ATTRIBUTE NO_ASAN dg_bitstream_read_bit(dg_bitstream *thisptr) {
  if (thisptr->overflow || thisptr->bitoffset >= thisptr->bitsize) {
    thisptr->overflow = true;
    return false;
  }

  uint8_t MASKS[] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};

  uint8_t *ptr = (uint8_t *)thisptr->data + (thisptr->bitoffset >> 3);
  int offset_alignment = thisptr->bitoffset & 0x7;
  bitstream_advance(thisptr, 1);

  return *ptr & MASKS[offset_alignment];
}

uint64_t FUN_ATTRIBUTE dg_bitstream_read_uint(dg_bitstream *thisptr, unsigned int bits) {
  return read_ubit(thisptr, bits);
}

int64_t FUN_ATTRIBUTE dg_bitstream_read_sint(dg_bitstream *thisptr, unsigned int bits) {
  int64_t n_ret = dg_bitstream_read_uint(thisptr, bits);
  // Sign magic
  return (n_ret << (64 - bits)) >> (64 - bits);
}

float FUN_ATTRIBUTE dg_bitstream_read_float(dg_bitstream *thisptr) {
  union result {
    uint32_t uint;
    float res;
  };

  union result out;
  out.uint = dg_bitstream_read_uint32(thisptr);
  return out.res;
}

size_t FUN_ATTRIBUTE dg_bitstream_read_cstring(dg_bitstream *thisptr, char *dest,
                                               size_t max_bytes) {
  size_t i;
  bool overflow = true;
  for (i = 0; i < max_bytes; ++i) {
    uint64_t value = read_ubit(thisptr, 8);
    char c = *(char *)&value;
    dest[i] = c;

    if (value == 0) {
      overflow = false;
      ++i;
      break;
    }
  }

  if (overflow) {
    thisptr->overflow = true;
  }

  return i;
}

dg_bitangle_vector FUN_ATTRIBUTE dg_bitstream_read_bitvector(dg_bitstream *thisptr,
                                                             unsigned int bits) {
  dg_bitangle_vector out;
  out.x = bitstream_read_uint(thisptr, bits);
  out.y = bitstream_read_uint(thisptr, bits);
  out.z = bitstream_read_uint(thisptr, bits);
  out.bits = bits;
  return out;
}

dg_bitcoord_vector FUN_ATTRIBUTE dg_bitstream_read_coordvector(dg_bitstream *thisptr) {
  dg_bitcoord_vector out;
  memset(&out, 0, sizeof(out));
  out.x.exists = bitstream_read_uint(thisptr, 1);
  out.y.exists = bitstream_read_uint(thisptr, 1);
  out.z.exists = bitstream_read_uint(thisptr, 1);

  if (out.x.exists)
    out.x = dg_bitstream_read_bitcoord(thisptr);
  if (out.y.exists)
    out.y = dg_bitstream_read_bitcoord(thisptr);
  if (out.z.exists)
    out.z = dg_bitstream_read_bitcoord(thisptr);
  return out;
}

dg_bitcoord FUN_ATTRIBUTE dg_bitstream_read_bitcoord(dg_bitstream *thisptr) {
#if 1
  dg_bitcoord out;
  memset(&out, 0, sizeof(out));
  out.exists = true;

  const uint32_t bits = COORD_INTEGER_BITS + COORD_FRACTIONAL_BITS + 3;
  unsigned int bits_left = buffered_bits(thisptr);
  if (bits > bits_left) {
    fetch_ubit(thisptr);
  }

  uint64_t val = thisptr->buffered;
  unsigned bits_used = 2;

  out.has_int = (val & 0x1) != 0;
  out.has_frac = (val & 0x2) != 0;

  if (out.has_int || out.has_frac) {
    out.sign = (val & 0x4) != 0;
    val >>= 3;
    bits_used = 3;

    if (out.has_int) {
      const unsigned mask = ((1 << COORD_INTEGER_BITS) - 1);
      out.int_value = val & mask;
      val >>= COORD_INTEGER_BITS;
      bits_used += COORD_INTEGER_BITS;
    }
    if (out.has_frac) {
      const unsigned mask = ((1 << COORD_FRACTIONAL_BITS) - 1);
      out.frac_value = val & mask;
      bits_used += COORD_FRACTIONAL_BITS;
    }
  }

  bitstream_advance(thisptr, bits_used);

  return out;
#else
  // Old slow code
  dg_bitcoord out;
  memset(&out, 0, sizeof(out));
  out.exists = true;
  out.has_int = dg_bitstream_read_uint(thisptr, 1);
  out.has_frac = dg_bitstream_read_uint(thisptr, 1);

  if (out.has_int || out.has_frac) {
    out.sign = dg_bitstream_read_uint(thisptr, 1);
    if (out.has_int)
      out.int_value = dg_bitstream_read_uint(thisptr, COORD_INTEGER_BITS);
    if (out.has_frac)
      out.frac_value = dg_bitstream_read_uint(thisptr, COORD_FRACTIONAL_BITS);
  }

  return out;
#endif
}

uint32_t FUN_ATTRIBUTE dg_bitstream_read_uint32(dg_bitstream *thisptr) {
  return dg_bitstream_read_uint(thisptr, 32);
}

uint32_t FUN_ATTRIBUTE dg_bitstream_read_varuint32(dg_bitstream *thisptr) {
  uint32_t result = 0;
  for (int i = 0; i < 5; i++) {
    uint32_t b = dg_bitstream_read_uint(thisptr, 8);
    result |= (b & 0x7F) << (7 * i);
    if ((b & 0x80) == 0)
      break;
  }
  return result;
}

int32_t FUN_ATTRIBUTE dg_bitstream_read_sint32(dg_bitstream *thisptr) {
  return dg_bitstream_read_sint(thisptr, 32);
}

uint32_t FUN_ATTRIBUTE dg_bitstream_read_ubitint(dg_bitstream *thisptr) {
  uint32_t ret = bitstream_read_uint(thisptr, 4);
  uint32_t num = bitstream_read_uint(thisptr, 2);
  uint32_t add = 0;

  switch (num) {
  case 1:
    add = bitstream_read_uint(thisptr, 4);
    break;
  case 2:
    add = bitstream_read_uint(thisptr, 8);
    break;
  case 3:
    add = bitstream_read_uint(thisptr, 28);
    break;
  default:
    break;
  }

  return ret | (add << 4);
}

uint32_t FUN_ATTRIBUTE dg_bitstream_read_ubitvar(dg_bitstream *thisptr) {
#if 1
  if (thisptr->overflow)
    return 0;

  if (buffered_bits(thisptr) < 34) {
    fetch_ubit(thisptr);
  }

  const unsigned int masks[] = {(1 << 4) - 1, (1 << 8) - 1, (1 << 12) - 1, UINT32_MAX};

  const unsigned int bits_per_sel[] = {6, 10, 14, 34};

  uint64_t val = thisptr->buffered;
  uint32_t sel = val & 0x3;

  uint32_t output = (val >> 2) & masks[sel];
  bitstream_advance(thisptr, bits_per_sel[sel]);

  return output;

#else
  uint32_t sel = bitstream_read_uint(thisptr, 2);

  switch (sel) {
  case 0:
    return bitstream_read_uint(thisptr, 4);
  case 1:
    return bitstream_read_uint(thisptr, 8);
  case 2:
    return bitstream_read_uint(thisptr, 12);
  default:
    return bitstream_read_uint(thisptr, 32);
  }
#endif
}

dg_bitcellcoord FUN_ATTRIBUTE dg_bitstream_read_bitcellcoord(dg_bitstream *thisptr, bool is_int,
                                                             bool lp, unsigned bits) {
  dg_bitcellcoord output;
  memset(&output, 0, sizeof(output));
  output.int_val = bitstream_read_uint(thisptr, bits);

  if (!is_int) {
    if (lp) {
      output.fract_val = bitstream_read_uint(thisptr, FRAC_BITS_LP);
    } else {
      output.fract_val = bitstream_read_uint(thisptr, FRAC_BITS);
    }
  }

  return output;
}

dg_bitcoordmp FUN_ATTRIBUTE dg_bitstream_read_bitcoordmp(dg_bitstream *thisptr, bool is_int,
                                                         bool lp) {
  dg_bitcoordmp output;
  memset(&output, 0, sizeof(output));
  output.inbounds = bitstream_read_bit(thisptr);
  if (is_int) {
    output.int_has_val = bitstream_read_bit(thisptr);
    if (output.int_has_val) {
      output.sign = bitstream_read_bit(thisptr);

      if (output.inbounds) {
        output.int_val = bitstream_read_uint(thisptr, COORD_INT_BITS_MP);
      } else {
        output.int_val = bitstream_read_uint(thisptr, COORD_INTEGER_BITS);
      }
    }
  } else {
    output.int_has_val = bitstream_read_bit(thisptr);
    output.sign = bitstream_read_bit(thisptr);

    if (output.int_has_val) {
      if (output.inbounds) {
        output.int_val = bitstream_read_uint(thisptr, COORD_INT_BITS_MP);
      } else {
        output.int_val = bitstream_read_uint(thisptr, COORD_INTEGER_BITS);
      }
    }

    if (lp) {
      output.frac_val = bitstream_read_uint(thisptr, FRAC_BITS_LP);
    } else {
      output.frac_val = bitstream_read_uint(thisptr, FRAC_BITS);
    }
  }

  return output;
}

dg_bitnormal FUN_ATTRIBUTE dg_bitstream_read_bitnormal(dg_bitstream *thisptr) {
  dg_bitnormal output;
  memset(&output, 0, sizeof(output));
  const size_t frac_bits = 11;
  output.sign = bitstream_read_bit(thisptr);
  output.frac = bitstream_read_uint(thisptr, frac_bits);

  return output;
}

int32_t FUN_ATTRIBUTE dg_bitstream_read_field_index(dg_bitstream *thisptr, int32_t last_index,
                                                    bool new_way) {
  if (new_way && bitstream_read_bit(thisptr))
    return last_index + 1;

  int32_t ret;

  if (new_way && bitstream_read_bit(thisptr)) {
    ret = bitstream_read_uint(thisptr, 3);
  } else {
    ret = bitstream_read_uint(thisptr, 5);
    uint32_t sw = bitstream_read_uint(thisptr, 2);

    switch (sw) {
    case 1:
      ret |= bitstream_read_uint(thisptr, 2) << 5;
      break;
    case 2:
      ret |= bitstream_read_uint(thisptr, 4) << 5;
      break;
    case 3:
      ret |= bitstream_read_uint(thisptr, 7) << 5;
      break;
    default:
      break;
    }
  }

  if (ret == 0xFFF)
    return -1;

  return last_index + 1 + ret;
}
