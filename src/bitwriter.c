#include "demogobbler_bitwriter.h"
#include "utils.h"
#include <string.h>

void demogobbler_bitwriter_init(bitwriter *thisptr, size_t initial_size_bits) {
  memset(thisptr, 0, sizeof(*thisptr));
  initial_size_bits += initial_size_bits & 0x7;
  thisptr->ptr = malloc(initial_size_bits / 8 + 1);
  thisptr->bitoffset = 0;
  thisptr->bitsize = initial_size_bits;
}

int64_t demogobbler_bitwriter_get_available_bits(bitwriter *thisptr) {
  if (thisptr->bitoffset > thisptr->bitsize) {
    return 0;
  } else {
    return thisptr->bitsize - thisptr->bitoffset;
  }
}

static void bitwriter_allocate_space_if_needed(bitwriter *thisptr, unsigned int bits_wanted) {
  if (demogobbler_bitwriter_get_available_bits(thisptr) < bits_wanted) {
    thisptr->bitsize = MAX(thisptr->bitsize * 2, thisptr->bitsize + bits_wanted);
    thisptr->bitsize += thisptr->bitsize & 0x7;
    thisptr->ptr = realloc(thisptr->ptr, thisptr->bitsize / 8 + 1);
  }
}

#define CHECK_SIZE() bitwriter_allocate_space_if_needed(thisptr, bits)

void demogobbler_bitwriter_write_bit(bitwriter *thisptr, bool value) {
  bitwriter_allocate_space_if_needed(thisptr, 1);
  uint8_t MASKS[] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
  int index = thisptr->bitoffset & 0x7;
  uint8_t *dest = thisptr->ptr + thisptr->bitoffset / 8;
  bool bit_value = (*dest & MASKS[index]);

  // If value does not match then flip the bit
  if (bit_value != value) {
    *dest ^= MASKS[index];
  }

  thisptr->bitoffset += 1;
}

void demogobbler_bitwriter_write_bits(bitwriter *thisptr, const void *_src, unsigned int bits) {
  CHECK_SIZE();
  unsigned int src_offset = 0;
  while (bits > 0) {
    int dest_byte_offset = thisptr->bitoffset & 0x7;
    int src_byte_offset = src_offset & 0x7;
    int bits_written = MIN(8 - dest_byte_offset, 8 - src_byte_offset);
    bits_written = MIN(bits, bits_written);
    const uint8_t *src = (uint8_t *)_src + src_offset / 8;
    uint8_t *dest = thisptr->ptr + thisptr->bitoffset / 8;

    // Get rid of the extra bits at the end of the destination value
    uint8_t cur_value = *dest << (8 - dest_byte_offset);
    cur_value >>= (8 - dest_byte_offset);

    uint8_t src_value = *src;
    src_value >>= src_byte_offset;
    src_value <<= (8 - bits_written);
    src_value >>= (8 - bits_written);
    src_value <<= dest_byte_offset;
    uint8_t output = cur_value | src_value;

    *dest = output;
    bits -= bits_written;
    src_offset += bits_written;
    thisptr->bitoffset += bits_written;
  }
}

void demogobbler_bitwriter_write_bitcoord(bitwriter *thisptr, bitcoord coord) {
  bitwriter_write_bit(thisptr, coord.has_int);
  bitwriter_write_bit(thisptr, coord.has_frac);

  if (coord.has_int || coord.has_frac) {
    bitwriter_write_bit(thisptr, coord.sign);

    if (coord.has_int) {
      bitwriter_write_uint(thisptr, coord.int_value, COORD_INTEGER_BITS);
    }

    if (coord.has_frac) {
      bitwriter_write_uint(thisptr, coord.frac_value, COORD_FRACTIONAL_BITS);
    }
  }
}

void demogobbler_bitwriter_write_bitstream(bitwriter *thisptr, bitstream *stream) {
  while (demogobbler_bitstream_bits_left(stream) > 32) {
    uint32_t value = bitstream_read_uint32(stream);
    bitwriter_write_bits(thisptr, &value, 32);
  }

  unsigned int bits = demogobbler_bitstream_bits_left(stream);

  if (bits > 0) {
    uint32_t remainder = bitstream_read_uint(stream, bits);
    bitwriter_write_bits(thisptr, &remainder, bits);
  }
}

void demogobbler_bitwriter_write_bitangle(bitwriter *thisptr, float value, unsigned int bits) {
  float shift = 1 << (bits - 1);
  unsigned int mask = shift - 1;

  float ratio = (value / 360.0f);
  int d = (int)(ratio * shift);
  d &= mask;

  bitwriter_write_uint(thisptr, (unsigned int)d, bits);
}

void demogobbler_bitwriter_write_bitvector(bitwriter *thisptr, bitangle_vector value) {
  bitwriter_write_uint(thisptr, value.x, value.bits);
  bitwriter_write_uint(thisptr, value.y, value.bits);
  bitwriter_write_uint(thisptr, value.z, value.bits);
}

void demogobbler_bitwriter_write_coordvector(bitwriter *thisptr, bitcoord_vector vec) {
  bitwriter_write_bit(thisptr, vec.x.exists);
  bitwriter_write_bit(thisptr, vec.y.exists);
  bitwriter_write_bit(thisptr, vec.z.exists);

  if (vec.x.exists)
    bitwriter_write_bitcoord(thisptr, vec.x);
  if (vec.y.exists)
    bitwriter_write_bitcoord(thisptr, vec.y);
  if (vec.z.exists)
    bitwriter_write_bitcoord(thisptr, vec.z);
}

void demogobbler_bitwriter_write_cstring(bitwriter *thisptr, const char *text) {
  bitwriter_write_bits(thisptr, (void *)text, (strlen(text) + 1) * 8);
}

void demogobbler_bitwriter_write_float(bitwriter *thisptr, float value) {
  bitwriter_write_bits(thisptr, &value, 32);
}

void demogobbler_bitwriter_write_sint(bitwriter *thisptr, int64_t value, unsigned int bits) {
  bitwriter_write_uint(thisptr, value, bits);
}

void demogobbler_bitwriter_write_sint32(bitwriter *thisptr, int32_t value) {
  bitwriter_write_sint(thisptr, value, 32);
}

void demogobbler_bitwriter_write_uint(bitwriter *thisptr, uint64_t value, unsigned int bits) {
  bitwriter_write_bits(thisptr, &value, bits);
}

void demogobbler_bitwriter_write_uint32(bitwriter *thisptr, uint32_t value) {
  bitwriter_write_uint(thisptr, value, 32);
}

void demogobbler_bitwriter_write_varuint32(bitwriter *thisptr, uint32_t value) {
  for (int i = 0; i < 5; i++) {
    uint32_t b = value & 0x7F;
    value >>= 7;
    if (value == 0) {
      bitwriter_write_bits(thisptr, &b, 8);
      break;
    } else {
      b |= 0x80;
      bitwriter_write_bits(thisptr, &b, 8);
    }
  }
}

void demogobbler_bitwriter_free(bitwriter *thisptr) {
  free(thisptr->ptr);
  memset(thisptr, 0, sizeof(bitwriter));
}
