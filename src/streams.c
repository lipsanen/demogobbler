#include "demogobbler/allocator.h"
#include "demogobbler/streams.h"
#include "utils.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

void* dg_fstream_init(const char* filepath, const char* modes)
{
  return fopen(filepath, modes);
}

void dg_fstream_free(void* stream)
{
  fclose(stream);
}

size_t dg_fstream_read(void* stream, void* dest, size_t bytes)
{
  return fread(dest, 1, bytes, stream);
}

int dg_fstream_seek(void* stream, long int offset)
{
  return fseek(stream, offset, SEEK_CUR);
}

size_t dg_fstream_write(void* stream, const void* src, size_t bytes)
{
  size_t out = fwrite(src, 1, bytes, stream);
  return out;
}

void dg_buffer_stream_init(buffer_stream* thisptr, void* buffer, size_t size)
{
  thisptr->buffer = buffer;
  thisptr->offset = 0;
  thisptr->size = size;
}

size_t dg_buffer_stream_read(void* ptr, void* dest, size_t bytes)
{
  buffer_stream* thisptr = ptr;
  if(bytes >= thisptr->size || bytes < 0)
  {
    thisptr->overflow = true;
    return 0;
  }
  else
  {
    size_t read = MIN(thisptr->size - thisptr->offset, bytes);
    uint8_t* src = (uint8_t*)thisptr->buffer + thisptr->offset;
    memcpy(dest, src, read);
    thisptr->offset += read;
    return read;
  }
}

int dg_buffer_stream_seek(void* ptr, long int offset)
{
  buffer_stream* thisptr = ptr;
  thisptr->offset += offset;

  if(thisptr->offset > thisptr->size) {
    thisptr->overflow = true;
    thisptr->offset = thisptr->size;
  }

  return 0;
}


uint8_t dg_buffer_stream_read_byte(buffer_stream* thisptr)
{
  uint8_t rval;
  dg_buffer_stream_read(thisptr, &rval, 1);
  return rval;
}

uint16_t dg_buffer_stream_read_short(buffer_stream* thisptr)
{
  uint16_t rval;
  dg_buffer_stream_read(thisptr, &rval, 2);
  return rval;
}

const char* dg_buffer_stream_read_string(buffer_stream* thisptr)
{
  char* start = (char*)thisptr->buffer + thisptr->offset;
  char* str = start;

  while(*str != '\0' && (str - (char*)thisptr->buffer) < thisptr->size) {
    ++str;
  }

  thisptr->offset = str - (char*)thisptr->buffer;
  return start;
}
