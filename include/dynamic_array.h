#pragma once

#include <stdlib.h>

struct demogobbler_dynamic_array
{
  void* ptr;
  size_t count;
  size_t min_allocation;
  size_t allocated;
  size_t item_size;
};

typedef struct demogobbler_dynamic_array dynamic_array;
