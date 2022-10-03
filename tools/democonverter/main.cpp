#include "demogobbler.h"
#include "demogobbler_bitwriter.h"
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>

struct writer_state {
  demogobbler_writer demo_writer;
  bitwriter message_bitwriter;
  demogobbler_header transplant_header;

  ~writer_state() {
    demogobbler_writer_free(&this->demo_writer);
    bitwriter_free(&this->message_bitwriter);
  }

  writer_state(const char *filepath) {
    demogobbler_writer_init(&this->demo_writer);
    demogobbler_writer_open_file(&this->demo_writer, filepath);

    if (this->demo_writer.error) {
      fprintf(stderr, "Error opening file %s: %s", filepath, this->demo_writer.error_message);
      exit(1);
    }

    bitwriter_init(&this->message_bitwriter, 1 << 18);
  }
};

#define CHECK_ERROR()                                                                              \
  if (state->demo_writer.error) {                                                                  \
    fprintf(stderr, "Got error, exiting program: %s\n", state->demo_writer.error_message);         \
    exit(1);                                                                                       \
  }

void handle_header(parser_state *_state, demogobbler_header *header) {
  writer_state *state = (writer_state *)_state->client_state;
  demogobbler_header temp = *header;
  // Copy the demo protocol/net protocol and game dir from transplant
  temp.demo_protocol = state->transplant_header.demo_protocol;
  temp.net_protocol = state->transplant_header.net_protocol;
  memcpy(temp.game_directory, state->transplant_header.game_directory, 260);
  demogobbler_write_header(&state->demo_writer, &temp);
  CHECK_ERROR();
}

void handle_consolecmd(parser_state *_state, demogobbler_consolecmd *message) {
  writer_state *state = (writer_state *)_state->client_state;
  demogobbler_write_consolecmd(&state->demo_writer, message);
  CHECK_ERROR();
}

void handle_customdata(parser_state *_state, demogobbler_customdata *message) {
  writer_state *state = (writer_state *)_state->client_state;
  demogobbler_write_customdata(&state->demo_writer, message);
  CHECK_ERROR();
}

void handle_datatables(parser_state *_state, demogobbler_datatables *message) {
  writer_state *state = (writer_state *)_state->client_state;
  demogobbler_write_datatables(&state->demo_writer, message);
  CHECK_ERROR();
}
void handle_stringtables(parser_state *_state, demogobbler_stringtables *message) {
  writer_state *state = (writer_state *)_state->client_state;
  demogobbler_write_stringtables(&state->demo_writer, message);
  CHECK_ERROR();
}

void handle_stop(parser_state *_state, demogobbler_stop *message) {
  writer_state *state = (writer_state *)_state->client_state;
  demogobbler_write_stop(&state->demo_writer, message);
  CHECK_ERROR();
}

void handle_synctick(parser_state *_state, demogobbler_synctick *message) {
  writer_state *state = (writer_state *)_state->client_state;
  demogobbler_write_synctick(&state->demo_writer, message);
  CHECK_ERROR();
}

void handle_usercmd(parser_state *_state, demogobbler_usercmd *message) {
  writer_state *state = (writer_state *)_state->client_state;
  demogobbler_write_usercmd(&state->demo_writer, message);
  CHECK_ERROR();
}

static void packet_parsed_handler(parser_state *_state, packet_parsed* parsed) {
  writer_state *state = (writer_state *)_state->client_state;

  for(size_t i=0; i < parsed->message_count; ++i) {
    packet_net_message* message = &parsed->messages[i];
    if(message->mtype == svc_serverinfo) {
      auto* ptr = message->message_svc_serverinfo;
      ptr->network_protocol = state->demo_writer.version.network_protocol;
    }
  }

  demogobbler_write_packet_parsed(&state->demo_writer, parsed);
}

struct version_header_stuff
{
  demo_version_data version_data;
  demogobbler_header header;
};

static void version_handle_demo_version(parser_state* state, demo_version_data version)
{
  version_header_stuff* ptr = (version_header_stuff*)state->client_state;
  ptr->version_data = version;
}

static void version_handle_header(parser_state* state, demogobbler_header* header)
{
  version_header_stuff* ptr = (version_header_stuff*)state->client_state;
  ptr->header = *header;
}

static version_header_stuff get_demo_version(char* example_path)
{
  version_header_stuff data;
  demogobbler_settings settings;
  demogobbler_settings_init(&settings);
  settings.client_state = &data;
  settings.demo_version_handler = version_handle_demo_version;
  settings.header_handler = version_handle_header;

  demogobbler_parse_result result = demogobbler_parse_file(&settings, example_path);

  if(result.error)
  {
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

  writer_state writer(argv[3]);
  writer.demo_writer.version = data.version_data;
  writer.transplant_header = data.header;

  demogobbler_settings settings;
  demogobbler_settings_init(&settings);
  settings.consolecmd_handler = handle_consolecmd;
  settings.customdata_handler = handle_customdata;
  settings.datatables_handler = handle_datatables;
  settings.header_handler = handle_header;
  settings.stop_handler = handle_stop;
  settings.stringtables_handler = handle_stringtables;
  settings.synctick_handler = handle_synctick;
  settings.usercmd_handler = handle_usercmd;
  settings.packet_parsed_handler = packet_parsed_handler;
  settings.client_state = &writer;

  demogobbler_parse_file(&settings, argv[2]);
  demogobbler_writer_close(&writer.demo_writer);

  return 0;
}