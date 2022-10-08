#pragma once

#include "parser.h"
#include "demogobbler/datatable_types.h"
#include "demogobbler/entity_types.h"

void demogobbler_estate_init_table(parser* thisptr, size_t index);
void demogobbler_parser_init_estate(parser *thisptr, demogobbler_datatables_parsed* message);
serverclass_data* demogobbler_estate_serverclass_data(parser* thisptr, size_t index);
eproparr demogobbler_eproparr_init(uint16_t prop_count);
// Get a prop_value_inner for this index, also creates it if doesnt exist 
prop_value_inner* demogobbler_eproparr_get(eproparr *thisptr, uint16_t index);
// Get the next value that is not default, pass in NULL as current to get the first value
// Returns null when no more values left
prop_value_inner* demogobbler_eproparr_next(const eproparr* thisptr, prop_value_inner* current);
void demogobbler_eproparr_free(eproparr* thisptr);

eproplist demogobbler_eproplist_init();
epropnode* demogobbler_eproplist_get(eproplist *thisptr, epropnode* initial_guess, uint16_t index);
epropnode* demogobbler_eproplist_next(const eproplist* thisptr, epropnode* current);
void demogobbler_eproplist_free(eproplist* thisptr);
