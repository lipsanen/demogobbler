#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "demogobbler_bitstream.h"
#include "demogobbler_vector.h"
#include <stdint.h>

#ifdef DEBUG
#define GROUND_TRUTH_CHECK 1
#endif

struct demogobbler_bitwriter {
  uint8_t *ptr;
  int64_t bitsize;
  int64_t bitoffset;
  bool error;
  char* error_message;
#ifdef GROUND_TRUTH_CHECK
  void* truth_data;
  size_t truth_size_bits;
#endif
};

typedef struct demogobbler_bitwriter bitwriter;

void demogobbler_bitwriter_init(bitwriter *thisptr, size_t initial_size_bits);
int64_t demogobbler_bitwriter_get_available_bits(bitwriter *thisptr);
void demogobbler_bitwriter_write_bit(bitwriter *thisptr, bool value);
void demogobbler_bitwriter_write_bitcoord(bitwriter *thisptr, bitcoord value);
void demogobbler_bitwriter_write_bits(bitwriter *thisptr, const void *src, unsigned int bits);
void demogobbler_bitwriter_write_bitstream(bitwriter *thisptr, bitstream *stream);
void demogobbler_bitwriter_write_bitvector(bitwriter *thisptr, bitangle_vector value);
void demogobbler_bitwriter_write_coordvector(bitwriter *thisptr, bitcoord_vector value);
void demogobbler_bitwriter_write_cstring(bitwriter *thisptr, const char *text);
void demogobbler_bitwriter_write_float(bitwriter *thisptr, float value);
void demogobbler_bitwriter_write_sint(bitwriter *thisptr, int64_t value, unsigned int bits);
void demogobbler_bitwriter_write_sint32(bitwriter *thisptr, int32_t value);
void demogobbler_bitwriter_write_uint(bitwriter *thisptr, uint64_t value, unsigned int bits);
void demogobbler_bitwriter_write_uint32(bitwriter *thisptr, uint32_t value);
void demogobbler_bitwriter_write_varuint32(bitwriter *thisptr, uint32_t value);
void demogobbler_bitwriter_write_bitcellcoord(bitwriter* thisptr, demogobbler_bitcellcoord value, bool is_int, bool lp, unsigned bits);
void demogobbler_bitwriter_write_bitcoordmp(bitwriter* thisptr, demogobbler_bitcoordmp value, bool is_int, bool lp);
void demogobbler_bitwriter_write_bitnormal(bitwriter* thisptr, demogobbler_bitnormal value);
void demogobbler_bitwriter_write_field_index(bitwriter* thisptr, int32_t new_index, int32_t last_index, bool new_way);
void demogobbler_bitwriter_write_ubitint(bitwriter *thisptr, uint32_t value);
void demogobbler_bitwriter_free(bitwriter *thisptr);

#define bitwriter_init demogobbler_bitwriter_init
#define bitwriter_get_available_bits demogobbler_bitwriter_get_available_bits
#define bitwriter_write_bit demogobbler_bitwriter_write_bit
#define bitwriter_write_bitcoord demogobbler_bitwriter_write_bitcoord
#define bitwriter_write_bits demogobbler_bitwriter_write_bits
#define bitwriter_write_bitstream demogobbler_bitwriter_write_bitstream
#define bitwriter_write_bitvector demogobbler_bitwriter_write_bitvector
#define bitwriter_write_coordvector demogobbler_bitwriter_write_coordvector
#define bitwriter_write_cstring demogobbler_bitwriter_write_cstring
#define bitwriter_write_float demogobbler_bitwriter_write_float
#define bitwriter_write_sint demogobbler_bitwriter_write_sint
#define bitwriter_write_sint32 demogobbler_bitwriter_write_sint32
#define bitwriter_write_uint demogobbler_bitwriter_write_uint
#define bitwriter_write_uint32 demogobbler_bitwriter_write_uint32
#define bitwriter_write_varuint32 demogobbler_bitwriter_write_varuint32
#define bitwriter_free demogobbler_bitwriter_free

#ifdef __cplusplus
}
#endif
