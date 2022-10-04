#pragma once

#include "demogobbler.h"
#include <stdint.h>

void demogobbler_write_byte(writer* writer, uint8_t value);
void demogobbler_write_short(writer* writer, uint16_t value);
void demogobbler_write_int32(writer* thisptr, int32_t value);
void demogobbler_write_data(writer* writer, void* src, uint32_t bytes);
void demogobbler_write_string(writer* writer, const char* str);
