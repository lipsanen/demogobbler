#pragma once

#include "demogobbler_arena.h"
#include "demogobbler_bitstream.h"

size_t demogobbler_bitstream_read_cstring_arena(bitstream* thisptr, char** ptr, arena* a);
