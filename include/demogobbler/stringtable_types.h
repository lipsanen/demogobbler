#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "demogobbler/bitstream.h"
#include "packettypes.h"
#include <stdint.h>

struct dg_stringtable_entry {
  const char *name;
  dg_bitstream data;
  uint16_t size;
  bool has_data;
};

typedef struct dg_stringtable_entry dg_stringtable_entry;

struct dg_stringtable {
  const char *table_name;
  dg_stringtable_entry *entries;
  dg_stringtable_entry *classes;
  uint16_t entries_count;
  uint16_t classes_count;
  bool has_classes;
};

typedef struct dg_stringtable dg_stringtable;

struct dg_stringtables_parsed {
  dg_stringtables orig;
  dg_bitstream leftover_bits;
  dg_stringtable *tables;
  uint8_t tables_count;
};

typedef struct dg_stringtables_parsed dg_stringtables_parsed;

struct dg_bitstream;
struct estate;
struct dg_demver_data;
struct dg_alloc_state;
struct dg_ent_update;

struct dg_instancebaseline_args {
  const struct dg_bitstream* stream;
  struct estate* estate_ptr;
  struct dg_demver_data* demver_data;
  struct dg_alloc_state* permanent_allocator;
  struct dg_alloc_state* allocator;
  struct dg_ent_update* output;
  uint32_t datatable_id;
};

typedef struct dg_instancebaseline_args dg_instancebaseline_args;

#ifdef __cplusplus
}
#endif
