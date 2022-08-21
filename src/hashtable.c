#include "demogobbler_hashtable.h"
#include <stdlib.h>
#include <string.h>

static size_t get_hash_index(hashtable* thisptr, const char* str) {
  size_t hash = 0;

  for(; *str; ++str)
  {
      hash += *str;
      hash += (hash << 10);
      hash ^= (hash >> 6);
  }

  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);

  return hash % thisptr->array_size;
}

hashtable demogobbler_hashtable_create(size_t array_size) {
  hashtable table;
  memset(&table, 0, sizeof(table));
  table.array_size = array_size;

  const size_t array_size_bytes = array_size * sizeof(hashtable_entry);
  table.arr = malloc(array_size_bytes);
  memset(table.arr, 0, array_size_bytes);

  return table;
}

hashtable_entry demogobbler_hashtable_get(hashtable* thisptr, const char* str) {
  size_t initial_index = get_hash_index(thisptr, str);
  size_t i=initial_index;
  while(thisptr->arr[i].str) {
    if(strcmp(thisptr->arr[i].str, str) == 0) {
      return thisptr->arr[i];
    }
    i += 1;
    if(i == thisptr->array_size)
      i = 0;
  }

  hashtable_entry not_found;
  memset(&not_found, 0, sizeof(not_found));
  return not_found;
}

bool demogobbler_hashtable_insert(hashtable* thisptr, hashtable_entry entry) {
  if(thisptr->item_count == thisptr->array_size - 1) {
    return false; // Hash table is not resizable
  }

  ++thisptr->item_count;
  size_t initial_index = get_hash_index(thisptr, entry.str);
  size_t i=initial_index;
  while(thisptr->arr[i].str) {
    if(strcmp(thisptr->arr[i].str, entry.str) == 0) {
      return false;
    }
    i += 1;
    if(i == thisptr->array_size)
      i = 0;
  }

  thisptr->arr[i] = entry;

  return true;
}

void demogobbler_hashtable_free(hashtable* thisptr) {
  free(thisptr->arr);
}
