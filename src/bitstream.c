#include "demogobbler_bitstream.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>

// This rounds up
static uint64_t get_size_in_bytes(uint64_t bits) {
  if (bits & 0x7) {
    return bits / 8 + 1;
  }
  else {
    return bits / 8;
  }
}

static void fetch_ubit(bitstream *thisptr) {
  if(thisptr->bitoffset >= thisptr->bitsize) {
    thisptr->buffered = 0;
    thisptr->buffered_bits = 64;
    return;
  }

  uint64_t val = 0;
  uint8_t* src = (uint8_t*)thisptr->data + thisptr->bitoffset / 8;
  uint64_t end_byte = get_size_in_bytes(thisptr->bitsize);
  uint64_t bytes_to_read = end_byte - thisptr->bitoffset / 8;
  bytes_to_read = MIN(bytes_to_read, 8);

  for(int i=0; i < bytes_to_read; ++i) {
    val |= ((uint64_t)src[i]) << (8 * i);
  }

  thisptr->buffered = val;
  thisptr->buffered_bits = bytes_to_read * 8;

  int64_t byte_offset = thisptr->bitoffset & 0x7;

  if(byte_offset != 0) {
    thisptr->buffered >>= byte_offset;
    thisptr->buffered_bits -= byte_offset;
  }
}

bitstream demogobbler_bitstream_create(void *data, size_t size) {
  bitstream stream;
  stream.data = data;
  stream.bitsize = size;
  stream.bitoffset = 0;
  stream.overflow = false;
  fetch_ubit(&stream);
  return stream;
}

void demogobbler_bitstream_advance(bitstream* thisptr, unsigned int bits) {
  unsigned int new_offset = bits + thisptr->bitoffset;

  if(new_offset > thisptr->bitsize) {
    new_offset = thisptr->bitsize;
    thisptr->overflow = true;
  }

  thisptr->bitoffset = new_offset;
  fetch_ubit(thisptr);
}

bitstream demogobbler_bitstream_fork_and_advance(bitstream* stream, unsigned int bits) {
  bitstream output;

  output.bitoffset = stream->bitoffset;
  output.bitsize = stream->bitoffset + bits;
  output.data = stream->data;
  output.overflow = stream->overflow;
  fetch_ubit(&output);
  demogobbler_bitstream_advance(stream, bits);

  return output;
}

static uint64_t read_ubit(bitstream *thisptr, unsigned requested_bits) {
  uint64_t rval;

  unsigned int bits_left = requested_bits;

  if(thisptr->buffered_bits == 0) {
      fetch_ubit(thisptr);
  }

  if(thisptr->buffered_bits >= bits_left) {
    rval = thisptr->buffered << (64 - bits_left);
    rval >>= (64 - bits_left);
    thisptr->buffered >>= bits_left;
    thisptr->buffered_bits -= bits_left;
    thisptr->bitoffset += bits_left;
  }
  else {
    unsigned int first_read = thisptr->buffered_bits;
    rval = thisptr->buffered;
    thisptr->bitoffset += thisptr->buffered_bits;
    bits_left -= thisptr->buffered_bits;

    fetch_ubit(thisptr);

    uint64_t temp = thisptr->buffered << (64 - bits_left);
    temp >>= (64 - bits_left - first_read);

    rval |= temp;

    thisptr->bitoffset += bits_left;
    thisptr->buffered >>= bits_left;
    thisptr->buffered_bits -= bits_left;
  }
  
  if(thisptr->bitoffset > thisptr->bitsize) {
    thisptr->bitoffset = thisptr->bitsize;
    thisptr->overflow = true;
  }

  return rval;
}

void demogobbler_bitstream_read_bits(bitstream *thisptr, void *_dest, unsigned _bits) {
  uint8_t* dest = (uint8_t*)_dest;

  for(int i=0; _bits > 0; ++i) {
    unsigned int bits_to_read = MIN(8, _bits);
    dest[i] = read_ubit(thisptr, bits_to_read);
    _bits -= bits_to_read;
  }
}

bool demogobbler_bitstream_read_bit(bitstream* thisptr) {
  uint8_t MASKS[] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};

  uint8_t* ptr = (uint8_t*)thisptr->data + thisptr->bitoffset / 8;
  int offset_alignment = thisptr->bitoffset & 0x7;
  bitstream_advance(thisptr, 1);

  return *ptr & MASKS[offset_alignment];
}

uint64_t demogobbler_bitstream_read_uint(bitstream *thisptr, unsigned int bits) {
  //fprintf(stderr, "reading %d : %d, bits %u\n", thisptr->bitoffset, thisptr->bitsize, bits);
  uint64_t value = 0;
  // If we have more than 40 bits left, then grab the next 40 bits
  if(demogobbler_bitstream_bits_left(thisptr) >= 40 && bits <= 32) {
    uint8_t* ptr = (uint8_t*)thisptr->data + thisptr->bitoffset / 8;
    value = ((uint64_t)*(ptr + 4)) << 32 | ((uint64_t)*(ptr + 3)) << 24 |
    ((uint64_t)*(ptr + 2)) << 16 | ((uint64_t)*(ptr + 1)) << 8 | ((uint64_t)*ptr);
    value >>= thisptr->bitoffset & 0x7;
    value <<= (64 - bits);
    value >>= (64 - bits);
    bitstream_advance(thisptr, bits);
  }
  else {
    demogobbler_bitstream_read_bits(thisptr, &value, bits);
  }
  return value;
}

int64_t demogobbler_bitstream_read_sint(bitstream *thisptr, unsigned int bits) {
  int64_t n_ret = demogobbler_bitstream_read_uint(thisptr, bits);
	// Sign magic
	return ( n_ret << ( 64 - bits ) ) >> ( 64 - bits );
}

float demogobbler_bitstream_read_float(bitstream* thisptr)
{
  uint32_t uint = demogobbler_bitstream_read_uint32(thisptr);

  return *(float*) &uint;
}

size_t demogobbler_bitstream_read_cstring(bitstream* thisptr, char* dest, size_t max_bytes) {
  size_t i;
  for(i=0; i < max_bytes; ++i) {
    uint64_t value = read_ubit(thisptr, 8);
    char c = *(char*)&value;
    dest[i] = c;

    if(value == 0)
    {
      ++i;
      break;
    }
  }

  return i;
}

bitangle_vector demogobbler_bitstream_read_bitvector(bitstream* thisptr, unsigned int bits) {
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

  if(out.x.exists)
    out.x = demogobbler_bitstream_read_bitcoord(thisptr);
  if(out.y.exists)
    out.y = demogobbler_bitstream_read_bitcoord(thisptr);
  if(out.z.exists)
    out.z = demogobbler_bitstream_read_bitcoord(thisptr);
  return out;
}

bitcoord demogobbler_bitstream_read_bitcoord(bitstream* thisptr) {
  bitcoord out;
  memset(&out, 0, sizeof(out));
  out.exists = true;
  out.has_int = demogobbler_bitstream_read_uint(thisptr, 1);
  out.has_frac = demogobbler_bitstream_read_uint(thisptr, 1);

  if(out.has_int || out.has_frac) {
    out.sign = demogobbler_bitstream_read_uint(thisptr, 1);
    if(out.has_int)
      out.int_value = demogobbler_bitstream_read_uint(thisptr, COORD_INTEGER_BITS);
    if(out.has_frac)
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
