#include "demogobbler.h"
#include "parser.h"
#include "stddef.h"
#include "stdlib.h"

void demogobbler_parser_init(demogobbler_parser *thisptr, demogobbler_settings *settings) {
  if (!thisptr)
    return;

  thisptr->_parser = malloc(sizeof(parser));
  parser_init((parser *)thisptr->_parser, settings);
  thisptr->last_parse_error_message = NULL;
  thisptr->last_parse_successful = false;
}

void demogobbler_parser_parse(demogobbler_parser *thisptr, const char *filepath) {
  if (!thisptr)
    return;

  parser_parse((parser *)thisptr->_parser, filepath);
}

void demogobbler_parser_free(demogobbler_parser *thisptr) {
  if (!thisptr)
    return;

  free(thisptr->_parser);
}

void demogobbler_settings_init(demogobbler_settings *settings) {
  settings->consolecmd_handler = NULL;
  settings->customdata_handler = NULL;
  settings->datatables_handler = NULL;
  settings->header_handler = NULL;
  settings->packet_handler = NULL;
  settings->stop_handler = NULL;
  settings->stringtables_handler = NULL;
  settings->synctick_handler = NULL;
  settings->usercmd_handler = NULL;
}
