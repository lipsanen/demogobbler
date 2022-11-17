#pragma once

#include "demogobbler.h"
#include "demogobbler/bitwriter.h"
#include "demogobbler/parser.h"
#include "demogobbler/usercmd_types.h"

dg_parse_result dg_parser_parse_usercmd(const dg_demver_data* version_data, const dg_usercmd *input, dg_usercmd_parsed* out);
void dg_bitwriter_write_usercmd(bitwriter* thisptr, dg_usercmd_parsed* parsed);
