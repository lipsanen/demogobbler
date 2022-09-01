#include "arena.h"
#include "bitstream.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

// This rounds up
static uint64_t get_size_in_bytes(uint64_t bits) {
  if (bits & 0x7) {
    return bits / 8 + 1;
  } else {
    return bits / 8;
  }
}

static unsigned int buffered_bits(bitstream *thisptr) {
  uint8_t *cur_address = (uint8_t *)thisptr->data + thisptr->bitoffset / 8;
  uint64_t difference = (cur_address - thisptr->buffered_address);

  if (cur_address < thisptr->buffered_address || difference >= 8) {
    return 0;
  } else {
    return (thisptr->buffered_bytes_read - difference) * 8 - (thisptr->bitoffset & 0x7);
  }
}

static void __attribute__((no_sanitize("address"))) fetch_ubit(bitstream *thisptr) {
  if (thisptr->bitoffset >= thisptr->bitsize) {
    thisptr->buffered = 0;
    thisptr->buffered_bits = 64;
    thisptr->buffered_address = NULL;
    return;
  }

  uint64_t val = 0;
  thisptr->buffered_address = (uint8_t *)thisptr->data + thisptr->bitoffset / 8;
  uint64_t end_byte = get_size_in_bytes(thisptr->bitsize);
  thisptr->buffered_bytes_read = end_byte - thisptr->bitoffset / 8;
  thisptr->buffered_bytes_read = MIN(thisptr->buffered_bytes_read, 8);

  for (int i = 0; i < thisptr->buffered_bytes_read; ++i) {
    val |= ((uint64_t)thisptr->buffered_address[i]) << (8 * i);
  }

  thisptr->buffered = val;
  thisptr->buffered_bits = thisptr->buffered_bytes_read * 8;

  int64_t byte_offset = thisptr->bitoffset & 0x7;

  if (byte_offset != 0) {
    thisptr->buffered >>= byte_offset;
    thisptr->buffered_bits -= byte_offset;
  }
}

bitstream demogobbler_bitstream_create(void *data, size_t size) {
  bitstream stream;
  memset(&stream, 0, sizeof(bitstream));
  stream.data = data;
  stream.bitsize = size;
  stream.bitoffset = 0;
  stream.overflow = false;
  return stream;
}

void demogobbler_bitstream_advance(bitstream *thisptr, unsigned int bits) {
  int64_t diff = thisptr->bitsize - thisptr->bitoffset;

  if (bits > diff) {
    thisptr->bitoffset = thisptr->bitsize;
    thisptr->overflow = true;
  } else {
    thisptr->bitoffset = bits + thisptr->bitoffset;
    if (bits > thisptr->buffered_bits) {
      thisptr->buffered = 0;
      thisptr->buffered_bits = 0;
    } else {
      thisptr->buffered_bits -= bits;
      thisptr->buffered >>= bits;
    }
  }
}

bitstream demogobbler_bitstream_fork_and_advance(bitstream *stream, unsigned int bits) {
  bitstream output;

  memset(&output, 0, sizeof(output));
  output.bitoffset = stream->bitoffset;
  output.bitsize = stream->bitoffset + bits;
  output.data = stream->data;
  output.overflow = stream->overflow;
  demogobbler_bitstream_advance(stream, bits);

  return output;
}

static uint64_t __attribute__((no_sanitize("address")))
read_ubit(bitstream *thisptr, unsigned requested_bits) {
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

void demogobbler_bitstream_read_fixed_string(bitstream *thisptr, void *_dest, size_t bytes) {
  uint8_t *dest = (uint8_t *)_dest;

  for (size_t i = 0; i < bytes; ++i) {
    uint8_t val = read_ubit(thisptr, 8);
    if(dest)
      dest[i] = val;
  }
}

bool __attribute__((no_sanitize("address"))) demogobbler_bitstream_read_bit(bitstream *thisptr) {
  if (thisptr->overflow || thisptr->bitoffset >= thisptr->bitsize) {
    thisptr->overflow = true;
    return false;
  }

  uint8_t MASKS[] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};

  uint8_t *ptr = (uint8_t *)thisptr->data + thisptr->bitoffset / 8;
  int offset_alignment = thisptr->bitoffset & 0x7;
  bitstream_advance(thisptr, 1);

  return *ptr & MASKS[offset_alignment];
}

uint64_t demogobbler_bitstream_read_uint(bitstream *thisptr, unsigned int bits) {
  return read_ubit(thisptr, bits);
}

int64_t demogobbler_bitstream_read_sint(bitstream *thisptr, unsigned int bits) {
  int64_t n_ret = demogobbler_bitstream_read_uint(thisptr, bits);
  // Sign magic
  return (n_ret << (64 - bits)) >> (64 - bits);
}

float demogobbler_bitstream_read_float(bitstream *thisptr) {
  uint32_t uint = demogobbler_bitstream_read_uint32(thisptr);

  return *(float *)&uint;
}

size_t demogobbler_bitstream_read_cstring(bitstream *thisptr, char *dest, size_t max_bytes) {
  size_t i;
  for (i = 0; i < max_bytes; ++i) {
    uint64_t value = read_ubit(thisptr, 8);
    char c = *(char *)&value;
    dest[i] = c;

    if (value == 0) {
      ++i;
      break;
    }
  }

  return i;
}

