#include "demogobbler.h"
#include "stdio.h"

void print_header(demogobbler_header* header)
{
    printf("ID: %s\n", header->ID);
    printf("Demo protocol: %d\n", header->demo_protocol);
    printf("Net protocol: %d\n", header->net_protocol);
    printf("Server name: %s\n", header->server_name);
    printf("Client name: %s\n", header->client_name);
    printf("Map name: %s\n", header->map_name);
    printf("Game directory: %s\n", header->game_directory);
    printf("Seconds: %f\n", header->seconds);
    printf("Tick count: %d\n", header->tick_count);
    printf("Event count: %d\n", header->event_count);
    printf("Signon length: %d\n", header->signon_length);
}

int main(int argc, char** argv)
{
    if(argc <= 1)
    {
        printf("Usage: demogobbler <filepath>\n");
        return 0;
    }

    demogobbler_parser parser;
    demogobbler_settings settings;
    settings.header_handler = print_header;

    demogobbler_parser_init(&parser, &settings);
    demogobbler_parser_parse(&parser, argv[1]);
    demogobbler_parser_free(&parser);

    return 0;
}