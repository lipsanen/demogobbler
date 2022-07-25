#include "demogobbler.h"
#include <cstdio>
#include <cstring>
#include <iostream>

struct writer_state {
  demogobbler_writer demo_writer;
  bitwriter message_bitwriter;
  demogobbler_packet current_packet;

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

  void write_packet() {
    // This has to be assigned here, as it could get moved due to being realloced while being
    // written to
    this->current_packet.data = this->message_bitwriter.ptr;
    int32_t size = this->message_bitwriter.bitoffset / 8;
    int32_t adjusted_size = size;

    if ((this->message_bitwriter.bitoffset & 0x7) != 0) {
      adjusted_size += 1;
    }

    if (adjusted_size != this->current_packet.size_bytes) {
      int i = 0;
    }
    this->current_packet.size_bytes = adjusted_size;

    demogobbler_write_packet(&this->demo_writer, &this->current_packet);
  }
};

#define CHECK_ERROR()                                                                              \
  if (state->demo_writer.error) {                                                                  \
    fprintf(stderr, "Got error, exiting program: %s\n", state->demo_writer.error_message);         \
    exit(1);                                                                                       \
  }

void handle_header(void *client_state, demogobbler_header *header) {
  printf("Enter data for conversion target, empty for default value\n");
  writer_state *state = (writer_state *)client_state;
  std::string temp;
  printf("Network protocol [%d]: ", header->net_protocol);
  std::getline(std::cin, temp);

  if (!temp.empty()) {
    header->net_protocol = std::atoi(temp.c_str());
  }

  state->demo_writer.version = demogobbler_get_demo_version(header);
  demogobbler_write_header(&state->demo_writer, header);
  CHECK_ERROR();
}

void handle_consolecmd(void *client_state, demogobbler_consolecmd *message) {
  writer_state *state = (writer_state *)client_state;
  demogobbler_write_consolecmd(&state->demo_writer, message);
  CHECK_ERROR();
}

void handle_customdata(void *client_state, demogobbler_customdata *message) {
  writer_state *state = (writer_state *)client_state;
  demogobbler_write_customdata(&state->demo_writer, message);
  CHECK_ERROR();
}

void handle_datatables(void *client_state, demogobbler_datatables *message) {
  writer_state *state = (writer_state *)client_state;
  demogobbler_write_datatables(&state->demo_writer, message);
  CHECK_ERROR();
}

void handle_packet(void *client_state, demogobbler_packet *message) {
  writer_state *state = (writer_state *)client_state;
  memcpy(&state->current_packet, message, sizeof(state->current_packet));
  state->message_bitwriter.bitoffset = 0;
  //  We finish this later on
}

void handle_stringtables(void *client_state, demogobbler_stringtables *message) {
  writer_state *state = (writer_state *)client_state;
  demogobbler_write_stringtables(&state->demo_writer, message);
  CHECK_ERROR();
}

void handle_stop(void *client_state, demogobbler_stop *message) {
  writer_state *state = (writer_state *)client_state;
  demogobbler_write_stop(&state->demo_writer, message);
  CHECK_ERROR();
}

void handle_synctick(void *client_state, demogobbler_synctick *message) {
  writer_state *state = (writer_state *)client_state;
  demogobbler_write_synctick(&state->demo_writer, message);
  CHECK_ERROR();
}

void handle_usercmd(void *client_state, demogobbler_usercmd *message) {
  writer_state *state = (writer_state *)client_state;
  demogobbler_write_usercmd(&state->demo_writer, message);
  CHECK_ERROR();
}

void netmessage_handler(void *client_state, packet_net_message *message) {
  writer_state *state = (writer_state *)client_state;

  if(message->mtype == svc_serverinfo) {
    auto* ptr = &message->message_svc_serverinfo;
    ptr->network_protocol = state->demo_writer.version.network_protocol;
  }

  demogobbler_bitwriter_write_netmessage(&state->message_bitwriter, &state->demo_writer.version,
                                         message);
  CHECK_ERROR();

  if (message->last_message) {
    unsigned int message_type_bits = state->demo_writer.version.netmessage_type_bits;
    unsigned int bits_remaining_in_last_byte = 8 - (state->message_bitwriter.bitoffset & 0x7);

    // Add a noop to the end if we have enough space
    if (bits_remaining_in_last_byte >= message_type_bits) {
      packet_net_message noop;
      noop.mtype = net_nop;
      demogobbler_bitwriter_write_netmessage(&state->message_bitwriter, &state->demo_writer.version,
                                             &noop);
    }
  
    state->write_packet();
    CHECK_ERROR();
  }

  if (state->message_bitwriter.error) {
    fprintf(stderr, "Got error, exiting program: %s\n", state->message_bitwriter.error_message);
    exit(1);
  }
}

int main(int argc, char **argv) {
  if (argc <= 2) {
    printf("Usage: democonverter <input file> <output file>\n");
    return 0;
  }

  demogobbler_parser parser;
  demogobbler_settings settings;
  demogobbler_settings_init(&settings);
  settings.consolecmd_handler = handle_consolecmd;
  settings.customdata_handler = handle_customdata;
  settings.datatables_handler = handle_datatables;
  settings.header_handler = handle_header;
  settings.packet_handler = handle_packet;
  settings.stop_handler = handle_stop;
  settings.stringtables_handler = handle_stringtables;
  settings.synctick_handler = handle_synctick;
  settings.usercmd_handler = handle_usercmd;
  settings.packet_net_message_handler = netmessage_handler;

  demogobbler_parser_init(&parser, &settings);
  writer_state writer(argv[2]);
  parser.client_state = &writer;
  demogobbler_parser_parse_file(&parser, argv[1]);
  demogobbler_writer_close(&writer.demo_writer);
  demogobbler_parser_free(&parser);

  return 0;
}