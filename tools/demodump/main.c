#include "demogobbler.h"
#include "stdio.h"

void print_header(demogobbler_header *header) {
  printf("ID: %s\n", header->ID);
  printf("Demo protocol: %d\n", header->demo_protocol);
  printf("Net protocol: %d\n", header->net_protocol);
  printf("Server name: %s\n", header->server_name);
  printf("Client name: %s\n", header->client_name);
  printf("Map name: %s\n", header->map_name);
  printf("Game directory: %s\n", header->game_directory);
  printf("Seconds: %f\n", header->seconds);
  printf("Tick count: %d\n", header->tick_count);
  printf("Frame count: %d\n", header->frame_count);
  printf("Signon length: %d\n", header->signon_length);
}

#define PRINT_MESSAGE_PREAMBLE(name) printf(#name ":\n"); \
  printf("\tType %d, Tick %d, \n", message->preamble.type, message->preamble.tick);


void print_consolecmd(demogobbler_consolecmd* message)
{
  PRINT_MESSAGE_PREAMBLE(consolecmd);
}

void print_customdata(demogobbler_customdata* message)
{
  PRINT_MESSAGE_PREAMBLE(customdata);
}

void print_datatables(demogobbler_datatables* message)
{
  PRINT_MESSAGE_PREAMBLE(datatables);
}

void print_packet(demogobbler_packet* message)
{
  PRINT_MESSAGE_PREAMBLE(packet);
}

void print_stringtables(demogobbler_stringtables* message)
{
  PRINT_MESSAGE_PREAMBLE(stringtables);
}

void print_stop(demogobbler_stop* message)
{
  printf("stop:\n");
}

void print_synctick(demogobbler_synctick* message)
{
  PRINT_MESSAGE_PREAMBLE(synctick);
}

void print_usercmd(demogobbler_usercmd* message)
{
  PRINT_MESSAGE_PREAMBLE(usercmd);
}

int main(int argc, char **argv) {
  if (argc <= 1) {
    printf("Usage: demodump <filepath>\n");
    return 0;
  }

  demogobbler_parser parser;
  demogobbler_settings settings;
  settings.consolecmd_handler = print_consolecmd;
  settings.customdata_handler = print_customdata;
  settings.datatables_handler = print_datatables;
  settings.header_handler = print_header;
  settings.packet_handler = print_packet;
  settings.stop_handler = print_stop;
  settings.stringtables_handler = print_stringtables;
  settings.synctick_handler = print_synctick;
  settings.usercmd_handler = print_usercmd;

  demogobbler_parser_init(&parser, &settings);
  demogobbler_parser_parse_file(&parser, argv[1]);
  demogobbler_parser_free(&parser);

  return 0;
}