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

#ifdef __cplusplus
}
#endif
