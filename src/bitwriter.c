#include "demogobbler/bitwriter.h"
#include "demogobbler/bitstream.h"
#include "demogobbler/utils.h"
#include <string.h>

#ifdef _MSC_VER
#define NO_ASAN
#else
#define NO_ASAN __attribute__((no_sanitize("address")))
#endif

void dg_bitwriter_init(dg_bitwriter *thisptr, uint32_t initial_size_bits) {
  memset(thisptr, 0, sizeof(*thisptr));
  uint32_t bytes = initial_size_bits / 8;
  if((initial_size_bits & 0x7) != 0)
    ++bytes;
  thisptr->ptr = malloc(bytes);
  thisptr->bitoffset = 0;
  thisptr->bitsize = bytes * 8;
}

#ifdef GROUND_TRUTH_CHECK
typedef struct {
  dg_bitstream truth;
  dg_bitstream written;
  bool has_truth;
} truth_state;

static truth_state get_truth_state(dg_bitwriter *writer, unsigned int bits) {
  truth_state out;
  memset(&out, 0, sizeof(truth_state));
  if (writer->truth_data) {
    out.has_truth = true;
    out.truth = dg_bitstream_create(writer->truth_data, writer->truth_size_bits);
    out.written = dg_bitstream_create(writer->ptr, writer->bitsize);
    dg_bitstream_advance(&out.truth, writer->bitoffset - bits + writer->truth_data_offset);
    dg_bitstream_advance(&out.written, writer->bitoffset - bits);
    out.truth.bitsize = out.truth.bitoffset + bits;
    out.written.bitsize = out.written.bitoffset + bits;
  }

  return out;
}

static void ground_truth_check(dg_bitwriter *thisptr, unsigned int bits) {
  truth_state state = get_truth_state(thisptr, bits);
  if (state.has_truth) {
    unsigned int bits_left = dg_bitstream_bits_left(&state.truth);
    while (bits_left > 0) {
      unsigned int bits = MIN(bits_left, 64);

      uint64_t truth_value = dg_bitstream_read_uint(&state.truth, bits);
      uint64_t write_value = dg_bitstream_read_uint(&state.written, bits);

      if (truth_value != write_value) {
        thisptr->error = true;
        thisptr->error_message = "Did not match with ground truth.";
        break;
      }

      bits_left = MAX(dg_bitstream_bits_left(&state.truth), dg_bitstream_bits_left(&state.written));
    }
  }
}
#endif

int64_t dg_bitwriter_get_available_bits(dg_bitwriter *thisptr) {
  if (thisptr->bitoffset > thisptr->bitsize) {
    return 0;
  } else {
    return thisptr->bitsize - thisptr->bitoffset;
  }
}

static void bitwriter_allocate_space_if_needed(dg_bitwriter *thisptr, unsigned int bits_wanted) {
  if (dg_bitwriter_get_available_bits(thisptr) < bits_wanted) {
    uint32_t bytes = (thisptr->bitsize + bits_wanted) / 8;
    uint32_t current_bytes = thisptr->bitsize / 8;
    if(bits_wanted % 8 != 0)
      ++bytes;
    bytes = MAX(current_bytes * 2, bytes);
    thisptr->ptr = realloc(thisptr->ptr, bytes);
    thisptr->bitsize = bytes * 8;
  }
}

#define CHECK_SIZE() bitwriter_allocate_space_if_needed(thisptr, bits)

void NO_ASAN dg_bitwriter_write_bit(dg_bitwriter *thisptr, bool value) {
  bitwriter_allocate_space_if_needed(thisptr, 1);
  uint8_t MASKS[] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
  int index = thisptr->bitoffset & 0x7;
  uint8_t *dest = thisptr->ptr + thisptr->bitoffset / 8;

  // If value does not match then flip the bit
  *dest &= ~(MASKS[index]);
  if(value)
    *dest |= MASKS[index];

  thisptr->bitoffset += 1;
#ifdef GROUND_TRUTH_CHECK
  ground_truth_check(thisptr, 1);
#endif
}

