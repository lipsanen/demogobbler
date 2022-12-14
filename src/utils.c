#include "utils.h"
#include "demogobbler.h"
#include <string.h>

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

void dg_sendprop_name(char* buffer, size_t size, const dg_sendprop *prop)
{
	snprintf(buffer, size, "%s.%s", prop->baseclass->name, prop->name);
}
