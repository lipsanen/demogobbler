#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "demogobbler_entity_types.h"
#include "demogobbler_datatables.h"

float demogobbler_prop_to_float(demogobbler_sendprop* prop, prop_value_inner value);

#ifdef __cplusplus
}
#endif