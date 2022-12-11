#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "demogobbler/allocator.h"
#include "demogobbler/datatable_types.h"
#include "demogobbler/entity_types.h"
#include "demogobbler/filereader.h"
#include "demogobbler/packettypes.h"
#include "demogobbler/parser_types.h"
#include "demogobbler/stringtable_types.h"
#include "stdbool.h"
#include "stdio.h"

#ifdef DEBUG
#define DEBUG_BREAK_PROP 1
extern int DG_CURRENT_DEBUG_INDEX;
#endif

struct dg_parser;

// This API is only necessary when you need to change how the parser actually works
// The only use case for this is to extend/change the demo format to your own format
typedef void (*func_dg_parse)(struct dg_parser *thisptr);
typedef void (*func_dg_parse_typed)(struct dg_parser *thisptr, enum dg_type type);

typedef struct {
  func_dg_parse parse_consolecmd;
  func_dg_parse parse_customdata;
  func_dg_parse parse_datatables;
  func_dg_parse_typed parse_packet;
  func_dg_parse parse_stop;
  func_dg_parse_typed parse_stringtables;
  func_dg_parse parse_synctick;
  func_dg_parse parse_usercmd;
} dg_parser_funcs;

typedef struct dg_parser_state parser_state;
struct packet_parsed;
struct dg_header;

typedef void (*func_dg_consolecmd)(parser_state *state, dg_consolecmd *ptr);
typedef void (*func_dg_customdata)(parser_state *state, dg_customdata *ptr);
typedef void (*func_dg_datatables)(parser_state *state, dg_datatables *ptr);
typedef void (*func_dg_demover)(parser_state *state, dg_demver_data version);
typedef void (*func_dg_header)(parser_state *state, struct dg_header *header);
typedef void (*func_dg_packet)(parser_state *state, dg_packet *ptr);
typedef void (*func_dg_packet_parsed)(parser_state *state, struct packet_parsed *ptr);
typedef void (*func_dg_synctick)(parser_state *state, dg_synctick *ptr);
typedef void (*func_dg_stop)(parser_state *state, dg_stop *ptr);
typedef void (*func_dg_stringtables)(parser_state *state, dg_stringtables *header);
typedef void (*func_dg_stringtables_parsed)(parser_state *state, dg_stringtables_parsed *message);
typedef void (*func_dg_usercmd)(parser_state *state, dg_usercmd *ptr);
typedef void (*func_dg_datatables_parsed)(parser_state *state, dg_datatables_parsed *message);
typedef void (*func_dg_packetentities_parsed)(parser_state *state,
                                              dg_svc_packetentities_parsed *message);
typedef void (*func_dg_estate_init)(parser_state *state);
typedef struct dg_settings dg_settings;

// The settings struct contains all the callbacks for application code
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
  dg_parser_funcs funcs;
  dg_alloc_state temp_alloc_state;
  dg_alloc_state permanent_alloc_state;
  bool store_ents;
  bool store_props;
  void *client_state;
};

typedef struct {
  dg_pes excluded_props;
  dg_hashtable dts_with_excludes;
  dg_hashtable dt_hashtable;
} entity_parse_scrap;

struct dg_parser {
  parser_state state;
  dg_settings m_settings;
  dg_parser_funcs _parser_funcs;
  dg_filereader m_reader;
  dg_demver_data demo_version;
  const char *error_message;
  bool error;
  bool parse_netmessages;
  entity_parse_scrap ent_scrap;
};

typedef struct dg_parser dg_parser;

void dg_parser_init(dg_parser *thisptr, dg_settings *settings);
void dg_parser_arena_check_init(dg_parser *thisptr);
void dg_parser_parse(dg_parser *thisptr, void *stream, dg_input_interface input);
void dg_parser_update_l4d2_version(dg_parser *thisptr, int l4d2_version);
dg_alloc_state* dg_parser_temp_allocator(dg_parser *thisptr);
dg_alloc_state* dg_parser_perm_allocator(dg_parser *thisptr);

#ifdef __cplusplus
}
#endif