void NO_ASAN dg_bitwriter_write_bits(dg_bitwriter *thisptr, const void *_src, unsigned int bits) {
  CHECK_SIZE();
  unsigned int src_offset = 0;
#ifdef GROUND_TRUTH_CHECK
  unsigned int requested_bits = bits;
#endif
  while (bits > 0) {
    int dest_byte_offset = thisptr->bitoffset & 0x7;
    int src_byte_offset = src_offset & 0x7;
    int bits_written = MIN(8 - dest_byte_offset, 8 - src_byte_offset);
    bits_written = MIN(bits, bits_written);
    const uint8_t *src = (uint8_t *)_src + src_offset / 8;
    uint8_t *dest = thisptr->ptr + thisptr->bitoffset / 8;

    // Get rid of the extra bits at the end of the destination value
    uint8_t cur_value = 0;
    if(dest_byte_offset != 0) {
      cur_value = *dest << (8 - dest_byte_offset);
      cur_value >>= (8 - dest_byte_offset);
    }

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

#ifdef GROUND_TRUTH_CHECK
  ground_truth_check(thisptr, requested_bits);
#endif
}

void dg_bitwriter_write_bitcoord(dg_bitwriter *thisptr, dg_bitcoord coord) {
  dg_bitwriter_write_bit(thisptr, coord.has_int);
  dg_bitwriter_write_bit(thisptr, coord.has_frac);

  if (coord.has_int || coord.has_frac) {
    dg_bitwriter_write_bit(thisptr, coord.sign);

    if (coord.has_int) {
      dg_bitwriter_write_uint(thisptr, coord.int_value, COORD_INTEGER_BITS);
    }

    if (coord.has_frac) {
      dg_bitwriter_write_uint(thisptr, coord.frac_value, COORD_FRACTIONAL_BITS);
    }
  }
}

void dg_bitwriter_write_bitstream(dg_bitwriter *thisptr, const dg_bitstream *_stream) {
  dg_bitstream copy = *_stream;
  while (dg_bitstream_bits_left(&copy) > 32) {
    uint32_t value = dg_bitstream_read_uint32(&copy);
    dg_bitwriter_write_bits(thisptr, &value, 32);
  }

  unsigned int bits = dg_bitstream_bits_left(&copy);

  if (bits > 0) {
    uint32_t remainder = dg_bitstream_read_uint(&copy, bits);
    dg_bitwriter_write_bits(thisptr, &remainder, bits);
  }
}

void dg_bitwriter_write_bitangle(dg_bitwriter *thisptr, float value, unsigned int bits) {
  float shift = 1 << (bits - 1);
  unsigned int mask = shift - 1;

  float ratio = (value / 360.0f);
  int d = (int)(ratio * shift);
  d &= mask;

  dg_bitwriter_write_uint(thisptr, (unsigned int)d, bits);
}

void dg_bitwriter_write_bitvector(dg_bitwriter *thisptr, dg_bitangle_vector value) {
  dg_bitwriter_write_uint(thisptr, value.x, value.bits);
  dg_bitwriter_write_uint(thisptr, value.y, value.bits);
  dg_bitwriter_write_uint(thisptr, value.z, value.bits);
}

void dg_bitwriter_write_coordvector(dg_bitwriter *thisptr, dg_bitcoord_vector vec) {
  dg_bitwriter_write_bit(thisptr, vec.x.exists);
  dg_bitwriter_write_bit(thisptr, vec.y.exists);
  dg_bitwriter_write_bit(thisptr, vec.z.exists);

  if (vec.x.exists)
    dg_bitwriter_write_bitcoord(thisptr, vec.x);
  if (vec.y.exists)
    dg_bitwriter_write_bitcoord(thisptr, vec.y);
  if (vec.z.exists)
    dg_bitwriter_write_bitcoord(thisptr, vec.z);
}

void dg_bitwriter_write_cstring(dg_bitwriter *thisptr, const char *text) {
  if (text == NULL) {
    dg_bitwriter_write_uint(thisptr, 0, 8);
  } else {
    dg_bitwriter_write_bits(thisptr, (void *)text, (strlen(text) + 1) * 8);
  }
}

void dg_bitwriter_write_float(dg_bitwriter *thisptr, float value) {
  dg_bitwriter_write_bits(thisptr, &value, 32);
}

void dg_bitwriter_write_sint(dg_bitwriter *thisptr, int64_t value, unsigned int bits) {
  dg_bitwriter_write_uint(thisptr, value, bits);
}

void dg_bitwriter_write_sint32(dg_bitwriter *thisptr, int32_t value) {
  dg_bitwriter_write_sint(thisptr, value, 32);
}

void dg_bitwriter_write_uint(dg_bitwriter *thisptr, uint64_t value, unsigned int bits) {
  dg_bitwriter_write_bits(thisptr, &value, bits);
}

void dg_bitwriter_write_uint32(dg_bitwriter *thisptr, uint32_t value) {
  dg_bitwriter_write_uint(thisptr, value, 32);
}

void dg_bitwriter_write_varuint32(dg_bitwriter *thisptr, uint32_t value) {
  for (int i = 0; i < 5; i++) {
    uint32_t b = value & 0x7F;
    value >>= 7;
    if (value == 0) {
      dg_bitwriter_write_bits(thisptr, &b, 8);
      break;
    } else {
      b |= 0x80;
      dg_bitwriter_write_bits(thisptr, &b, 8);
    }
  }
}

void dg_bitwriter_write_bitcellcoord(dg_bitwriter *thisptr, dg_bitcellcoord value, bool is_int,
                                     bool lp, unsigned bits) {
  dg_bitwriter_write_uint(thisptr, value.int_val, bits);

  if (!is_int) {
    if (lp) {
      dg_bitwriter_write_uint(thisptr, value.fract_val, FRAC_BITS_LP);
    } else {
      dg_bitwriter_write_uint(thisptr, value.fract_val, FRAC_BITS);
    }
  }
}

void dg_bitwriter_write_bitcoordmp(dg_bitwriter *thisptr, dg_bitcoordmp value, bool is_int, bool lp) {
  dg_bitwriter_write_bit(thisptr, value.inbounds);

  if (is_int) {
    dg_bitwriter_write_bit(thisptr, value.int_has_val);
    if (value.int_has_val) {
      dg_bitwriter_write_bit(thisptr, value.sign);

      if (value.inbounds) {
        dg_bitwriter_write_uint(thisptr, value.int_val, COORD_INT_BITS_MP);
      } else {
        dg_bitwriter_write_uint(thisptr, value.int_val, COORD_INTEGER_BITS);
      }
    }
  } else {
    dg_bitwriter_write_bit(thisptr, value.int_has_val);
    dg_bitwriter_write_bit(thisptr, value.sign);

    if (value.int_has_val) {
      if (value.inbounds) {
        dg_bitwriter_write_uint(thisptr, value.int_val, COORD_INT_BITS_MP);
      } else {
        dg_bitwriter_write_uint(thisptr, value.int_val, COORD_INTEGER_BITS);
      }
    }

    if (lp) {
      dg_bitwriter_write_uint(thisptr, value.frac_val, FRAC_BITS_LP);
    } else {
      dg_bitwriter_write_uint(thisptr, value.frac_val, FRAC_BITS);
    }
  }
}

void dg_bitwriter_write_bitnormal(dg_bitwriter *thisptr, dg_bitnormal value) {
  const size_t frac_bits = 11;
  dg_bitwriter_write_bit(thisptr, value.sign);
  dg_bitwriter_write_uint(thisptr, value.frac, frac_bits);
}

void dg_bitwriter_write_field_index(dg_bitwriter *thisptr, int32_t new_index, int32_t last_index,
                                    bool new_way) {
  int32_t diff;

  if (new_index == -1) {
    diff = 0xFFF;
  } else {
    diff = new_index - last_index - 1;
  }

  if (new_way) {
    if (diff == 0) {
      dg_bitwriter_write_bit(thisptr, true);
      return;
    } else {
      dg_bitwriter_write_bit(thisptr, false);
    }
  }

  if (new_way && diff < 8) {
    dg_bitwriter_write_bit(thisptr, true);
    dg_bitwriter_write_uint(thisptr, diff, 3);
    return;
  } else {
    if (new_way) {
      dg_bitwriter_write_bit(thisptr, false);
    }

    const size_t case_0_max = (1 << 5) - 1;
    const size_t case_1_max = (1 << 7) - 1;
    const size_t case_2_max = (1 << 9) - 1;

    dg_bitwriter_write_uint(thisptr, diff & case_0_max, 5);
    size_t bits_to_write;

    if (diff <= case_0_max) {
      bits_to_write = 0;
      dg_bitwriter_write_uint(thisptr, 0, 2);
    } else if (diff <= case_1_max) {
      bits_to_write = 2;
      dg_bitwriter_write_uint(thisptr, 1, 2);
    } else if (diff <= case_2_max) {
      bits_to_write = 4;
      dg_bitwriter_write_uint(thisptr, 2, 2);
    } else {
      bits_to_write = 7;
      dg_bitwriter_write_uint(thisptr, 3, 2);
    }

    if (bits_to_write > 0) {
      diff >>= 5;
      dg_bitwriter_write_uint(thisptr, diff, bits_to_write);
    }
  }
}

void dg_bitwriter_write_ubitint(dg_bitwriter *thisptr, uint32_t value) {
  const uint32_t case_0_max = (1 << 4) - 1;
  const uint32_t case_1_max = (1 << 8) - 1;
  const uint32_t case_2_max = (1 << 12) - 1;

  size_t bits_to_write;
  uint32_t val1 = value & case_0_max;
  dg_bitwriter_write_uint(thisptr, val1, 4);
  uint32_t sel;

  if (value <= case_0_max) {
    bits_to_write = 0;
    sel = 0;
  } else if (value <= case_1_max) {
    bits_to_write = 4;
    sel = 1;
  } else if (value <= case_2_max) {
    bits_to_write = 8;
    sel = 2;
  } else {
    bits_to_write = 28;
    sel = 3;
  }

  dg_bitwriter_write_uint(thisptr, sel, 2);

  if (bits_to_write > 0) {
    value >>= 4;
    dg_bitwriter_write_uint(thisptr, value, bits_to_write);
  }
}

void dg_bitwriter_write_ubitvar(dg_bitwriter *thisptr, uint32_t value) {
  const uint32_t case_0_max = (1 << 4) - 1;
  const uint32_t case_1_max = (1 << 8) - 1;
  const uint32_t case_2_max = (1 << 12) - 1;

  if (value <= case_0_max) {
    dg_bitwriter_write_uint(thisptr, 0, 2);
    dg_bitwriter_write_uint(thisptr, value, 4);
  } else if (value <= case_1_max) {
    dg_bitwriter_write_uint(thisptr, 1, 2);
    dg_bitwriter_write_uint(thisptr, value, 8);
  } else if (value <= case_2_max) {
    dg_bitwriter_write_uint(thisptr, 2, 2);
    dg_bitwriter_write_uint(thisptr, value, 12);
  } else {
    dg_bitwriter_write_uint(thisptr, 3, 2);
    dg_bitwriter_write_uint(thisptr, value, 32);
  }
}

void dg_bitwriter_free(dg_bitwriter *thisptr) {
  free(thisptr->ptr);
  memset(thisptr, 0, sizeof(dg_bitwriter));
}
