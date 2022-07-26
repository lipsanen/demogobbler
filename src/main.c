#include "demogobbler.h"
#include "parser.h"
#include "streams.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

demogobbler_parse_result demogobbler_parse(demogobbler_settings *settings, void *stream,
                              input_interface input_interface) {
  demogobbler_parse_result out;
  memset(&out, 0, sizeof(out));
  parser parser;
  parser_init(&parser, settings);
  parser_parse(&parser, stream, input_interface);
  out.error = parser.error;
  out.error_message = parser.error_message;
  parser_free(&parser);

  return out;
}

demogobbler_parse_result demogobbler_parse_file(demogobbler_settings *settings, const char *filepath) {
  demogobbler_parse_result out;
  memset(&out, 0, sizeof(out));
  FILE *file = fopen(filepath, "rb");

  if (file) {
    input_interface input;
    input.read = fstream_read;
    input.seek = fstream_seek;

    out = demogobbler_parse(settings, file, input);

    fclose(file);
  } else {
    out.error = true;
    out.error_message = "Unable to open file";
  }

  return out;
}

demogobbler_parse_result demogobbler_parse_buffer(demogobbler_settings *settings, void *buffer, size_t size) {
  demogobbler_parse_result out;
  memset(&out, 0, sizeof(out));

  if(buffer) {
    input_interface input;
    input.read = buffer_stream_read;
    input.seek = buffer_stream_seek;

    buffer_stream stream;
    buffer_stream_init(&stream, buffer, size);
    out = demogobbler_parse(settings, &stream, input);
  } else {
    out.error = true;
    out.error_message = "Buffer was NULL";
  }

  return out;
}

void demogobbler_settings_init(demogobbler_settings *settings) {
  memset(settings, 0, sizeof(demogobbler_settings));
}
