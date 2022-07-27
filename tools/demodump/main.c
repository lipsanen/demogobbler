#include "demogobbler.h"
#include "stdio.h"

void print_header(parser_state* a, demogobbler_header *header) {
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


void print_consolecmd(parser_state* a, demogobbler_consolecmd* message)
{
  PRINT_MESSAGE_PREAMBLE(consolecmd);
  printf("\t%s\n", message->data);
}

void print_customdata(parser_state* a, demogobbler_customdata* message)
{
  PRINT_MESSAGE_PREAMBLE(customdata);
}

void print_datatables(parser_state* a, demogobbler_datatables* message)
{
  PRINT_MESSAGE_PREAMBLE(datatables);
}

void print_packet(parser_state* a, demogobbler_packet* message)
{
  PRINT_MESSAGE_PREAMBLE(packet);
}

void print_stringtables(parser_state* a, demogobbler_stringtables* message)
{
  PRINT_MESSAGE_PREAMBLE(stringtables);
}

void print_stop(parser_state* a, demogobbler_stop* message)
{
  printf("stop:\n");
}

void print_synctick(parser_state* a, demogobbler_synctick* message)
{
  PRINT_MESSAGE_PREAMBLE(synctick);
}

void print_usercmd(parser_state* a, demogobbler_usercmd* message)
{
  PRINT_MESSAGE_PREAMBLE(usercmd);
}

void print_netmessage(parser_state* a, packet_net_message* message)
{
}

int main(int argc, char **argv) {
  if (argc <= 1) {
    printf("Usage: demodump <filepath>\n");
    return 0;
  }

  demogobbler_settings settings;
  demogobbler_settings_init(&settings);
  settings.consolecmd_handler = print_consolecmd;
  settings.customdata_handler = print_customdata;
  settings.datatables_handler = print_datatables;
  settings.header_handler = print_header;
  settings.packet_handler = print_packet;
  settings.stop_handler = print_stop;
  settings.stringtables_handler = print_stringtables;
  settings.synctick_handler = print_synctick;
  settings.usercmd_handler = print_usercmd;
  settings.packet_net_message_handler = print_netmessage;

  demogobbler_parse_file(&settings, argv[1]);

  return 0;
}