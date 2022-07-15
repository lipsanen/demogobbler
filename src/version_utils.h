#pragma once

#include "demogobbler.h"

static inline bool version_has_slot_in_preamble(demo_version version) {
  if(version == portal2 || version == csgo || version == l4d || version == l4d2) {
    return true;
  }
  else {
    return false;
  }
}

static inline int version_cmdinfo_size(demo_version version) {
  if (version == l4d || version == l4d2) {
    return 4;
  }
  else if (version == portal2 || version == csgo) {
    return 2;
  } else {
    return 1;
  }
}

demo_version get_demo_version(demogobbler_header* header);
