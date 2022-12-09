#include "demogobbler.h"
#include "demogobbler/bitwriter.h"
#include <stdio.h>
#include <string.h>

typedef struct {
  writer demo_writer;
  bitwriter message_bitwriter;
  dg_header transplant_header;
  int32_t stringtable_crc;
  int32_t map_crc;
  char map_md5[16];
  bool overwrite_crc;
} writer_state;

void writer_state_destruct(writer_state *thisptr) {
  dg_writer_free(&thisptr->demo_writer);
  bitwriter_free(&thisptr->message_bitwriter);
}

void writer_state_constructor(writer_state *thisptr, const char *filepath) {
  dg_writer_init(&thisptr->demo_writer);
  dg_writer_open_file(&thisptr->demo_writer, filepath);

  if (thisptr->demo_writer.error) {
    fprintf(stderr, "Error opening file %s: %s", filepath, thisptr->demo_writer.error_message);
    exit(1);
  }

  bitwriter_init(&thisptr->message_bitwriter, 1 << 18);
}

#define CHECK_ERROR()                                                                              \
  if (state->demo_writer.error) {                                                                  \
    fprintf(stderr, "Got error, exiting program: %s\n", state->demo_writer.error_message);         \
    exit(1);                                                                                       \
  }

void handle_header(parser_state *_state, dg_header *header) {
  writer_state *state = (writer_state *)_state->client_state;
  dg_header temp = *header;
  // Copy the demo protocol/net protocol and game dir from transplant
  temp.demo_protocol = state->transplant_header.demo_protocol;
  temp.net_protocol = state->transplant_header.net_protocol;
#if 0
  state->overwrite_crc = strcmp(state->transplant_header.map_name, header->map_name) == 0;
  if(state->overwrite_crc) {
    printf("Same map as example demo, CRC correction available\n");
  } else {
    printf("Map is not the same as in the example, cannot fix map CRC\n");
  }
#endif

  memcpy(temp.game_directory, state->transplant_header.game_directory, 260);
  dg_write_header(&state->demo_writer, &temp);
  CHECK_ERROR();
}

void handle_consolecmd(parser_state *_state, dg_consolecmd *message) {
  writer_state *state = (writer_state *)_state->client_state;
  dg_write_consolecmd(&state->demo_writer, message);
  CHECK_ERROR();
}

void handle_customdata(parser_state *_state, dg_customdata *message) {
  writer_state *state = (writer_state *)_state->client_state;
  dg_write_customdata(&state->demo_writer, message);
  CHECK_ERROR();
}

void handle_datatables_parsed(parser_state *_state, dg_datatables_parsed *message) {
  writer_state *state = (writer_state *)_state->client_state;
  dg_write_datatables_parsed(&state->demo_writer, message);
  CHECK_ERROR();
}
void handle_stringtables(parser_state *_state, dg_stringtables *message) {
  writer_state *state = (writer_state *)_state->client_state;
  dg_write_stringtables(&state->demo_writer, message);
  CHECK_ERROR();
}

void handle_stop(parser_state *_state, dg_stop *message) {
  writer_state *state = (writer_state *)_state->client_state;
  dg_write_stop(&state->demo_writer, message);
  CHECK_ERROR();
}

void handle_synctick(parser_state *_state, dg_synctick *message) {
  writer_state *state = (writer_state *)_state->client_state;
  dg_write_synctick(&state->demo_writer, message);
  CHECK_ERROR();
}

void handle_usercmd(parser_state *_state, dg_usercmd *message) {
  writer_state *state = (writer_state *)_state->client_state;
  dg_write_usercmd(&state->demo_writer, message);
  CHECK_ERROR();
}

static void handle_packetentities(parser_state *_state, dg_svc_packetentities_parsed *message) {
  writer_state *state = (writer_state *)_state->client_state;
  write_packetentities_args args;
  args.data = message->data;
  args.is_delta = message->orig->is_delta;
  args.version = &state->demo_writer.version;

  state->message_bitwriter.bitoffset = 0;
  dg_bitwriter_write_packetentities(&state->message_bitwriter, args);
}

