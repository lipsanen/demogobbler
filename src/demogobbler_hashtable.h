#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct {
  const char* str;
  size_t value;
} hashtable_entry;

typedef struct {
  hashtable_entry* arr;
  size_t array_size;
  size_t item_count;
} hashtable;

hashtable demogobbler_hashtable_create(size_t array_size);
hashtable_entry demogobbler_hashtable_get(hashtable* thisptr, const char* str);
bool demogobbler_hashtable_insert(hashtable* thisptr, hashtable_entry entry);
void demogobbler_hashtable_free(hashtable* thisptr);