bitangle_vector demogobbler_bitstream_read_bitvector(bitstream *thisptr, unsigned int bits) {
  bitangle_vector out;
  out.x = bitstream_read_uint(thisptr, bits);
  out.y = bitstream_read_uint(thisptr, bits);
  out.z = bitstream_read_uint(thisptr, bits);
  out.bits = bits;
  return out;
}

bitcoord_vector demogobbler_bitstream_read_coordvector(bitstream *thisptr) {
  bitcoord_vector out;
  memset(&out, 0, sizeof(out));
  out.x.exists = bitstream_read_uint(thisptr, 1);
  out.y.exists = bitstream_read_uint(thisptr, 1);
  out.z.exists = bitstream_read_uint(thisptr, 1);

  if (out.x.exists)
    out.x = demogobbler_bitstream_read_bitcoord(thisptr);
  if (out.y.exists)
    out.y = demogobbler_bitstream_read_bitcoord(thisptr);
  if (out.z.exists)
    out.z = demogobbler_bitstream_read_bitcoord(thisptr);
  return out;
}

bitcoord demogobbler_bitstream_read_bitcoord(bitstream *thisptr) {
  bitcoord out;
  memset(&out, 0, sizeof(out));
  out.exists = true;
  out.has_int = demogobbler_bitstream_read_uint(thisptr, 1);
  out.has_frac = demogobbler_bitstream_read_uint(thisptr, 1);

  if (out.has_int || out.has_frac) {
    out.sign = demogobbler_bitstream_read_uint(thisptr, 1);
    if (out.has_int)
      out.int_value = demogobbler_bitstream_read_uint(thisptr, COORD_INTEGER_BITS);
    if (out.has_frac)
      out.frac_value = demogobbler_bitstream_read_uint(thisptr, COORD_FRACTIONAL_BITS);
  }

  return out;
}

uint32_t demogobbler_bitstream_read_uint32(bitstream *thisptr) {
  return demogobbler_bitstream_read_uint(thisptr, 32);
}

uint32_t demogobbler_bitstream_read_varuint32(bitstream *thisptr) {
  uint32_t result = 0;
  for (int i = 0; i < 5; i++) {
    uint32_t b = demogobbler_bitstream_read_uint(thisptr, 8);
    result |= (b & 0x7F) << (7 * i);
    if ((b & 0x80) == 0)
      break;
  }
  return result;
}

int32_t demogobbler_bitstream_read_sint32(bitstream *thisptr) {
  return demogobbler_bitstream_read_sint(thisptr, 32);
}

uint32_t demogobbler_bitstream_read_ubitint(bitstream *thisptr) {
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

uint32_t demogobbler_bitstream_read_ubitvar(bitstream* thisptr) {
  uint32_t sel = bitstream_read_uint(thisptr, 2);

  switch(sel) {
    case 0:
      return bitstream_read_uint(thisptr, 4);
    case 1:
      return bitstream_read_uint(thisptr, 8);
    case 2:
      return bitstream_read_uint(thisptr, 12);
    default:
      return bitstream_read_uint(thisptr, 32);
  }
}

demogobbler_bitcellcoord demogobbler_bitstream_read_bitcellcoord(bitstream *thisptr, bool is_int,
                                                                bool lp, unsigned bits) {
  demogobbler_bitcellcoord output;
  memset(&output, 0, sizeof(output));
  output.int_val = bitstream_read_uint(thisptr, bits);

  if(!is_int) {
    if(lp) {
      output.fract_val = bitstream_read_uint(thisptr, FRAC_BITS_LP);
    } else {
      output.fract_val = bitstream_read_uint(thisptr, FRAC_BITS);
    }
  }

  return output;
}

demogobbler_bitcoordmp demogobbler_bitstream_read_bitcoordmp(bitstream *thisptr, bool is_int,
                                                             bool lp) {
  demogobbler_bitcoordmp output;
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

    if(lp) {
      output.frac_val = bitstream_read_uint(thisptr, FRAC_BITS_LP);
    } else {
      output.frac_val = bitstream_read_uint(thisptr, FRAC_BITS);
    }
  }

  return output;
}

demogobbler_bitnormal demogobbler_bitstream_read_bitnormal(bitstream *thisptr) {
  demogobbler_bitnormal output;
  memset(&output, 0, sizeof(output));
  const size_t frac_bits = 11;
  output.sign = bitstream_read_bit(thisptr);
  output.frac = bitstream_read_uint(thisptr, frac_bits);

  return output;
}

int32_t demogobbler_bitstream_read_field_index(bitstream *thisptr, int32_t last_index, bool new_way) {
  if(new_way && bitstream_read_bit(thisptr))
    return last_index + 1;
  
  int32_t ret;

  if(new_way && bitstream_read_bit(thisptr)) {
    ret = bitstream_read_uint(thisptr, 3);
  } else {
    ret = bitstream_read_uint(thisptr, 5);
    uint32_t sw = bitstream_read_uint(thisptr, 2);

    switch(sw) {
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

  if(ret == 0xFFF)
    return -1;
  
  return last_index + 1 + ret;
}