static void packet_parsed_handler(parser_state *_state, packet_parsed *parsed) {
  writer_state *state = (writer_state *)_state->client_state;

  for (size_t i = 0; i < parsed->message_count; ++i) {
    packet_net_message *message = &parsed->messages[i];
    if (message->mtype == svc_serverinfo) {
      struct dg_svc_serverinfo *ptr = message->message_svc_serverinfo;
#if 0
      if(state->overwrite_crc) {
        ptr->stringtable_crc = state->stringtable_crc;
        ptr->map_crc = state->map_crc;
        memcpy(ptr->map_md5, state->map_md5, sizeof(ptr->map_md5));
      }
#endif
      ptr->network_protocol = state->demo_writer.version.network_protocol;
    } else if (message->mtype == svc_packet_entities) {
      struct dg_svc_packet_entities *ptr = &message->message_svc_packet_entities;
      // Fix the packetentities data
      memset(&ptr->data, 0, sizeof(dg_bitstream));
      ptr->data_length = state->message_bitwriter.bitoffset;
      ptr->data.bitoffset = 0;
      ptr->data.data = state->message_bitwriter.ptr;
      ptr->data.bitsize = state->message_bitwriter.bitoffset;
    }
  }

  dg_write_packet_parsed(&state->demo_writer, parsed);
}

typedef struct {
  dg_demver_data version_data;
  dg_header header;
  int32_t stringtable_crc;
  int32_t map_crc;
  char map_md5[16];
} version_header_stuff;

static void version_handle_demo_version(parser_state *state, dg_demver_data version) {
  version_header_stuff *ptr = (version_header_stuff *)state->client_state;
  ptr->version_data = version;
}

static void version_handle_header(parser_state *state, dg_header *header) {
  version_header_stuff *ptr = (version_header_stuff *)state->client_state;
  ptr->header = *header;
}

static void version_handle_packet_parsed(parser_state *state, packet_parsed *parsed) {
  version_header_stuff *ver = (version_header_stuff *)state->client_state;
  for (size_t i = 0; i < parsed->message_count; ++i) {
    packet_net_message *message = &parsed->messages[i];
    if (message->mtype == svc_serverinfo) {
      struct dg_svc_serverinfo *ptr = message->message_svc_serverinfo;
      ver->stringtable_crc = ptr->stringtable_crc;
      ver->map_crc = ptr->map_crc;
      memcpy(ver->map_md5, ptr->map_md5, sizeof(ptr->map_md5));
    }
  }
}

static version_header_stuff get_demo_version(char *example_path) {
  version_header_stuff data;
  memset(&data, 0, sizeof(data));
  dg_settings settings;
  dg_settings_init(&settings);
  settings.client_state = &data;
  settings.demo_version_handler = version_handle_demo_version;
  settings.header_handler = version_handle_header;
  settings.packet_parsed_handler = version_handle_packet_parsed;

  dg_parse_result result = dg_parse_file(&settings, example_path);

  if (result.error) {
    fprintf(stderr, "Error parsing example demo: %s\n", result.error_message);
    exit(1);
  }

  return data;
}

int main(int argc, char **argv) {
  if (argc <= 3) {
    printf("Usage: democonverter <example file> <input file> <output file>\n");
    return 0;
  }

  version_header_stuff data = get_demo_version(argv[1]);

  writer_state writer;
  writer_state_constructor(&writer, argv[3]);
  writer.stringtable_crc = data.stringtable_crc;
  writer.map_crc = data.map_crc;
  memcpy(writer.map_md5, data.map_md5, sizeof(data.map_md5));
  writer.demo_writer.version = data.version_data;
  writer.transplant_header = data.header;

  dg_settings settings;
  dg_settings_init(&settings);
  settings.consolecmd_handler = handle_consolecmd;
  settings.customdata_handler = handle_customdata;
  settings.datatables_parsed_handler = handle_datatables_parsed;
  settings.packetentities_parsed_handler = handle_packetentities;
  settings.header_handler = handle_header;
  settings.stop_handler = handle_stop;
  settings.stringtables_handler = handle_stringtables;
  settings.synctick_handler = handle_synctick;
  settings.usercmd_handler = handle_usercmd;
  settings.packet_parsed_handler = packet_parsed_handler;
  settings.client_state = &writer;

  dg_parse_file(&settings, argv[2]);
  dg_writer_close(&writer.demo_writer);

  return 0;
}