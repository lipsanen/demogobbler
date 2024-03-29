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

enum dg_alloc_type { dg_alloc_temp, dg_alloc_permanent };
typedef enum dg_alloc_type dg_alloc_type;

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
  dg_alloc_type packet_alloc_type;
  bool parse_packetentities;
  void *client_state;
};

struct dg_parser {
  parser_state state;
  dg_settings m_settings;
  dg_parser_funcs _parser_funcs;
  dg_filereader m_reader;
  dg_demver_data demo_version;
  const char *error_message;
  bool error;
  bool parse_netmessages;
};

typedef struct dg_parser dg_parser;

#ifdef __cplusplus
}
#endif
