#pragma once

#include "parser.h"
#include "demogobbler_datatable_types.h"

void demogobbler_estate_init_table(parser* thisptr, size_t index);
void demogobbler_parser_init_estate(parser *thisptr, demogobbler_datatables_parsed* message);
serverclass_data* demogobbler_estate_serverclass_data(parser* thisptr, size_t index);
