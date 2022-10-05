#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "demogobbler/vector.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

enum demogobbler_type {
  demogobbler_type_signon = 1,
  demogobbler_type_packet = 2,
  demogobbler_type_synctick = 3,
  demogobbler_type_consolecmd = 4,
  demogobbler_type_usercmd = 5,
  demogobbler_type_datatables = 6,
  demogobbler_type_stop = 7,
  demogobbler_type_customdata = 8,
  demogobbler_type_stringtables = 9
  // These arent necessarily 1 : 1 with the number in the demo
  // Customdata is 8 and Stringtables is either 8 or 9 depending on version
};

struct demogobbler_message_preamble {
  uint8_t type;
  int32_t tick;
  uint8_t slot;
  enum demogobbler_type converted_type;
};

typedef struct demogobbler_message_preamble demogobbler_message_preamble;

struct demogobbler_cmdinfo_raw {
  int32_t data[19];
};

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
  union {
    struct demogobbler_cmdinfo_raw cmdinfo_raw[4];
    demogobbler_cmdinfo cmdinfo[4]; // allocates memory enough for all games
  };
  size_t cmdinfo_size;
  int32_t in_sequence;
  int32_t out_sequence;
  int32_t size_bytes;
  void *data;
};

typedef struct demogobbler_packet demogobbler_packet;

struct demogobbler_consolecmd {
  demogobbler_message_preamble preamble;
  int32_t size_bytes;
  char *data;
};

typedef struct demogobbler_consolecmd demogobbler_consolecmd;

struct demogobbler_usercmd {
  demogobbler_message_preamble preamble;
  int32_t cmd;
  int32_t size_bytes;
  void *data;
};

typedef struct demogobbler_usercmd demogobbler_usercmd;

struct demogobbler_datatables {
  demogobbler_message_preamble preamble;
  int32_t size_bytes;
  void *data;
};

typedef struct demogobbler_datatables demogobbler_datatables;

struct demogobbler_stop {
  int32_t size_bytes; // Inferred, not in the demo
  void *data;
};

typedef struct demogobbler_stop demogobbler_stop;

struct demogobbler_synctick {
  demogobbler_message_preamble preamble;
};

typedef struct demogobbler_synctick demogobbler_synctick;

struct demogobbler_customdata {
  demogobbler_message_preamble preamble;
  int32_t unknown;
  int32_t size_bytes;
  void *data;
};

typedef struct demogobbler_customdata demogobbler_customdata;

struct demogobbler_stringtables {
  demogobbler_message_preamble preamble;
  int32_t size_bytes;
  void *data;
};

typedef struct demogobbler_stringtables demogobbler_stringtables;

#ifdef __cplusplus
}
#endif
