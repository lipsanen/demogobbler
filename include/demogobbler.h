#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "header.h"
#include "packettypes.h"
#include <stdbool.h>

struct demogobbler_settings {
  func_demogobbler_handle_consolecmd consolecmd_handler;
  func_demogobbler_handle_customdata customdata_handler;
  func_demogobbler_handle_datatables datatables_handler;
  func_demogobbler_handle_header header_handler;
  func_demogobbler_handle_packet packet_handler;
  func_demogobbler_handle_synctick synctick_handler;
  func_demogobbler_handle_stop stop_handler;
  func_demogobbler_handle_stringtables stringtables_handler;
  func_demogobbler_handle_usercmd usercmd_handler;
};

typedef struct demogobbler_settings demogobbler_settings;

void demogobbler_settings_init(demogobbler_settings *settings);

struct demogobbler_parser {
  void *_parser;
  const char *last_parse_error_message;
  bool last_parse_successful;
};

typedef struct demogobbler_parser demogobbler_parser;

void demogobbler_parser_init(demogobbler_parser *thisptr, demogobbler_settings *settings);
void demogobbler_parser_parse(demogobbler_parser *thisptr, const char *filepath);
void demogobbler_parser_free(demogobbler_parser *thisptr);

#ifdef __cplusplus
}
#endif