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

struct freddie_packet {
  demogobbler_message_preamble preamble;
  demogobbler_cmdinfo *cmdinfo;
  size_t cmdinfo_size;
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
    demogobbler_stop stop;
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

typedef void(*freddie_consume_string_func)(const char* str);
void freddie_header_title(demogobbler_header* header, freddie_consume_string_func func);
void freddie_header_internals(demogobbler_header* header, freddie_consume_string_func func);
void freddie_message_title(freddie_demo_message* message, freddie_consume_string_func func);
void freddie_message_internals(freddie_demo_message* message, freddie_consume_string_func func);
void freddie_netmessage_title(packet_net_message* message, freddie_consume_string_func func);
void freddie_netmessage_internals(packet_net_message* message, freddie_consume_string_func func);

#ifdef __cplusplus
}
#endif
