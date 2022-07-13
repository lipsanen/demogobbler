#include "demogobbler.h"
#include "stdio.h"

static FILE *output = NULL;

#define WRITE_STRING(field, sizebytes) fwrite(header->field, 1, sizebytes, output);
#define WRITE_INT32(field) fwrite(&header->field, 1, 4, output);

void write_header(demogobbler_header *header) {
  WRITE_STRING(ID, 8);
  WRITE_INT32(demo_protocol);
  WRITE_INT32(net_protocol);
  WRITE_STRING(server_name, 260);
  WRITE_STRING(client_name, 260);
  WRITE_STRING(map_name, 260);
  WRITE_STRING(game_directory, 260);
  WRITE_INT32(seconds);
  WRITE_INT32(tick_count);
  WRITE_INT32(event_count);
  WRITE_INT32(signon_length);
}

int main(int argc, char **argv) {
  if (argc <= 2) {
    printf("Usage: democopy <input filepath> <output filepath>\n");
    return 0;
  }

  output = fopen(argv[2], "wb");

  if (!output) {
    printf("Unable to open file %s\n", argv[2]);
    return 1;
  }

  demogobbler_parser parser;
  demogobbler_settings settings;
  settings.header_handler = write_header;

  demogobbler_parser_init(&parser, &settings);
  demogobbler_parser_parse(&parser, argv[1]);
  demogobbler_parser_free(&parser);

  return 0;
}