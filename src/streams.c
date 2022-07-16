#include "streams.h"
#include "utils.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

size_t fstream_read(void* stream, void* dest, size_t bytes)
{
  return fread(dest, 1, bytes, stream);
}

int fstream_seek(void* stream, long int offset)
{
  return fseek(stream, offset, SEEK_CUR);
}

size_t fstream_write(void* stream, const void* src, size_t bytes)
{
  return fwrite(src, 1, bytes, stream);
}

void buffer_stream_init(buffer_stream* thisptr, void* buffer, size_t size)
{
  thisptr->buffer = buffer;
  thisptr->offset = 0;
  thisptr->size = size;
}

size_t buffer_stream_read(void* ptr, void* dest, size_t bytes)
{
  buffer_stream* thisptr = ptr;
  if(bytes >= thisptr->size || bytes < 0)
  {
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

int buffer_stream_seek(void* ptr, long int offset)
{
  buffer_stream* thisptr = ptr;
  thisptr->offset += offset;
  thisptr->offset = CLAMP(0, thisptr->offset, thisptr->size);

  return 0;
}
