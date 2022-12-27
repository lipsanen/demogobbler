#pragma once

#include "demogobbler/datatable_types.h"
#include "demogobbler/entity_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

dg_hashtable dg_hashtable_create(size_t array_size);
dg_hashtable_entry dg_hashtable_get(dg_hashtable *thisptr, const char *str);
bool dg_hashtable_insert(dg_hashtable *thisptr, dg_hashtable_entry entry);
void dg_hashtable_clear(dg_hashtable *thisptr);
void dg_hashtable_free(dg_hashtable *thisptr);

dg_pes dg_pes_create(size_t array_size);
bool dg_pes_has(dg_pes *thisptr, dg_sendtable *table, dg_sendprop *prop);
bool dg_pes_insert(dg_pes *thisptr, dg_sendprop *entry);
void dg_pes_free(dg_pes *thisptr);
void dg_pes_clear(dg_pes *thisptr);
