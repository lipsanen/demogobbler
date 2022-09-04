#include "demogobbler_conversions.h"
#include "utils.h"

static float bitcoordmp_to_float(demogobbler_bitcoordmp value, bool is_int, bool lp) {
  if(is_int) {
    return value.int_val * (value.sign ? -1 : 1);
  } else {
    float val = value.int_val * (value.sign ? -1 : 1);
    if(lp) {
      val += value.frac_val * COORD_RESOLUTION_LP;
    } else {
      val += value.frac_val * COORD_RESOLUTION;
    }
    return val;
  }
}

static float bitnormal_to_float(demogobbler_bitnormal value) {
  float out = value.frac / NORM_RES;

  if(value.sign)
    out = -out;
  return out;
}

static float bitcellcoord_to_float(demogobbler_bitcellcoord value, bool is_int, bool lp) {
  float out = value.int_val;
  
  if(!is_int) {
    if(lp) {
      out += value.fract_val * COORD_RESOLUTION_LP;
    } else {
      out += value.fract_val * COORD_RESOLUTION;
    }
  }

  return out;
}

static float bitcoord_to_float(bitcoord value) {
  float out = 0;

  if(value.has_int)
    out += value.int_value + 1;
  out += value.frac_value * COORD_RESOLUTION;
  if(value.sign)
    out *= -1;

  return out;
}

float demogobbler_prop_to_float(demogobbler_sendprop* prop, prop_value_inner value) {
  if (prop->flag_coord) {
    return bitcoord_to_float(value.bitcoord_val);
  } else if (prop->flag_coordmp) {
    return bitcoordmp_to_float(value.bitcoordmp_val, false, false);
  } else if (prop->flag_coordmplp) {
    return bitcoordmp_to_float(value.bitcoordmp_val, false, true);
  } else if (prop->flag_coordmpint) {
    return bitcoordmp_to_float(value.bitcoordmp_val, true, false);
  } else if (prop->flag_noscale) {
    return value.float_val;
  } else if (prop->flag_normal) {
    return bitnormal_to_float(value.bitnormal_val);
  } else if (prop->flag_cellcoord) {
    return bitcellcoord_to_float(value.bitcellcoord_val, false, false);
  } else if (prop->flag_cellcoordlp) {
    return bitcellcoord_to_float(value.bitcellcoord_val, false, true);
  } else if (prop->flag_cellcoordint) {
    return bitcellcoord_to_float(value.bitcellcoord_val, true, false);
  } else {
    float scale = (float)value.unsigned_val / ((1 << prop->prop_numbits) - 1);
    return prop->prop_.low_value + (prop->prop_.high_value - prop->prop_.low_value) * scale;
  }

  return 0.0f;
}
