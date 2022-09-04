#pragma once

#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif
