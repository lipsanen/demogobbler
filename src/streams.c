#include "streams.h"
#include "stdio.h"

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
