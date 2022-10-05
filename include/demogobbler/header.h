#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef struct demogobbler_header demogobbler_header;

struct demogobbler_header {
  char ID[8];
  int32_t demo_protocol;
  int32_t net_protocol;
  char server_name[260];
  char client_name[260];
  char map_name[260];
  char game_directory[260];
  float seconds;
  int32_t tick_count;
  int32_t frame_count;
  int32_t signon_length;
};

#ifdef __cplusplus
}
#endif
