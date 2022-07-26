#pragma once

#include "demogobbler.h"
#include "filereader.h"
#include "stackalloc.h"
#include "stdbool.h"
#include "stdio.h"

typedef struct parser parser;

struct parser {
  parser_state state;
  demogobbler_settings m_settings;
  filereader m_reader;
  allocator allocator;
  demo_version_data demo_version;
  const char* error_message;
  bool error;
};

void parser_init(parser *thisptr, demogobbler_settings *settings);
void parser_parse(parser *thisptr, void* stream, input_interface input);
void parser_update_l4d2_version(parser* thisptr, int l4d2_version);
void parser_free(parser* thisptr);
