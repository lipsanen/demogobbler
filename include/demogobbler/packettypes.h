#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "demogobbler/vector.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

enum dg_type {
  dg_type_signon = 1,
  dg_type_packet = 2,
  dg_type_synctick = 3,
  dg_type_consolecmd = 4,
  dg_type_usercmd = 5,
  dg_type_datatables = 6,
  dg_type_stop = 7,
  dg_type_customdata = 8,
  dg_type_stringtables = 9
  // These arent necessarily 1 : 1 with the number in the demo
  // Customdata is 8 and Stringtables is either 8 or 9 depending on version
};

struct dg_message_preamble {
  uint8_t type;
  int32_t tick;
  uint8_t slot;
  enum dg_type converted_type;
};

typedef struct dg_message_preamble dg_message_preamble;

struct dg_cmdinfo_raw {
  int32_t data[19];
};

struct dg_cmdinfo {
  int32_t interp_flags;
  vector view_origin;
  vector view_angles;
  vector local_viewangles;
  vector view_origin2;
  vector view_angles2;
  vector local_viewangles2;
};

typedef struct dg_cmdinfo dg_cmdinfo;

struct dg_packet {
  dg_message_preamble preamble;
  union {
    struct dg_cmdinfo_raw cmdinfo_raw[4];
    dg_cmdinfo cmdinfo[4]; // allocates memory enough for all games
  };
  size_t cmdinfo_size;
  int32_t in_sequence;
  int32_t out_sequence;
  int32_t size_bytes;
  void *data;
};

typedef struct dg_packet dg_packet;

struct dg_consolecmd {
  dg_message_preamble preamble;
  int32_t size_bytes;
  char *data;
};

typedef struct dg_consolecmd dg_consolecmd;

struct dg_usercmd {
  dg_message_preamble preamble;
  int32_t cmd;
  int32_t size_bytes;
  void *data;
};

typedef struct dg_usercmd dg_usercmd;

struct dg_datatables {
  dg_message_preamble preamble;
  int32_t size_bytes;
  void *data;
};

typedef struct dg_datatables dg_datatables;

struct dg_stop {
  int32_t size_bytes; // Inferred, not in the demo
  void *data;
};

typedef struct dg_stop dg_stop;

struct dg_synctick {
  dg_message_preamble preamble;
};

typedef struct dg_synctick dg_synctick;

struct dg_customdata {
  dg_message_preamble preamble;
  int32_t unknown;
  int32_t size_bytes;
  void *data;
};

typedef struct dg_customdata dg_customdata;

struct dg_stringtables {
  dg_message_preamble preamble;
  int32_t size_bytes;
  void *data;
};

typedef struct dg_stringtables dg_stringtables;

#ifdef __cplusplus
}
#endif
