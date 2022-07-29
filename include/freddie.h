#pragma once


// Freddie is a higher level API for demo related stuff

#ifdef __cplusplus
extern "C" {
#endif

#include "demogobbler.h"
#include <stddef.h>

struct demogobbler_freddie_result {
  bool error;
  const char* error_message;
};

typedef struct demogobbler_freddie_result freddie_result;
freddie_result demogobbler_freddie_splice_demos(const char *output_path, const char** demo_paths, size_t demo_count);


#ifdef __cplusplus
}
#endif
