#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "demogobbler/bitstream.h"
#include "packettypes.h"
#include <stdint.h>

struct dg_bitwriter;

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
  const struct dg_demver_data* demver_data;
  struct dg_alloc_state* permanent_allocator;
  struct dg_alloc_state* allocator;
  struct dg_ent_update* output;
  uint32_t datatable_id;
};

typedef struct dg_instancebaseline_args dg_instancebaseline_args;
struct dg_alloc_state;
struct dg_demver_data;

struct dg_sentry_parse_args {
  dg_bitstream stream;
  struct dg_alloc_state *allocator;
  const struct dg_demver_data *demver_data;
  uint32_t num_updated_entries;
  uint32_t max_entries;
  uint32_t user_data_size_bits;
  uint32_t flags;
  bool user_data_fixed_size;
};

typedef struct dg_sentry_parse_args dg_sentry_parse_args;

struct dg_sentry_value {
  bool entry_bit;
  uint32_t entry_index;
  bool has_name;
  bool reuse_previous_value;
  uint32_t reuse_str_index;
  uint32_t reuse_length;
  char *stored_string; // only contains the string stored, doesnt handle the reuse thing
  dg_bitstream userdata;
  uint32_t userdata_length;
  bool has_user_data;
};

typedef struct dg_sentry_value dg_sentry_value;

struct dg_sentry {
  dg_sentry_value *values;
  uint32_t values_length;
  uint32_t max_entries;
  uint32_t user_data_size_bits;
  uint32_t flags;
  bool user_data_fixed_size;
  bool dictionary_enabled;
};

typedef struct dg_sentry dg_sentry;

struct dg_sentry_write_args {
  struct dg_bitwriter* writer;
  const dg_sentry *input;
};

typedef struct dg_sentry_write_args dg_sentry_write_args;


#ifdef __cplusplus
}
#endif
