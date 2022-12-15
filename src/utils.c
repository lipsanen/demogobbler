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

enum dg_proptype dg_sendprop_type(const dg_sendprop* prop) {
  if(prop->proptype == sendproptype_float) {
    if (prop->flag_coord) {
      return dg_float_bitcoord;
    } else if (prop->flag_coordmp) {
      return dg_float_bitcoordmp;
    } else if (prop->flag_coordmplp) {
      return dg_float_bitcoordmplp;
    } else if (prop->flag_coordmpint) {
      return dg_float_bitcoordmpint;
    } else if (prop->flag_noscale) {
      return dg_float_noscale;
    } else if (prop->flag_normal) {
      return dg_float_bitnormal;
    } else if (prop->flag_cellcoord) {
      return dg_float_bitcellcoord;
    } else if (prop->flag_cellcoordlp) {
      return dg_float_bitcellcoordlp;
    } else if (prop->flag_cellcoordint) {
      return dg_float_bitcellcoordint;
    } else {
      return dg_float_unsigned;
    }
  } else if(prop->proptype == sendproptype_int) {
    if (prop->flag_normal) {
      return dg_int_varuint32;
    } else if (prop->flag_unsigned) {
      return dg_int_unsigned;
    } else {
      return dg_int_signed;
    }
  }
  else {
    return INT32_MAX;
  }
}
