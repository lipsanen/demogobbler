#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

// For parsing
typedef size_t (*demogobbler_input_read)(void* stream, void* dest, size_t bytes);
typedef int (*demogobbler_input_seek)(void* stream, long int offset);

struct demogobbler_input_interface
{
  demogobbler_input_read read;
  demogobbler_input_seek seek;
};

typedef struct demogobbler_input_interface input_interface;

// For writing
typedef size_t (*demogobbler_output_write)(void* stream, const void* src, size_t bytes);

struct demogobbler_output_interface
{
  demogobbler_output_write write;
};

typedef struct demogobbler_output_interface output_interface;

#ifdef __cplusplus
}
#endif
