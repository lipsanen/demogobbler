#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "demogobbler_bitstream.h"
#include "packettypes.h"
#include <stdint.h>

struct stringtable_entry {
    const char* name;
    bitstream data;
    uint16_t size;
    bool has_data;
};

typedef struct stringtable_entry stringtable_entry;

struct stringtable {
    const char* table_name;
    stringtable_entry* entries;
    stringtable_entry* classes;
    uint16_t entries_count;
    uint16_t classes_count;
    bool has_classes;
};

typedef struct stringtable stringtable;

struct demogobbler_stringtables_parsed {
    demogobbler_stringtables orig;
    bitstream leftover_bits;
    stringtable* tables;
    uint8_t tables_count;
};

typedef struct demogobbler_stringtables_parsed demogobbler_stringtables_parsed;

#ifdef __cplusplus
}
#endif
