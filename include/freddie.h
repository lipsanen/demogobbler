#pragma once


// Freddie is a higher level API for demo related stuff

#ifdef __cplusplus
extern "C" {
#endif

#include "demogobbler.h"
#include "demogobbler_arena.h"
#include "packet_netmessages.h"
#include "packettypes.h"
#include <stddef.h>

demogobbler_parse_result freddie_splice_demos(const char *output_path, const char** demo_paths, size_t demo_count);

typedef void(*freddie_consume_string_func)(const char* str);
void freddie_consolecmd_tostring(demogobbler_consolecmd* message, freddie_consume_string_func func);
void freddie_customdata_tostring(demogobbler_customdata* message, freddie_consume_string_func func);
void freddie_datatables_tostring(demogobbler_datatables* message, freddie_consume_string_func func);
void freddie_netmessage_tostring(packet_net_message* message, freddie_consume_string_func func);
void freddie_packet_tostring(demogobbler_packet* message, freddie_consume_string_func func);
void freddie_stringtables_tostring(demogobbler_stringtables* message, freddie_consume_string_func func);
void freddie_synctick_tostring(demogobbler_synctick* message, freddie_consume_string_func func);
void freddie_usercmd_tostring(demogobbler_usercmd* message, freddie_consume_string_func func);

struct freddie_packet {
  demogobbler_message_preamble preamble;
  demogobbler_cmdinfo *cmdinfo;
  size_t cmdinfo_count;
  packet_net_message* net_messages;
  size_t net_messages_count;
};

typedef struct freddie_packet freddie_packet;

struct freddie_demo_message {
  enum demogobbler_type mtype;
  union {
    demogobbler_consolecmd consolecmd;
    demogobbler_customdata customdata;
    demogobbler_datatables datatables;
    freddie_packet packet;
    demogobbler_stringtables stringtables;
    demogobbler_synctick synctick;
    demogobbler_usercmd usercmd;
  };
};

typedef struct freddie_demo_message freddie_demo_message;

struct freddie_demo {
  demogobbler_header header;
  freddie_demo_message* messages;
  size_t message_count;
  arena memory_arena;
};

typedef struct freddie_demo freddie_demo;

demogobbler_parse_result freddie_parse_file(const char* filepath, freddie_demo* output);
void freddie_free_demo(freddie_demo* demo);

#ifdef __cplusplus
}
#endif
