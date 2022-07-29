#include "demogobbler.h"
#include <stdio.h>
#include <string.h>

void print_header(parser_state *a, demogobbler_header *header) {
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

struct dump_state {
  demo_version_data version_data;
};

typedef struct dump_state dump_state;

#define PRINT_MESSAGE_PREAMBLE(name) printf(#name ", Tick %d, Slot %d\n", message->preamble.tick, message->preamble.slot);

void print_consolecmd(parser_state *a, demogobbler_consolecmd *message) {
  PRINT_MESSAGE_PREAMBLE(consolecmd);
  printf("\t%s\n", message->data);
}

void print_customdata(parser_state *a, demogobbler_customdata *message) {
  PRINT_MESSAGE_PREAMBLE(customdata);
}

void print_datatables(parser_state *a, demogobbler_datatables *message) {
  PRINT_MESSAGE_PREAMBLE(datatables);
}

void print_packet(parser_state *a, demogobbler_packet *message) {
  dump_state *state = a->client_state;
  if (message->preamble.type == demogobbler_type_signon) {
    printf("Signon, Tick %d, Slot %d\n", message->preamble.tick, message->preamble.slot);
  } else {
    printf("Packet, Tick %d, Slot %d\n", message->preamble.tick, message->preamble.slot);
  }

  for (int i = 0; i < state->version_data.cmdinfo_size; ++i) {
    printf("Cmdinfo %d:\n", i);
    printf("\tInterp flags %d\n", message->cmdinfo[i].interp_flags);
    printf("\tLocal viewangles (%f, %f, %f)\n", message->cmdinfo[i].local_viewangles.x,
           message->cmdinfo[i].local_viewangles.y, message->cmdinfo[i].local_viewangles.z);
    printf("\tLocal viewangles 2 (%f, %f, %f)\n", message->cmdinfo[i].local_viewangles2.x,
           message->cmdinfo[i].local_viewangles2.y, message->cmdinfo[i].local_viewangles2.z);
    printf("\tView angles (%f, %f, %f)\n", message->cmdinfo[i].view_angles.x,
           message->cmdinfo[i].view_angles.y, message->cmdinfo[i].view_angles.z);
    printf("\tView angles2 (%f, %f, %f)\n", message->cmdinfo[i].view_angles2.x,
           message->cmdinfo[i].view_angles2.y, message->cmdinfo[i].view_angles2.z);
    printf("\tOrigin (%f, %f, %f)\n", message->cmdinfo[i].view_origin.x,
           message->cmdinfo[i].view_origin.y, message->cmdinfo[i].view_origin.z);
    printf("\tOrigin 2 (%f, %f, %f)\n", message->cmdinfo[i].view_origin2.x,
           message->cmdinfo[i].view_origin2.y, message->cmdinfo[i].view_origin2.z);
  }

  printf("In sequence: %d, Out sequence: %d\n", message->in_sequence, message->out_sequence);
  printf("Messages:\n");
}

void print_stringtables(parser_state *a, demogobbler_stringtables *message) {
  PRINT_MESSAGE_PREAMBLE(stringtables);
}

void print_stop(parser_state *a, demogobbler_stop *message) { printf("stop:\n"); }

void print_synctick(parser_state *a, demogobbler_synctick *message) {
  PRINT_MESSAGE_PREAMBLE(synctick);
}

void print_usercmd(parser_state *a, demogobbler_usercmd *message) {
  PRINT_MESSAGE_PREAMBLE(usercmd);
}

void print_netmessage(parser_state *a, packet_net_message *message) {
  
}

void handle_version(parser_state *a, demo_version_data message) {
  dump_state *state = a->client_state;
  state->version_data = message;
}

int main(int argc, char **argv) {
  if (argc <= 1) {
    printf("Usage: demodump <filepath>\n");
    return 0;
  }

  dump_state dump;
  memset(&dump, 0, sizeof(dump_state));
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