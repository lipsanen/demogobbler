#pragma once

#include "demogobbler.h"

demo_version_data get_demo_version(demogobbler_header *header);
void version_update_build_info(demo_version_data* data);
bool get_l4d2_build(const char* str, int* out);
