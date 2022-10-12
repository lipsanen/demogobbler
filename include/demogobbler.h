#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "demogobbler/arena.h"
#include "demogobbler/bitwriter.h"
#include "demogobbler/datatable_types.h"
#include "demogobbler/entity_types.h"
#include "demogobbler/header.h"
#include "demogobbler/io.h"
#include "demogobbler/packet_netmessages.h"
#include "demogobbler/packettypes.h"
#include "demogobbler/parser_types.h"
#include "demogobbler/stringtable_types.h"
#include <stdbool.h>
#include <stdio.h>

typedef struct dg_parser_state parser_state;

typedef void (*func_dg_consolecmd)(parser_state *state, dg_consolecmd *ptr);
typedef void (*func_dg_customdata)(parser_state *state, dg_customdata *ptr);
typedef void (*func_dg_datatables)(parser_state *state, dg_datatables *ptr);
typedef void (*func_dg_demover)(parser_state *state, dg_demver_data version);
typedef void (*func_dg_header)(parser_state *state, dg_header *header);
typedef void (*func_dg_packet)(parser_state *state, dg_packet *ptr);
typedef void (*func_dg_packet_parsed)(parser_state *state, packet_parsed *ptr);
typedef void (*func_dg_synctick)(parser_state *state, dg_synctick *ptr);
typedef void (*func_dg_stop)(parser_state *state, dg_stop *ptr);
typedef void (*func_dg_stringtables)(parser_state *state, dg_stringtables *header);
typedef void (*func_dg_stringtables_parsed)(parser_state *state, dg_stringtables_parsed *message);
typedef void (*func_dg_usercmd)(parser_state *state, dg_usercmd *ptr);
typedef void (*func_dg_datatables_parsed)(parser_state *state, dg_datatables_parsed *message);
typedef void (*func_dg_packetentities_parsed)(parser_state *state,
                                              dg_svc_packetentities_parsed *message);
typedef void (*func_dg_estate_init)(parser_state *state);

struct dg_settings {
  func_dg_consolecmd consolecmd_handler;
  func_dg_customdata customdata_handler;
  func_dg_datatables datatables_handler;
  func_dg_datatables_parsed datatables_parsed_handler;
  func_dg_packetentities_parsed packetentities_parsed_handler;
  func_dg_demover demo_version_handler;
  func_dg_estate_init flattened_props_handler; // Called after parsing prop flattening stuff
  func_dg_header header_handler;
  func_dg_packet packet_handler;
  func_dg_packet_parsed packet_parsed_handler;
  func_dg_synctick synctick_handler;
  func_dg_stop stop_handler;
  func_dg_stringtables stringtables_handler;
  func_dg_stringtables_parsed stringtables_parsed_handler;
  func_dg_usercmd usercmd_handler;
  bool store_ents;
  bool store_props;
  void *client_state;
};

typedef struct dg_settings dg_settings;

void dg_settings_init(dg_settings *settings);
dg_demver_data dg_get_demo_version(dg_header *header);

struct dg_parse_result {
  const char *error_message;
  bool error;
};

typedef struct dg_parse_result dg_parse_result;

dg_parse_result dg_parse_file(dg_settings *settings, const char *filepath);
dg_parse_result dg_parse_buffer(dg_settings *settings, void *buffer, size_t size);
dg_parse_result dg_parse(dg_settings *settings, void *stream, input_interface input_interface);

struct dg_writer {
  void *_stream;
  const char *error_message;
  output_interface output_funcs;
  bool error;
  bool _custom_stream;
  dg_demver_data version;
  struct dg_bitwriter bitwriter;
};

typedef struct dg_writer writer;

void dg_writer_init(writer *thisptr);
void dg_writer_open_file(writer *thisptr, const char *filepath);
void dg_writer_open(writer *thisptr, void *stream, output_interface output_interface);
void dg_writer_close(writer *thisptr);
void dg_write_consolecmd(writer *thisptr, dg_consolecmd *message);
void dg_write_customdata(writer *thisptr, dg_customdata *message);
void dg_write_datatables(writer *thisptr, dg_datatables *message);
void dg_write_datatables_parsed(writer *thisptr, dg_datatables_parsed *datatables);
void dg_write_header(writer *thisptr, dg_header *message);
void dg_write_packet(writer *thisptr, dg_packet *message);
void dg_write_packet_parsed(writer *thisptr, packet_parsed *message);
void dg_write_preamble(writer *thisptr, dg_message_preamble preamble);
void dg_write_synctick(writer *thisptr, dg_synctick *message);
void dg_write_stop(writer *thisptr, dg_stop *message);
void dg_write_stringtables(writer *thisptr, dg_stringtables *message);
void dg_write_stringtables_parsed(writer *thisptr, dg_stringtables_parsed *message);
void dg_write_usercmd(writer *thisptr, dg_usercmd *message);
void dg_writer_free(writer *thisptr);

typedef struct {
  dg_demver_data *version_data;
  dg_datatables_parsed *message;
  bool flatten_datatables;
  bool should_store_props;
} estate_init_args;

typedef struct {
  dg_arena *memory_arena;
  dg_stringtables *message;
} stringtable_parse_args;

typedef struct {
  dg_pes excluded_props;
  dg_hashtable dts_with_excludes;
  dg_hashtable dt_hashtable;
} entity_parse_scrap;

dg_datatables_parsed_rval dg_parse_datatables(dg_demver_data *state, dg_arena *a,
                                              dg_datatables *message);
dg_parse_result dg_estate_init(estate *thisptr, entity_parse_scrap *scrap, estate_init_args args);
dg_parse_result dg_parse_stringtables(dg_stringtables_parsed *out, stringtable_parse_args args);
dg_parse_result dg_estate_update(estate *entity_state, const dg_packetentities_data *data);
void dg_estate_free(estate *thisptr);

#ifdef __cplusplus
}
#endif
