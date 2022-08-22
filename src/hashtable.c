#include "demogobbler_hashtable.h"
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
  size_t i= hash & (thisptr->max_items - 1);
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
}
