#pragma once


// Freddie is a higher level API for parsing a demo.
// It returns a struct with various fields, rather than relying on callbacks.


#ifdef __cplusplus
extern "C" {
#endif

#include "demogobbler.h"
#include "dynamic_array.h"
#include "header.h"
#include <stdint.h>

struct demogobbler_freddie_consolecmd {
  int32_t tick;
  union {
    const char *cmd;
    int64_t __offset; // Only used during parsing
  };
};

typedef struct demogobbler_freddie_consolecmd freddie_consolecmd;

struct demogobbler_freddie {
  demogobbler_header header;      // Header with fixed tick count
  demogobbler_cmdinfo *cmdinfo;   // Array to command infos from player 0 (no split screen support)
  size_t cmdinfo_count;           // number of command infos
  freddie_consolecmd *consolecmd; // Array of console commands
  size_t consolecmd_count;        // Number of console commands

  dynamic_array _memory_chunk;
  const char *error_message;
  bool error;
};

typedef struct demogobbler_freddie demogobbler_freddie;

demogobbler_freddie demogobbler_freddie_file(const char *filepath);
demogobbler_freddie demogobbler_freddie_buffer(void *buffer, size_t buffer_size);
void demogobbler_freddie_free(demogobbler_freddie *demo);

#ifdef __cplusplus
}
#endif
