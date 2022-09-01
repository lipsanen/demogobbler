#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "demogobbler_datatables.h"
#include "demogobbler_entity_types.h"

typedef struct {
  const char* str;
  size_t value;
} hashtable_entry;

typedef struct {
  hashtable_entry* arr;
  size_t max_items;
  size_t item_count;
} hashtable;

hashtable demogobbler_hashtable_create(size_t array_size);
hashtable_entry demogobbler_hashtable_get(hashtable* thisptr, const char* str);
bool demogobbler_hashtable_insert(hashtable* thisptr, hashtable_entry entry);
void demogobbler_hashtable_free(hashtable* thisptr);

typedef struct {
  demogobbler_sendprop* exclude_prop;
} prop_exclude_entry;

typedef struct {
  prop_exclude_entry* arr;
  size_t max_items;
  size_t item_count;
} prop_exclude_set;

prop_exclude_set demogobbler_pes_create(size_t array_size);
bool demogobbler_pes_has(prop_exclude_set* thisptr, demogobbler_sendtable* table, demogobbler_sendprop* prop);
bool demogobbler_pes_insert(prop_exclude_set* thisptr, demogobbler_sendprop* entry);
void demogobbler_pes_free(prop_exclude_set* thisptr);
void demogobbler_pes_clear(prop_exclude_set* thisptr);
