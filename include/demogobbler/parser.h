#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "demogobbler.h"
#include "demogobbler/arena.h"
#include "demogobbler/datatable_types.h"
#include "demogobbler/filereader.h"
#include "stdbool.h"
#include "stdio.h"

#ifdef DEBUG
#define DEBUG_BREAK_PROP 1
#endif

extern int CURRENT_DEBUG_INDEX;

typedef struct {
  parser_state state;
  dg_settings m_settings;
  dg_filereader m_reader;
  dg_arena temp_arena;
  dg_demver_data demo_version;
  const char *error_message;
  bool error;
  bool parse_netmessages;
  entity_parse_scrap ent_scrap;
} dg_parser;

void dg_parser_init(dg_parser *thisptr, dg_settings *settings);
void dg_parser_arena_check_init(dg_parser *thisptr);
void dg_parser_parse(dg_parser *thisptr, void *stream, dg_input_interface input);
void dg_parser_update_l4d2_version(dg_parser *thisptr, int l4d2_version);

#ifdef __cplusplus
}
#endif
