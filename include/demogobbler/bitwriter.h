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

typedef struct dg_bitwriter dg_bitwriter;

struct write_packetentities_args {
  dg_packetentities_data* data;
  const dg_demver_data *version;
  bool is_delta;
};

typedef struct write_packetentities_args write_packetentities_args;

void dg_bitwriter_init(dg_bitwriter *thisptr, uint32_t initial_size_bits);
int64_t dg_bitwriter_get_available_bits(dg_bitwriter *thisptr);
void dg_bitwriter_write_bit(dg_bitwriter *thisptr, bool value);
void dg_bitwriter_write_bitcoord(dg_bitwriter *thisptr, dg_bitcoord value);
void dg_bitwriter_write_bits(dg_bitwriter *thisptr, const void *src, unsigned int bits);
void dg_bitwriter_write_bitstream(dg_bitwriter *thisptr, const dg_bitstream *stream);
void dg_bitwriter_write_bitvector(dg_bitwriter *thisptr, dg_bitangle_vector value);
void dg_bitwriter_write_coordvector(dg_bitwriter *thisptr, dg_bitcoord_vector value);
void dg_bitwriter_write_cstring(dg_bitwriter *thisptr, const char *text);
void dg_bitwriter_write_float(dg_bitwriter *thisptr, float value);
void dg_bitwriter_write_sint(dg_bitwriter *thisptr, int64_t value, unsigned int bits);
void dg_bitwriter_write_sint32(dg_bitwriter *thisptr, int32_t value);
void dg_bitwriter_write_uint(dg_bitwriter *thisptr, uint64_t value, unsigned int bits);
void dg_bitwriter_write_uint32(dg_bitwriter *thisptr, uint32_t value);
void dg_bitwriter_write_varuint32(dg_bitwriter *thisptr, uint32_t value);
void dg_bitwriter_write_bitcellcoord(dg_bitwriter *thisptr, dg_bitcellcoord value, bool is_int,
                                     bool lp, unsigned bits);
void dg_bitwriter_write_bitcoordmp(dg_bitwriter *thisptr, dg_bitcoordmp value, bool is_int, bool lp);
void dg_bitwriter_write_bitnormal(dg_bitwriter *thisptr, dg_bitnormal value);
void dg_bitwriter_write_field_index(dg_bitwriter *thisptr, int32_t new_index, int32_t last_index,
                                    bool new_way);
void dg_bitwriter_write_ubitint(dg_bitwriter *thisptr, uint32_t value);
void dg_bitwriter_write_ubitvar(dg_bitwriter *thisptr, uint32_t value);
void dg_bitwriter_write_packetentities(dg_bitwriter *thisptr, struct write_packetentities_args args);
void dg_bitwriter_write_props(dg_bitwriter *thisptr, const dg_demver_data* demver_data, const dg_ent_update *update);
void dg_bitwriter_free(dg_bitwriter *thisptr);

#ifdef __cplusplus
}
#endif
