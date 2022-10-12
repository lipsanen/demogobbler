#pragma once

#include "demogobbler.h"
#include "demogobbler/arena.h"
#include "demogobbler/datatable_types.h"
#include "filereader.h"
#include "parser.h"
#include "stdbool.h"
#include "stdio.h"

#ifdef DEBUG
#define DEBUG_BREAK_PROP 1
#endif

extern int CURRENT_DEBUG_INDEX;

typedef struct {
  parser_state state;
  dg_settings m_settings;
  filereader m_reader;
  dg_arena temp_arena;
  dg_demver_data demo_version;
  const char *error_message;
  bool error;
  bool parse_netmessages;
  entity_parse_scrap ent_scrap;
} parser;

void parser_init(parser *thisptr, dg_settings *settings);
void dg_parser_arena_check_init(parser *thisptr);
void parser_parse(parser *thisptr, void *stream, input_interface input);
void parser_update_l4d2_version(parser *thisptr, int l4d2_version);
