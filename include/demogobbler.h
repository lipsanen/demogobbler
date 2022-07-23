#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "header.h"
#include "demogobbler_io.h"
#include "packettypes.h"
#include "packet_netmessages.h"
#include <stdio.h>
#include <stdbool.h>

enum demogobbler_game {
  csgo, l4d, l4d2, portal2, orangebox, steampipe
};

struct demogobbler_demo_version {
  enum demogobbler_game game : 3;
  unsigned int netmessage_type_bits : 3;
  unsigned int demo_protocol : 3;
  unsigned int cmdinfo_size : 3;
  unsigned int net_file_bits : 2;
  unsigned int stringtable_flags_bits : 2;
  unsigned int stringtable_userdata_size_bits : 5;
  unsigned int svc_user_message_bits : 4;
  unsigned int svc_prefetch_bits : 4;
  unsigned int model_index_bits : 4;
  bool has_slot_in_preamble : 1;
  bool has_nettick_times : 1;
  bool l4d2_version_finalized : 1;
  net_message_type* netmessage_array;
  unsigned int netmessage_count;
  unsigned int network_protocol;
  unsigned int l4d2_version;
};

typedef struct demogobbler_demo_version demo_version_data;

typedef void (*func_demogobbler_handle_demo_version) (void* client_state, demo_version_data version);

struct demogobbler_settings {
  func_demogobbler_handle_consolecmd consolecmd_handler;
  func_demogobbler_handle_customdata customdata_handler;
  func_demogobbler_handle_datatables datatables_handler;
  func_demogobbler_handle_demo_version demo_version_handler;
  func_demogobbler_handle_header header_handler;
  func_demogobbler_handle_packet packet_handler;
  func_demogobbler_handle_synctick synctick_handler;
  func_demogobbler_handle_stop stop_handler;
  func_demogobbler_handle_stringtables stringtables_handler;
  func_demogobbler_handle_usercmd usercmd_handler;
  func_demogobbler_handle_packet_net_message packet_net_message_handler;
};

typedef struct demogobbler_settings demogobbler_settings;

void demogobbler_settings_init(demogobbler_settings *settings);

struct demogobbler_parser {
  void *_parser;
  void *client_state;
  const char *error_message;
  bool error;
};

typedef struct demogobbler_parser demogobbler_parser;

void demogobbler_parser_init(demogobbler_parser *thisptr, demogobbler_settings *settings);
void demogobbler_parser_parse_file(demogobbler_parser *thisptr, const char *filepath);
void demogobbler_parser_parse_buffer(demogobbler_parser* thisptr, void* buffer, size_t size);
void demogobbler_parser_parse(demogobbler_parser *thisptr, void *stream,
                             input_interface input_interface);
void demogobbler_parser_free(demogobbler_parser *thisptr);

struct demogobbler_writer {
  void *_stream;
  const char *error_message;
  output_interface output_funcs;
  bool error_set;
  bool _custom_stream;
  demo_version_data version;
};

typedef struct demogobbler_writer writer;

void demogobbler_writer_init(writer *thisptr);
void demogobbler_writer_open_file(writer *thisptr, const char *filepath);
void demogobbler_writer_open(writer *thisptr, void *stream,
                             output_interface output_interface);
void demogobbler_writer_close(writer *thisptr);
void demogobbler_write_consolecmd(writer *thisptr, demogobbler_consolecmd *message);
void demogobbler_write_customdata(writer *thisptr, demogobbler_customdata *message);
void demogobbler_write_datatables(writer *thisptr, demogobbler_datatables *message);
void demogobbler_write_header(writer *thisptr, demogobbler_header *message);
void demogobbler_write_packet(writer *thisptr, demogobbler_packet *message);
void demogobbler_write_synctick(writer *thisptr, demogobbler_synctick *message);
void demogobbler_write_stop(writer *thisptr, demogobbler_stop *message);
void demogobbler_write_stringtables(writer *thisptr, demogobbler_stringtables *message);
void demogobbler_write_usercmd(writer *thisptr, demogobbler_usercmd *message);
void demogobbler_writer_free(writer *thisptr);

#ifdef __cplusplus
}
#endif
