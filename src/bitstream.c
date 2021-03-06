#include "demogobbler_bitstream.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>

bitstream demogobbler_bitstream_create(void *data, size_t size) {
  bitstream stream;
  stream.data = data;
  stream.bitsize = size;
  stream.bitoffset = 0;
  stream.overflow = false;
  return stream;
}

void demogobbler_bitstream_advance(bitstream* thisptr, unsigned int bits) {
  unsigned int new_offset = bits + thisptr->bitoffset;

  if(new_offset > thisptr->bitsize) {
    new_offset = thisptr->bitsize;
    thisptr->overflow = true;
  }

  thisptr->bitoffset = new_offset;
}

bitstream demogobbler_bitstream_fork_and_advance(bitstream* stream, unsigned int bits) {
  bitstream output;

  output.bitoffset = stream->bitoffset;
  output.bitsize = stream->bitoffset + bits;
  output.data = stream->data;
  output.overflow = stream->overflow;

  demogobbler_bitstream_advance(stream, bits);

  return output;
}

void demogobbler_bitstream_read_bits(bitstream *thisptr, void *dest, unsigned bits) {
  const uint8_t LOW_MASKS[] = {0x1, 0x3, 0x7, 0xF, 0x1F, 0x3F, 0x7F};
  uint8_t* data = (uint8_t*)thisptr->data + thisptr->bitoffset / 8;

  int size_misalignment = bits & 0x7;
  int src_misalignment = (thisptr->bitoffset & 0x7);
  int mask_index = size_misalignment - 1; // Not valid if byte aligned
  unsigned int bytes;

  unsigned int bits_left = demogobbler_bitstream_bits_left(thisptr);

  if(bits_left < bits) {
    thisptr->overflow = true;
    bits = bits_left;
  }

  if(size_misalignment == 0) {
    bytes = bits / 8;
  }
  else {
    bytes = bits / 8 + 1;
  }

  memcpy(dest, data, bytes);

  if (src_misalignment != 0) {
    if(bits_left <= 8) {
      // If less than 8 bits left we can just shift the bits into place
      // Accessing the next byte would be illegal
      for (size_t i = 0; i < bytes; ++i) {
        uint8_t *current_byte = (uint8_t *)dest + i;
        *current_byte >>= src_misalignment;
      }
    }
    else {
      // We do a little copying
      // Since memory is unaligned, do it two pieces.
      // First just copy the first half with memcpy, then on the 2nd pass do bit magic to move
      // everything into correct places

      for (size_t i = 0; i < bytes; ++i) {
        uint8_t *src = data + 1 + i;
        uint8_t incoming = (*src << (8 - src_misalignment));
        uint8_t *current_byte = (uint8_t *)dest + i;
        *current_byte >>= src_misalignment;
        *current_byte = *current_byte | incoming;
      }
    }
  }

  // mask out the high bits on the last unaligned read
  if (size_misalignment != 0 && bits > 0) {
    uint8_t *last_byte = (uint8_t *)dest + bytes - 1;
    *last_byte =
        *last_byte & LOW_MASKS[mask_index]; 
  } 

  thisptr->bitoffset += bits;
}

uint64_t demogobbler_bitstream_read_uint(bitstream *thisptr, unsigned int bits) {
  //fprintf(stderr, "reading %d : %d, bits %u\n", thisptr->bitoffset, thisptr->bitsize, bits);
  uint64_t value = 0;
  demogobbler_bitstream_read_bits(thisptr, &value, bits);
  return value;
}

int64_t demogobbler_bitstream_read_sint(bitstream *thisptr, unsigned int bits) {
  uint64_t uint = demogobbler_bitstream_read_uint(thisptr, bits);

  return *(int64_t*) &uint;
}

float demogobbler_bitstream_read_float(bitstream* thisptr)
{
  uint32_t uint = demogobbler_bitstream_read_uint32(thisptr);

  return *(float*) &uint;
}

size_t demogobbler_bitstream_read_cstring(bitstream* thisptr, char* dest, size_t max_bytes) {
  unsigned int bytes_left = demogobbler_bitstream_bits_left(thisptr) / 8;
  int src_misalignment = (thisptr->bitoffset & 0x7);
  uint8_t* src = (uint8_t* )thisptr->data + thisptr->bitoffset / 8;
  size_t i = 0;
  
  if(bytes_left > max_bytes) {
    max_bytes = bytes_left;
  }

  if(src_misalignment == 0) {
    for(; i < max_bytes; ++i) {
      dest[i] = src[i];

      if(dest[i] == '\0')
      {
        ++i;
        break;
      }
    }
  }
  else {
    for (; i < max_bytes; ++i) {
      uint8_t *src1 = src + i;
      uint8_t *src2 = src + 1 + i;
      uint8_t inc1 = (*src1 >> src_misalignment);
      uint8_t inc2 = (*src2 << (8 - src_misalignment));
      dest[i] = inc1 | inc2;

      if(dest[i] == '\0')
      {
        ++i;
        break;
      }
    }
  }

  if(i == max_bytes && i != 0) {
    dest[max_bytes - 1] = '\0';
    thisptr->overflow = true;
  }
  else if (i == 0) {
    thisptr->overflow = true;
    dest[0] = '\0';
  }

  thisptr->bitoffset += i * 8;

  return i;
}

vector demogobbler_bitstream_read_bitvector(bitstream* thisptr, unsigned int bits) {
  vector out;
  out.x = bitstream_read_uint(thisptr, bits) * (360.0f / (1 << bits));
  out.y = bitstream_read_uint(thisptr, bits) * (360.0f / (1 << bits));
  out.z = bitstream_read_uint(thisptr, bits) * (360.0f / (1 << bits));
  return out;
}

vector demogobbler_bitstream_read_coordvector(bitstream *thisptr) {
  vector out;
  memset(&out, 0, sizeof(vector));
  bool has_x = bitstream_read_uint(thisptr, 1);
  bool has_y = bitstream_read_uint(thisptr, 1);
  bool has_z = bitstream_read_uint(thisptr, 1);

  if(has_x)
    out.x = demogobbler_bitstream_read_bitcoord(thisptr);
  if(has_y)
    out.y = demogobbler_bitstream_read_bitcoord(thisptr);
  if(has_z)
    out.z = demogobbler_bitstream_read_bitcoord(thisptr);
  return out;
}

float demogobbler_bitstream_read_bitcoord(bitstream* thisptr) { 
  const int COORD_INT_BITS = 14;
  const int COORD_FRAC_BITS = 5;
  const float COORD_RES = 1.0f / (1 << COORD_FRAC_BITS);
  float val = 0;
  bool has_int = demogobbler_bitstream_read_uint(thisptr, 1);
  bool has_frac = demogobbler_bitstream_read_uint(thisptr, 1);

  if(has_int | has_frac) {
    bool sign = demogobbler_bitstream_read_uint(thisptr, 1);
    if(has_int)
      val += demogobbler_bitstream_read_uint(thisptr, COORD_INT_BITS) + 1;
    if(has_frac)
      val += demogobbler_bitstream_read_uint(thisptr, COORD_FRAC_BITS) * COORD_RES;
    if(sign)
      val = -val;
  }

  return val;
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
