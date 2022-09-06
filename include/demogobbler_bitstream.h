#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct demogobbler_bitstream {
  void *data;
  uint32_t bitsize;
  uint32_t bitoffset;
  uint64_t buffered;
  uint8_t* buffered_address;
  uint32_t buffered_bytes_read;
  bool overflow;
};

typedef struct demogobbler_bitstream bitstream;

#ifdef __cplusplus
}
#endif
