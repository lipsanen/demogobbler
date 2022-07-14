#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

enum demogobbler_type {
  demogobbler_type_signon = 1,
  demogobbler_type_packet = 2,
  demogobbler_type_synctick = 3,
  demogobbler_type_consolecmd = 4,
  demogobbler_type_usercmd = 5,
  demogobbler_type_datatables = 6,
  demogobbler_type_stop = 7
  // Customdata is 8
  // Stringtables is either 8 or 9 depending on version
};

struct demogobbler_vector {
  float x, y, z;
};

typedef struct demogobbler_vector vector;

struct demogobbler_message_preamble {
  uint8_t type;
  int32_t tick;
  uint8_t slot;
};

typedef struct demogobbler_message_preamble demogobbler_message_preamble;

struct demogobbler_cmdinfo {
  int32_t interp_flags;
  vector view_origin;
  vector view_angles;
  vector local_viewangles;
  vector view_origin2;
  vector view_angles2;
  vector local_viewangles2;
};

typedef struct demogobbler_cmdinfo demogobbler_cmdinfo;

struct demogobbler_packet {
  demogobbler_message_preamble preamble;
  demogobbler_cmdinfo cmdinfo[2]; // 2 for games with split screen, we'll just
                                  // leave it unused for other games
  int32_t in_sequence;
  int32_t out_sequence;
  int32_t size_bytes;
  void *data;
};

typedef struct demogobbler_packet demogobbler_packet;
typedef void (*func_demogobbler_handle_packet)(demogobbler_packet *ptr);

struct demogobbler_consolecmd {
  demogobbler_message_preamble preamble;
  int32_t size_bytes;
  const char *data;
};

typedef struct demogobbler_consolecmd demogobbler_consolecmd;
typedef void (*func_demogobbler_handle_consolecmd)(demogobbler_consolecmd *ptr);

struct demogobbler_usercmd {
  demogobbler_message_preamble preamble;
  int32_t cmd;
  int32_t size_bytes;
  void *data;
};

typedef struct demogobbler_usercmd demogobbler_usercmd;
typedef void (*func_demogobbler_handle_usercmd)(demogobbler_usercmd *ptr);

struct demogobbler_datatables {
  demogobbler_message_preamble preamble;
  int32_t size_bytes;
  void *data;
};

typedef struct demogobbler_datatables demogobbler_datatables;
typedef void (*func_demogobbler_handle_datatables)(demogobbler_datatables *ptr);

struct demogobbler_stop {
  int32_t size_bytes; // Inferred, not in the demo
  void *data;
};

typedef struct demogobbler_stop demogobbler_stop;
typedef void (*func_demogobbler_handle_stop)(demogobbler_stop *ptr);


struct demogobbler_synctick {
  demogobbler_message_preamble preamble;
};

typedef struct demogobbler_synctick demogobbler_synctick;
typedef void (*func_demogobbler_handle_synctick)(demogobbler_synctick *ptr);

struct demogobbler_customdata {
  demogobbler_message_preamble preamble;
  int32_t unknown;
  int32_t size_bytes;
  void *data;
};

typedef struct demogobbler_customdata demogobbler_customdata;
typedef void (*func_demogobbler_handle_customdata)(demogobbler_customdata *ptr);

struct demogobbler_stringtables {
  demogobbler_message_preamble preamble;
  int32_t size_bytes;
  void *data;
};

typedef struct demogobbler_stringtables demogobbler_stringtables;
typedef void (*func_demogobbler_handle_stringtables)(demogobbler_stringtables *header);

#ifdef __cplusplus
}
#endif
