#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

// For parsing
typedef size_t (*dg_input_read)(void *stream, void *dest, size_t bytes);
typedef int (*dg_input_seek)(void *stream, long int offset);

struct dg_input_interface {
  dg_input_read read;
  dg_input_seek seek;
};

typedef struct dg_input_interface input_interface;

// For writing
typedef size_t (*dg_output_write)(void *stream, const void *src, size_t bytes);

struct dg_output_interface {
  dg_output_write write;
};

typedef struct dg_output_interface output_interface;

#ifdef __cplusplus
}
#endif
