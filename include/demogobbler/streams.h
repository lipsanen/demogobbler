#pragma once


#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

void* dg_fstream_init(const char* filepath, const char* modes);
size_t dg_fstream_read(void* stream, void* dest, size_t bytes);
int dg_fstream_seek(void* stream, long int offset);
size_t dg_fstream_write(void* stream, const void* src, size_t bytes);
void dg_fstream_free(void* stream);

struct buffer_stream
{
  void* buffer;
  size_t size;
  uint32_t offset;
  bool overflow;
};

typedef struct buffer_stream buffer_stream;

void dg_buffer_stream_init(buffer_stream* thisptr, void* buffer, size_t size);
uint8_t dg_buffer_stream_read_byte(buffer_stream* thisptr);
uint16_t dg_buffer_stream_read_short(buffer_stream* thisptr);
const char* dg_buffer_stream_read_string(buffer_stream* thisptr);
size_t dg_buffer_stream_read(void* ptr, void* dest, size_t bytes);
int dg_buffer_stream_seek(void* ptr, long int offset);

#ifdef __cplusplus
}
#endif
