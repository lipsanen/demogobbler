#include "demogobbler.h"
#include "parser.h"
#include "streams.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

void demogobbler_parser_init(demogobbler_parser *thisptr, demogobbler_settings *settings) {
  if (!thisptr)
    return;

  thisptr->_parser = malloc(sizeof(parser));
  thisptr->client_state = NULL;
  parser_init((parser *)thisptr->_parser, thisptr, settings);
  thisptr->error_message = NULL;
  thisptr->error = false;
}

void demogobbler_parser_check_errors(demogobbler_parser* thisptr) {
  if(((parser *)thisptr->_parser)->error) {
    thisptr->error = true;
    thisptr->error_message = ((parser *)thisptr->_parser)->error_message;
  }
}

void demogobbler_parser_parse(demogobbler_parser *thisptr, void *stream,
                              input_interface input_interface) {
  parser_parse((parser *)thisptr->_parser, stream, input_interface);
  demogobbler_parser_check_errors(thisptr);
}

void demogobbler_parser_parse_file(demogobbler_parser *thisptr, const char *filepath) {
  if (!thisptr)
    return;

  FILE *file = fopen(filepath, "rb");

  if (file) {
    input_interface input;
    input.read = fstream_read;
    input.seek = fstream_seek;

    demogobbler_parser_parse(thisptr, file, input);

    fclose(file);
  } else {
    thisptr->error = true;
    thisptr->error_message = "Unable to open file";
  }
}

void demogobbler_parser_parse_buffer(demogobbler_parser *thisptr, void *buffer, size_t size) {
  if(buffer) {
    input_interface input;
    input.read = buffer_stream_read;
    input.seek = buffer_stream_seek;

    buffer_stream stream;
    buffer_stream_init(&stream, buffer, size);
    demogobbler_parser_parse(thisptr, &stream, input);
  } else {
    thisptr->error = true;
    thisptr->error_message = "Buffer was NULL";
  }
}

void demogobbler_parser_free(demogobbler_parser *thisptr) {
  if (!thisptr)
    return;

  parser_free((parser*)thisptr->_parser);
  free(thisptr->_parser);
}

void demogobbler_settings_init(demogobbler_settings *settings) {
  memset(settings, 0, sizeof(demogobbler_settings));
}
