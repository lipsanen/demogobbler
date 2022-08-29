#include "utils.h"
#include <string.h>

void demogobbler_dynamic_array_init(dynamic_array* thisptr, size_t min_allocation, size_t item_size)
{
  memset(thisptr, 0, sizeof(dynamic_array));
  thisptr->min_allocation = min_allocation;
  thisptr->item_size = item_size;
}

void* demogobbler_dynamic_array_add(dynamic_array* thisptr, size_t count)
{
  size_t old_count = thisptr->count;
  thisptr->count += count;
  size_t bytes_needed = thisptr->count * thisptr->item_size;

  if(bytes_needed > thisptr->allocated)
  {
    const double GROWTH_FACTOR = 2;
    size_t allocation = MAX(bytes_needed, thisptr->min_allocation);
    allocation = MAX(thisptr->allocated * GROWTH_FACTOR, allocation);
    thisptr->allocated = allocation;

    if(thisptr->ptr)
      thisptr->ptr = realloc(thisptr->ptr, thisptr->allocated);
    else
      thisptr->ptr = malloc(thisptr->allocated);
  }

  return (uint8_t*)thisptr->ptr + thisptr->item_size * old_count;
}

void demogobbler_dynamic_array_free(dynamic_array* thisptr)
{
  free(thisptr->ptr);
  memset(thisptr, 0, sizeof(dynamic_array));
}

void* demogobbler_dynamic_array_get(dynamic_array* thisptr, int64_t offset)
{
  if(thisptr->ptr && offset < (thisptr->count * thisptr->item_size) && offset >= 0) {
    return (uint8_t*)thisptr->ptr + offset;
  }
  else {
    return NULL;
  }
}

unsigned demogobbler_bits_required(unsigned i) {
  unsigned j = 1;
  uint64_t value = 2;

  while((value - 1) < i) {
    value <<= 1;
    ++j;
  }
  
  return j;
}


unsigned int highest_bit_index(unsigned int i) {
  int j;
  for (j = 31; j >= 0 && (i & (1 << j)) == 0; j--);
  return j;
}

int Q_log2(int val)
{
	int answer=0;
	while (val>>=1)
		answer++;
	return answer;
}
