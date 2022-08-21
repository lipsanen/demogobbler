#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "demogobbler_io.h"
#include "demogobbler_datatables.h"
#include "header.h"
#include "packet_netmessages.h"
#include "packettypes.h"
#include <stdbool.h>
#include <stdio.h>

typedef struct {
  demogobbler_sendprop *props;
  size_t prop_count;
} flattened_props;

typedef struct {
  flattened_props* class_props;
  size_t classes_count;
} estate;

struct demogobbler_parser_state {
  void *client_state;
  estate entity_state;
  const char *error_message;
  bool error;
};

typedef struct demogobbler_parser_state parser_state;

typedef void (*func_demogobbler_handle_consolecmd)(parser_state *state,
                                                   demogobbler_consolecmd *ptr);
typedef void (*func_demogobbler_handle_customdata)(parser_state *state,
                                                   demogobbler_customdata *ptr);
typedef void (*func_demogobbler_handle_datatables)(parser_state *state,
                                                   demogobbler_datatables *ptr);
typedef void (*func_demogobbler_handle_demo_version)(parser_state *state,
                                                     demo_version_data version);
typedef void (*func_demogobbler_handle_header)(parser_state *state, demogobbler_header *header);
typedef void (*func_demogobbler_handle_packet)(parser_state *state, demogobbler_packet *ptr);
typedef void (*func_demogobbler_handle_synctick)(parser_state *state, demogobbler_synctick *ptr);
typedef void (*func_demogobbler_handle_stop)(parser_state *state, demogobbler_stop *ptr);
typedef void (*func_demogobbler_handle_stringtables)(parser_state *state,
                                                     demogobbler_stringtables *header);
typedef void (*func_demogobbler_handle_usercmd)(parser_state *state, demogobbler_usercmd *ptr);
typedef void (*func_demogobbler_handle_packet_net_message)(parser_state *state,
                                                           packet_net_message *message);
typedef void (*func_demogobbler_handle_datatables_parsed)(parser_state *state,
                                                           demogobbler_datatables_parsed *message);
typedef void (*func_demogobbler_handle_entity_state_init)(parser_state *state);

struct demogobbler_settings {
  func_demogobbler_handle_consolecmd consolecmd_handler;
  func_demogobbler_handle_customdata customdata_handler;
  func_demogobbler_handle_datatables datatables_handler;
  func_demogobbler_handle_datatables_parsed datatables_parsed_handler;
  func_demogobbler_handle_demo_version demo_version_handler;
  func_demogobbler_handle_entity_state_init entity_state_init_handler; // Called after parsing prop flattening stuff
  func_demogobbler_handle_header header_handler;
  func_demogobbler_handle_packet packet_handler;
  func_demogobbler_handle_synctick synctick_handler;
  func_demogobbler_handle_stop stop_handler;
  func_demogobbler_handle_stringtables stringtables_handler;
  func_demogobbler_handle_usercmd usercmd_handler;
  func_demogobbler_handle_packet_net_message packet_net_message_handler;
  bool store_ents;
  void *client_state;
};

typedef struct demogobbler_settings demogobbler_settings;

void demogobbler_settings_init(demogobbler_settings *settings);
demo_version_data demogobbler_get_demo_version(demogobbler_header *header);

struct demogobbler_parse_result {
  const char *error_message;
  bool error;
};

typedef struct demogobbler_parse_result demogobbler_parse_result;

demogobbler_parse_result demogobbler_parse_file(demogobbler_settings *settings,
                                                       const char *filepath);
demogobbler_parse_result demogobbler_parse_buffer(demogobbler_settings *settings,
                                                         void *buffer, size_t size);
demogobbler_parse_result demogobbler_parse(demogobbler_settings *settings, void *stream,
                                                  input_interface input_interface);

struct demogobbler_writer {
  void *_stream;
  const char *error_message;
  output_interface output_funcs;
  bool error;
  bool _custom_stream;
  demo_version_data version;
};

typedef struct demogobbler_writer writer;

void demogobbler_writer_init(writer *thisptr);
void demogobbler_writer_open_file(writer *thisptr, const char *filepath);
void demogobbler_writer_open(writer *thisptr, void *stream, output_interface output_interface);
void demogobbler_writer_close(writer *thisptr);
void demogobbler_write_consolecmd(writer *thisptr, demogobbler_consolecmd *message);
void demogobbler_write_customdata(writer *thisptr, demogobbler_customdata *message);
void demogobbler_write_datatables(writer *thisptr, demogobbler_datatables *message);
void demogobbler_write_datatables_parsed(writer* thisptr, demogobbler_datatables_parsed* datatables);
void demogobbler_write_header(writer *thisptr, demogobbler_header *message);
void demogobbler_write_packet(writer *thisptr, demogobbler_packet *message);
void demogobbler_write_preamble(writer* thisptr, demogobbler_message_preamble preamble);
void demogobbler_write_synctick(writer *thisptr, demogobbler_synctick *message);
void demogobbler_write_stop(writer *thisptr, demogobbler_stop *message);
void demogobbler_write_stringtables(writer *thisptr, demogobbler_stringtables *message);
void demogobbler_write_usercmd(writer *thisptr, demogobbler_usercmd *message);
void demogobbler_writer_free(writer *thisptr);

#ifdef __cplusplus
}
#endif
