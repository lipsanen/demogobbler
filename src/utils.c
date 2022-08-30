#include "utils.h"
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
