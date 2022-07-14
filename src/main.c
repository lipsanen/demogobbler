#include "demogobbler.h"
#include "parser.h"
#include "streams.h"
#include <stddef.h>
#include <stdlib.h>

void demogobbler_parser_init(demogobbler_parser *thisptr, demogobbler_settings *settings) {
  if (!thisptr)
    return;

  thisptr->_parser = malloc(sizeof(parser));
  parser_init((parser *)thisptr->_parser, settings);
  thisptr->error_message = NULL;
  thisptr->error = false;
}

void demogobbler_parser_parse(demogobbler_parser *thisptr, void *stream,
                              input_interface input_interface) {
  parser_parse((parser *)thisptr->_parser, stream, input_interface);
}

void demogobbler_parser_parse_file(demogobbler_parser *thisptr, const char *filepath) {
  if (!thisptr)
    return;

  FILE* file = fopen(filepath, "rb");

  if(file)
  {
    input_interface input;
    input.read = fstream_read;
    input.seek = fstream_seek;

    parser_parse((parser *)thisptr->_parser, file, input);

    fclose(file);
  }
  else
  {
    thisptr->error = false;
    thisptr->error_message = "Unable to open file";
  }

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
