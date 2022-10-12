#pragma once

#include "demogobbler/io.h"
#include "demogobbler/packettypes.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

struct filereader {
  void *buffer;
  uint32_t buffer_size;
  uint32_t ufile_offset;
  uint32_t ibytes_available;
  uint32_t ibuffer_offset;
  bool eof;
  void *stream;
  input_interface input_funcs;
};

typedef struct filereader filereader;

void filereader_init(filereader *thisptr, void* buffer, size_t buffer_size, void* stream, input_interface funcs);
uint32_t filereader_readdata(filereader *thisptr, void *buffer, int bytes);
void filereader_skipbytes(filereader *thisptr, int bytes);
void filereader_skipto(filereader *thisptr, uint64_t offset);
uint8_t filereader_readbyte(filereader* thisptr);
int32_t filereader_readint32(filereader* thisptr);
float filereader_readfloat(filereader* thisptr);
