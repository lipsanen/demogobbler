#pragma once

#include "demogobbler/arena.h"
#include "demogobbler/datatable_types.h"
#include "demogobbler/entity_types.h"
#include "demogobbler/parser.h"
#include <stdbool.h>

void dg_estate_init_table(dg_parser *thisptr, size_t index);
void dg_parser_init_estate(dg_parser *thisptr, dg_datatables_parsed *message);
dg_serverclass_data *dg_estate_serverclass_data(dg_parser *thisptr, size_t index);
dg_eproparr dg_eproparr_init(uint16_t prop_count);
// Get a dg_prop_value_inner for this index, also creates it if doesnt exist
dg_prop_value_inner *dg_eproparr_get(dg_eproparr *thisptr, uint16_t index, bool *new_prop);
// Get the next value that is not default, pass in NULL as current to get the first value
// Returns null when no more values left
dg_prop_value_inner *dg_eproparr_next(const dg_eproparr *thisptr, dg_prop_value_inner *current);
void dg_eproparr_free(dg_eproparr *thisptr);

dg_eproplist dg_eproplist_init();
dg_epropnode *dg_eproplist_get(dg_eproplist *thisptr, dg_epropnode *initial_guess, uint16_t index,
                               bool *new_prop);
dg_epropnode *dg_eproplist_next(const dg_eproplist *thisptr, dg_epropnode *current);
void dg_eproplist_free(dg_eproplist *thisptr);
