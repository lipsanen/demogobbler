#pragma once


// Freddie is a higher level API for demo related stuff
// I had more stuff here but ended up throwing it in the dumpster

#ifdef __cplusplus
extern "C" {
#endif

#include "demogobbler.h"
#include <stddef.h>

demogobbler_parse_result freddie_splice_demos(const char *output_path, const char** demo_paths, size_t demo_count);

#ifdef __cplusplus
}
#endif
