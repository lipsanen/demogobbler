#include "demogobbler/hashtable.h"
#define XXH_INLINE_ALL
#include "xxhash.h"
#include <stdlib.h>
#include <string.h>

static uint32_t get_hash(const char *str) {
  // The length here is computed unnecessarily, could grab it when we parse the string
  return XXH32(str, strlen(str), 0);
}

dg_hashtable dg_hashtable_create(size_t max_items) {
  dg_hashtable table;
  memset(&table, 0, sizeof(table));

  table.max_items = 1; // User power of two sizes;

  while (table.max_items - 1 < max_items)
    table.max_items <<= 1;

  const size_t array_size_bytes = table.max_items * sizeof(dg_hashtable_entry);
  table.arr = malloc(array_size_bytes);
  memset(table.arr, 0, array_size_bytes);

  return table;
}

dg_hashtable_entry dg_hashtable_get(dg_hashtable *thisptr, const char *str) {
  uint32_t hash = get_hash(str);
  size_t i = hash & (thisptr->max_items - 1);
  while (thisptr->arr[i].str) {
    if (strcmp(thisptr->arr[i].str, str) == 0) {
      return thisptr->arr[i];
    }
    i += 1;
    if (i == thisptr->max_items)
      i = 0;
  }

  dg_hashtable_entry not_found;
  memset(&not_found, 0, sizeof(not_found));
  return not_found;
}

bool dg_hashtable_insert(dg_hashtable *thisptr, dg_hashtable_entry entry) {
  if (thisptr->item_count == thisptr->max_items - 1) {
    return false; // Hash table is not resizable
  }

  ++thisptr->item_count;
  uint32_t hash = get_hash(entry.str);
  size_t initial_index = hash & (thisptr->max_items - 1);
  size_t i = initial_index;
  while (thisptr->arr[i].str) {
    if (strcmp(thisptr->arr[i].str, entry.str) == 0) {
      return false;
    }
    i += 1;
    if (i == thisptr->max_items)
      i = 0;
  }

  thisptr->arr[i] = entry;

  return true;
}

void dg_hashtable_clear(dg_hashtable *thisptr) {
  const size_t array_size_bytes = thisptr->max_items * sizeof(dg_hashtable_entry);
  thisptr->item_count = 0;
  memset(thisptr->arr, 0, array_size_bytes);
}

void dg_hashtable_free(dg_hashtable *thisptr) {
  free(thisptr->arr);
  thisptr->arr = NULL;
}

dg_pes dg_pes_create(size_t max_items) {
  dg_pes set;
  memset(&set, 0, sizeof(set));
  set.max_items = 1;

  while (set.max_items - 1 < max_items)
    set.max_items <<= 1;

  const size_t array_size_bytes = set.max_items * sizeof(dg_pes_entry);
  set.arr = malloc(array_size_bytes);
  memset(set.arr, 0, array_size_bytes);

  return set;
}

static uint32_t get_hash_prop(dg_sendprop *prop) {
  // Only hash the prop name, dt name will likely match
  return XXH32(prop->name, strlen(prop->name), 0);
}

static bool match(dg_sendprop *exclude, dg_sendtable *table, dg_sendprop *prop) {
  return strcmp(exclude->exclude_name, table->name) == 0 && strcmp(exclude->name, prop->name) == 0;
}

static bool match_excludes(dg_sendprop *exclude1, dg_sendprop *exclude2) {
  return strcmp(exclude1->name, exclude2->name) == 0 &&
         strcmp(exclude1->dtname, exclude2->dtname) == 0;
}

bool dg_pes_has(dg_pes *thisptr, dg_sendtable *table, dg_sendprop *prop) {
  if (thisptr->item_count == 0) {
    return false;
  }

  uint32_t hash = get_hash_prop(prop);
  size_t i = hash & (thisptr->max_items - 1);
  while (thisptr->arr[i].exclude_prop) {
    if (match(thisptr->arr[i].exclude_prop, table, prop)) {
      return true;
    }
    i += 1;
    if (i == thisptr->max_items)
      i = 0;
  }

  return false;
}

bool dg_pes_insert(dg_pes *thisptr, dg_sendprop *prop) {
  if (thisptr->item_count == thisptr->max_items - 1) {
    return false; // Hash table is not resizable
  }

  dg_pes_entry entry;
  entry.exclude_prop = prop;

  ++thisptr->item_count;
  uint32_t hash = get_hash_prop(entry.exclude_prop);
  size_t initial_index = hash & (thisptr->max_items - 1);
  size_t i = initial_index;
  while (thisptr->arr[i].exclude_prop) {
    if (match_excludes(thisptr->arr[i].exclude_prop, entry.exclude_prop)) {
      return false;
    }
    i += 1;
    if (i == thisptr->max_items)
      i = 0;
  }

  thisptr->arr[i] = entry;

  return true;
}

void dg_pes_free(dg_pes *thisptr) {
  free(thisptr->arr);
  thisptr->arr = NULL;
}

void dg_pes_clear(dg_pes *thisptr) {
  const size_t array_size_bytes = thisptr->max_items * sizeof(dg_pes_entry);
  memset(thisptr->arr, 0, array_size_bytes);
}
