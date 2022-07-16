#pragma once

#include "demogobbler.h"
#include "filereader.h"
#include "stackalloc.h"
#include "stdbool.h"
#include "stdio.h"

typedef struct parser parser;

struct parser {
  demogobbler_settings m_settings;
  filereader m_reader;
  allocator allocator;
  int32_t demo_protocol;
  int32_t net_protocol;
  demo_version _demo_version;
  const char* error_message;
  bool error;
};

void parser_init(parser *thisptr, demogobbler_settings *settings);
void parser_parse(parser *thisptr, void* stream, input_interface input);
void parser_free(parser* thisptr);
