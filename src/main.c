#include "demogobbler.h"
#include "parser.h"
#include "streams.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

dg_parse_result dg_parse(dg_settings *settings, void *stream, input_interface input_interface) {
  dg_parse_result out;
  memset(&out, 0, sizeof(out));
  parser parser;
  parser_init(&parser, settings);
  parser_parse(&parser, stream, input_interface);
  out.error = parser.error;
  out.error_message = parser.error_message;

  return out;
}

dg_parse_result dg_parse_file(dg_settings *settings, const char *filepath) {
  dg_parse_result out;
  memset(&out, 0, sizeof(out));
  FILE *file = fopen(filepath, "rb");

  if (file) {
    input_interface input;
    input.read = fstream_read;
    input.seek = fstream_seek;

    out = dg_parse(settings, file, input);

    fclose(file);
  } else {
    out.error = true;
    out.error_message = "Unable to open file";
  }

  return out;
}

dg_parse_result dg_parse_buffer(dg_settings *settings, void *buffer, size_t size) {
  dg_parse_result out;
  memset(&out, 0, sizeof(out));

  if (buffer) {
    input_interface input;
    input.read = buffer_stream_read;
    input.seek = buffer_stream_seek;

    buffer_stream stream;
    buffer_stream_init(&stream, buffer, size);
    out = dg_parse(settings, &stream, input);
  } else {
    out.error = true;
    out.error_message = "Buffer was NULL";
  }

  return out;
}

void dg_settings_init(dg_settings *settings) { memset(settings, 0, sizeof(dg_settings)); }
