#pragma once

#include "demogobbler/arena.h"
#include "demogobbler/datatable_types.h"
#include "demogobbler.h"
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
  demogobbler_settings m_settings;
  filereader m_reader;
  arena temp_arena;
  demo_version_data demo_version;
  const char* error_message;
  bool error;
  bool parse_netmessages;
  entity_parse_scrap ent_scrap;
} parser;

void parser_init(parser *thisptr, demogobbler_settings *settings);
void demogobbler_parser_arena_check_init(parser *thisptr);
void parser_parse(parser *thisptr, void* stream, input_interface input);
void parser_update_l4d2_version(parser* thisptr, int l4d2_version);
