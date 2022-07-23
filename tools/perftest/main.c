#include "demogobbler.h"
#include "stdio.h"
#include <stdlib.h>

void print_header(void* a, demogobbler_header *header) {
}

void print_consolecmd(void* a, demogobbler_consolecmd* message)
{
}

void print_customdata(void* a, demogobbler_customdata* message)
{
}

void print_datatables(void* a, demogobbler_datatables* message)
{
}

void print_packet(void* a, demogobbler_packet* message)
{
}

void print_stringtables(void* a, demogobbler_stringtables* message)
{
}

void print_stop(void* a, demogobbler_stop* message)
{
}

void print_synctick(void* a, demogobbler_synctick* message)
{
}

void print_usercmd(void* a, demogobbler_usercmd* message)
{
}

void print_netmessages(void* a, packet_net_message* message)
{
}

int main(int argc, char **argv) {
  if (argc <= 2) {
    printf("Usage: perftest <filepath> <iterations>\n");
    return 0;
  }
  int iterations = atoi(argv[2]);

  for(int i=0; i < iterations; ++i)
  {
    demogobbler_parser parser;
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
    settings.packet_net_message_handler = print_netmessages;

    demogobbler_parser_init(&parser, &settings);
    demogobbler_parser_parse_file(&parser, argv[1]);
    demogobbler_parser_free(&parser);
  }

  return 0;
}
