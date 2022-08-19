#include "freddie.h"
#include <string.h>

static void copy_string(char **dest, const char *src, arena *memory_arena) {
  if (src == NULL) {
    *dest = NULL;
    return;
  }

  size_t length = strlen(src) + 1;
  char *chunk = demogobbler_arena_allocate(memory_arena, length, 1);

  if (chunk == NULL) {
    *dest = NULL;
    return;
  }

  memcpy(chunk, src, length);
  // Set the pointer afterwards, also works in the case the dest and src refer to the same pointer
  *dest = chunk;
}

struct freddie_demo_state {
  freddie_demo* demo;
  freddie_demo_message wip_packet;
};

typedef struct freddie_demo_state freddie_demo_state;

static void add_demo_message(freddie_demo *demo, freddie_demo_message msg) {
  ++demo->message_count;
  demo->messages = realloc(demo->messages, sizeof(*demo->messages) * demo->message_count);
  demo->messages[demo->message_count - 1] = msg;
}

static void handle_header(parser_state *_state, demogobbler_header *header) {
  freddie_demo_state *demo_state = _state->client_state;
  freddie_demo *demo = demo_state->demo;
  demo->header = *header;
}

static void handle_consolecmd(parser_state *_state, demogobbler_consolecmd *message) {
  freddie_demo_state *demo_state = _state->client_state;
  freddie_demo *demo = demo_state->demo;
  freddie_demo_message msg;
  msg.mtype = message->preamble.converted_type;
  msg.consolecmd = *message;
  copy_string(&msg.consolecmd.data, msg.consolecmd.data, &demo->memory_arena);
  add_demo_message(demo, msg);
}

static void handle_customdata(parser_state *_state, demogobbler_customdata *message) {
  freddie_demo_state *demo_state = _state->client_state;
  freddie_demo *demo = demo_state->demo;
  freddie_demo_message msg;
  msg.mtype = message->preamble.converted_type;
  msg.customdata = *message;
  msg.customdata.data = NULL;
  add_demo_message(demo, msg);
}

static void handle_datatables(parser_state *_state, demogobbler_datatables *message) {
  freddie_demo_state *demo_state = _state->client_state;
  freddie_demo *demo = demo_state->demo;
  freddie_demo_message msg;
  msg.mtype = message->preamble.converted_type;
  msg.datatables = *message;
  msg.datatables.data = NULL;
  add_demo_message(demo, msg);
}

static void handle_stringtables(parser_state *_state, demogobbler_stringtables *message) {
  freddie_demo_state *demo_state = _state->client_state;
  freddie_demo *demo = demo_state->demo;
  freddie_demo_message msg;
  msg.mtype = message->preamble.converted_type;
  msg.stringtables = *message;
  msg.stringtables.data = NULL;
  add_demo_message(demo, msg);
}

static void handle_stop(parser_state *_state, demogobbler_stop *message) {
  freddie_demo_state *demo_state = _state->client_state;
  freddie_demo *demo = demo_state->demo;
  freddie_demo_message msg;
  msg.mtype = demogobbler_type_stop;
  msg.stop = *message;
  add_demo_message(demo, msg);
}

static void handle_synctick(parser_state *_state, demogobbler_synctick *message) {
  freddie_demo_state *demo_state = _state->client_state;
  freddie_demo *demo = demo_state->demo;
  freddie_demo_message msg;
  msg.mtype = message->preamble.converted_type;
  msg.synctick = *message;
  add_demo_message(demo, msg);
}

static void handle_usercmd(parser_state *_state, demogobbler_usercmd *message) {
  freddie_demo_state *demo_state = _state->client_state;
  freddie_demo *demo = demo_state->demo;
  freddie_demo_message msg;
  msg.mtype = message->preamble.converted_type;
  msg.usercmd = *message;
  msg.usercmd.data = NULL;
  add_demo_message(demo, msg);
}

static void handle_packet(parser_state *_state, demogobbler_packet *incoming) {
  freddie_demo_state *demo_state = _state->client_state;
  freddie_demo *demo = demo_state->demo;
  memset(&demo_state->wip_packet, 0, sizeof(freddie_demo_message));
  demo_state->wip_packet.mtype = incoming->preamble.converted_type;
  demo_state->wip_packet.packet.preamble = incoming->preamble;
  demo_state->wip_packet.packet.cmdinfo = demogobbler_arena_allocate(
      &demo->memory_arena, sizeof(demogobbler_cmdinfo) * incoming->cmdinfo_size,
      alignof(demogobbler_cmdinfo));
  memcpy(demo_state->wip_packet.packet.cmdinfo, incoming->cmdinfo,
         sizeof(demogobbler_cmdinfo) * incoming->cmdinfo_size);
  demo_state->wip_packet.packet.cmdinfo_size = incoming->cmdinfo_size;
}

static void handle_netmessage(parser_state *_state, packet_net_message *message) {
  freddie_demo_state *demo_state = _state->client_state;
  freddie_demo *demo = demo_state->demo;
  freddie_packet *packet = &demo_state->wip_packet.packet;
  ++packet->net_messages_count;
  packet->net_messages = realloc(packet->net_messages, packet->net_messages_count * sizeof(packet_net_message));
  size_t last_index = packet->net_messages_count - 1;
  packet->net_messages[last_index] = *message;

  if (message->last_message) {
    add_demo_message(demo, demo_state->wip_packet);
  }
}

demogobbler_parse_result freddie_parse_file(const char *filepath, freddie_demo *output) {
  demogobbler_parse_result result;
  const int ARENA_INITIAL_SIZE_BYTES = 1 << 15;
  memset(output, 0, sizeof(*output));
  output->memory_arena = demogobbler_arena_create(ARENA_INITIAL_SIZE_BYTES);
  freddie_demo_state demo_state;
  memset(&demo_state, 0, sizeof(freddie_demo_state));
  demo_state.demo = output;

  demogobbler_settings settings;
  demogobbler_settings_init(&settings);
  settings.client_state = &demo_state;
  settings.consolecmd_handler = handle_consolecmd;
  settings.customdata_handler = handle_customdata;
  settings.header_handler = handle_header;
  settings.datatables_handler = handle_datatables;
  settings.packet_net_message_handler = handle_netmessage;
  settings.packet_handler = handle_packet;
  settings.stop_handler = handle_stop;
  settings.stringtables_handler = handle_stringtables;
  settings.synctick_handler = handle_synctick;
  settings.usercmd_handler = handle_usercmd;

  result = demogobbler_parse_file(&settings, filepath);

  return result;
}

void freddie_free_demo(freddie_demo *demo) {
  demogobbler_arena_free(&demo->memory_arena);
  for (size_t i = 0; i < demo->message_count; ++i) {
    if (demo->messages[i].mtype == demogobbler_type_signon ||
        demo->messages[i].mtype == demogobbler_type_packet) {
      free(demo->messages[i].packet.net_messages);
    }
  }

  free(demo->messages);
}
