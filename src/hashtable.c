#include "hashtable.h"
#include "demogobbler_datatables.h"
#define XXH_INLINE_ALL
#include "xxhash.h"
#include <stdlib.h>
#include <string.h>

static uint32_t get_hash(const char* str) {
  // The length here is computed unnecessarily, could grab it when we parse the string
  return XXH32(str, strlen(str), 0);
}

hashtable demogobbler_hashtable_create(size_t max_items) {
  hashtable table;
  memset(&table, 0, sizeof(table));

  table.max_items = 1; // User power of two sizes;
  
  while(table.max_items - 1 < max_items)
    table.max_items <<= 1;

  const size_t array_size_bytes = table.max_items * sizeof(hashtable_entry);
  table.arr = malloc(array_size_bytes);
  memset(table.arr, 0, array_size_bytes);

  return table;
}

hashtable_entry demogobbler_hashtable_get(hashtable* thisptr, const char* str) {
  uint32_t hash = get_hash(str);
  size_t i = hash & (thisptr->max_items - 1);
  while(thisptr->arr[i].str) {
    if(strcmp(thisptr->arr[i].str, str) == 0) {
      return thisptr->arr[i];
    }
    i += 1;
    if(i == thisptr->max_items)
      i = 0;
  }

  hashtable_entry not_found;
  memset(&not_found, 0, sizeof(not_found));
  return not_found;
}

bool demogobbler_hashtable_insert(hashtable* thisptr, hashtable_entry entry) {
  if(thisptr->item_count == thisptr->max_items - 1) {
    return false; // Hash table is not resizable
  }

  ++thisptr->item_count;
  uint32_t hash = get_hash(entry.str);
  size_t initial_index = hash & (thisptr->max_items - 1);
  size_t i=initial_index;
  while(thisptr->arr[i].str) {
    if(strcmp(thisptr->arr[i].str, entry.str) == 0) {
      return false;
    }
    i += 1;
    if(i == thisptr->max_items)
      i = 0;
  }

  thisptr->arr[i] = entry;

  return true;
}

void demogobbler_hashtable_free(hashtable* thisptr) {
  free(thisptr->arr);
  thisptr->arr = NULL;
}


prop_exclude_set demogobbler_pes_create(size_t max_items) {
  prop_exclude_set set;
  memset(&set, 0, sizeof(set));
  set.max_items = 1;

  while(set.max_items - 1 < max_items)
    set.max_items <<= 1;

  const size_t array_size_bytes = set.max_items * sizeof(prop_exclude_entry);
  set.arr = malloc(array_size_bytes);
  memset(set.arr, 0, array_size_bytes);

  return set;
}

static uint32_t get_hash_prop(demogobbler_sendprop* prop) {
  // Only hash the prop name, dt name will likely match
  return XXH32(prop->name, strlen(prop->name), 0);
}

static bool match(demogobbler_sendprop* exclude, demogobbler_sendtable* table, demogobbler_sendprop* prop) {
  return strcmp(exclude->exclude_name, table->name) == 0 && strcmp(exclude->name, prop->name) == 0;
}

static bool match_excludes(demogobbler_sendprop* exclude1, demogobbler_sendprop* exclude2) {
  return strcmp(exclude1->name, exclude2->name) == 0 && strcmp(exclude1->dtname, exclude2->dtname) == 0;
}

bool demogobbler_pes_has(prop_exclude_set* thisptr, demogobbler_sendtable* table, demogobbler_sendprop* prop) {
  if(thisptr->item_count == 0) {
    return false;
  }

  uint32_t hash = get_hash_prop(prop);
  size_t i = hash & (thisptr->max_items - 1);
  while(thisptr->arr[i].exclude_prop) {
    if(match(thisptr->arr[i].exclude_prop, table, prop)) {
      return true;
    }
    i += 1;
    if(i == thisptr->max_items)
      i = 0;
  }

  return false;
}

bool demogobbler_pes_insert(prop_exclude_set* thisptr, demogobbler_sendprop* prop) {
  if(thisptr->item_count == thisptr->max_items - 1) {
    return false; // Hash table is not resizable
  }

  prop_exclude_entry entry;
  entry.exclude_prop = prop;

  ++thisptr->item_count;
  uint32_t hash = get_hash_prop(entry.exclude_prop);
  size_t initial_index = hash & (thisptr->max_items - 1);
  size_t i=initial_index;
  while(thisptr->arr[i].exclude_prop) {
    if(match_excludes(thisptr->arr[i].exclude_prop, entry.exclude_prop)) {
      return false;
    }
    i += 1;
    if(i == thisptr->max_items)
      i = 0;
  }

  thisptr->arr[i] = entry;

  return true;
}

void demogobbler_pes_free(prop_exclude_set* thisptr) {
  free(thisptr->arr);
  thisptr->arr = NULL;
}

void demogobbler_pes_clear(prop_exclude_set* thisptr) {
  const size_t array_size_bytes = thisptr->max_items * sizeof(prop_exclude_entry);
  memset(thisptr->arr, 0, array_size_bytes);
}
