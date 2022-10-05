#pragma once

#include "demogobbler/arena.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

size_t fstream_read(void* stream, void* dest, size_t bytes);
int fstream_seek(void* stream, long int offset);
size_t fstream_write(void* stream, const void* src, size_t bytes);


struct buffer_stream
{
  void* buffer;
  size_t size;
  uint32_t offset;
  bool overflow;
};

typedef struct buffer_stream buffer_stream;

void buffer_stream_init(buffer_stream* thisptr, void* buffer, size_t size);
uint8_t buffer_stream_read_byte(buffer_stream* thisptr);
uint16_t buffer_stream_read_short(buffer_stream* thisptr);
const char* buffer_stream_read_string(buffer_stream* thisptr);
size_t buffer_stream_read(void* ptr, void* dest, size_t bytes);
int buffer_stream_seek(void* ptr, long int offset);
