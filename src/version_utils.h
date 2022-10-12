#pragma once

#include "demogobbler.h"

dg_demver_data dg_get_demo_version(dg_header *header);
void version_update_build_info(dg_demver_data *data);
bool get_l4d2_build(const char *str, int *out);
