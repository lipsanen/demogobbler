#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "demogobbler/io.h"
#include "demogobbler/packettypes.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

struct dg_filereader {
  void *buffer;
  uint32_t buffer_size;
  uint32_t ufile_offset;
  uint32_t ibytes_available;
  uint32_t ibuffer_offset;
  bool eof;
  void *stream;
  dg_input_interface input_funcs;
};

typedef struct dg_filereader dg_filereader;

void dg_filereader_init(dg_filereader *thisptr, void* buffer, size_t buffer_size, void* stream, dg_input_interface funcs);
uint32_t dg_filereader_readdata(dg_filereader *thisptr, void *buffer, int bytes);
void dg_filereader_skipbytes(dg_filereader *thisptr, int bytes);
void dg_filereader_skipto(dg_filereader *thisptr, uint64_t offset);
uint8_t dg_filereader_readbyte(dg_filereader* thisptr);
int32_t dg_filereader_readint32(dg_filereader* thisptr);
float dg_filereader_readfloat(dg_filereader* thisptr);

#ifdef __cplusplus
}
#endif
