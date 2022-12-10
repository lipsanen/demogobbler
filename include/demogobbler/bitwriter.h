#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "demogobbler/bitstream.h"
#include "demogobbler/datatable_types.h"
#include "demogobbler/entity_types.h"
#include "demogobbler/parser_types.h"
#include "demogobbler/vector.h"
#include <stdint.h>

#ifdef DEBUG
#define GROUND_TRUTH_CHECK 1
#endif

struct dg_bitwriter {
  uint8_t *ptr;
  uint32_t bitsize;
  uint32_t bitoffset;
  bool error;
  char *error_message;
#ifdef GROUND_TRUTH_CHECK
  void *truth_data;
  uint32_t truth_data_offset;
  uint32_t truth_size_bits;
#endif
};

typedef struct dg_bitwriter bitwriter;

struct write_packetentities_args {
  dg_packetentities_data data;
  const dg_demver_data *version;
  bool is_delta;
};

typedef struct write_packetentities_args write_packetentities_args;

void dg_bitwriter_init(bitwriter *thisptr, uint32_t initial_size_bits);
int64_t dg_bitwriter_get_available_bits(bitwriter *thisptr);
void dg_bitwriter_write_bit(bitwriter *thisptr, bool value);
void dg_bitwriter_write_bitcoord(bitwriter *thisptr, dg_bitcoord value);
void dg_bitwriter_write_bits(bitwriter *thisptr, const void *src, unsigned int bits);
void dg_bitwriter_write_bitstream(bitwriter *thisptr, dg_bitstream *stream);
void dg_bitwriter_write_bitvector(bitwriter *thisptr, dg_bitangle_vector value);
void dg_bitwriter_write_coordvector(bitwriter *thisptr, dg_bitcoord_vector value);
void dg_bitwriter_write_cstring(bitwriter *thisptr, const char *text);
void dg_bitwriter_write_float(bitwriter *thisptr, float value);
void dg_bitwriter_write_sint(bitwriter *thisptr, int64_t value, unsigned int bits);
void dg_bitwriter_write_sint32(bitwriter *thisptr, int32_t value);
void dg_bitwriter_write_uint(bitwriter *thisptr, uint64_t value, unsigned int bits);
void dg_bitwriter_write_uint32(bitwriter *thisptr, uint32_t value);
void dg_bitwriter_write_varuint32(bitwriter *thisptr, uint32_t value);
void dg_bitwriter_write_bitcellcoord(bitwriter *thisptr, dg_bitcellcoord value, bool is_int,
                                     bool lp, unsigned bits);
void dg_bitwriter_write_bitcoordmp(bitwriter *thisptr, dg_bitcoordmp value, bool is_int, bool lp);
void dg_bitwriter_write_bitnormal(bitwriter *thisptr, dg_bitnormal value);
void dg_bitwriter_write_field_index(bitwriter *thisptr, int32_t new_index, int32_t last_index,
                                    bool new_way);
void dg_bitwriter_write_ubitint(bitwriter *thisptr, uint32_t value);
void dg_bitwriter_write_ubitvar(bitwriter *thisptr, uint32_t value);
void dg_bitwriter_write_packetentities(bitwriter *thisptr, struct write_packetentities_args args);
void dg_bitwriter_free(bitwriter *thisptr);

#define bitwriter_init dg_bitwriter_init
#define bitwriter_get_available_bits dg_bitwriter_get_available_bits
#define bitwriter_write_bit dg_bitwriter_write_bit
#define bitwriter_write_bitcoord dg_bitwriter_write_bitcoord
#define bitwriter_write_bits dg_bitwriter_write_bits
#define bitwriter_write_bitstream dg_bitwriter_write_bitstream
#define bitwriter_write_bitvector dg_bitwriter_write_bitvector
#define bitwriter_write_coordvector dg_bitwriter_write_coordvector
#define bitwriter_write_cstring dg_bitwriter_write_cstring
#define bitwriter_write_float dg_bitwriter_write_float
#define bitwriter_write_sint dg_bitwriter_write_sint
#define bitwriter_write_sint32 dg_bitwriter_write_sint32
#define bitwriter_write_uint dg_bitwriter_write_uint
#define bitwriter_write_uint32 dg_bitwriter_write_uint32
#define bitwriter_write_varuint32 dg_bitwriter_write_varuint32
#define bitwriter_free dg_bitwriter_free

#ifdef __cplusplus
}
#endif
