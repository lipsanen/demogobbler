#include "memory_stream.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>

void* memory_stream::get_ptr()
{
  u_int8_t* ptr = (u_int8_t*)this->buffer;
  return ptr + this->offset;
}

std::size_t memory_stream::get_bytes_left()
{
  if(this->offset > this->file_size)
  {
    return 0;
  }
  else
  {
    return this->file_size - this->offset;
  }
}

void memory_stream::allocate_space(size_t bytes)
{
  if(bytes + this->offset > this->buffer_size)
  {
    std::size_t next_buffer_size = std::max(bytes + this->offset, this->buffer_size * 2);
    if(this->buffer)
    {
      this->buffer = realloc(this->buffer, next_buffer_size);
    }
    else
    {
      this->buffer = malloc(next_buffer_size);
    }

    this->buffer_size = next_buffer_size;
  }
}

void memory_stream::fill_with_file(const char* filepath)
{
  FILE* stream = fopen(filepath, "rb");
  const int read_size = 4096;
  bool eof = false;

  while(!eof)
  {
    this->allocate_space(read_size);
    auto ptr = this->get_ptr();
    auto readIt = fread(ptr, 1, read_size, stream);
    eof = readIt != read_size;
    this->offset += readIt;
  }

  this->file_size = offset;
  this->offset = 0; // Set to beginning of file
  fclose(stream);
}

memory_stream::~memory_stream()
{
  if(this->buffer)
  {
    free(this->buffer);
  }
}

std::size_t memory_stream_read(void* s, void* dest, size_t bytes)
{
  memory_stream* stream = reinterpret_cast<memory_stream*>(s);

  if(stream->offset > stream->file_size)
  {
    return 0;
  }
  else
  {
    auto ptr = stream->get_ptr();
    bytes = std::min(stream->get_bytes_left(), bytes);
    memcpy(dest, ptr, bytes);
    stream->offset += bytes;
    return bytes;
  }

}

int memory_stream_seek(void* s, long int offset)
{
  memory_stream* stream = reinterpret_cast<memory_stream*>(s);
  stream->offset += offset;

  return 0;
}

std::size_t memory_stream_write(void* s, const void* src, size_t bytes)
{
  memory_stream* stream = reinterpret_cast<memory_stream*>(s);
  stream->allocate_space(bytes);
  auto dest = stream->get_ptr();
  memcpy(dest, src, bytes);
  stream->offset += bytes;
  stream->file_size += bytes;

  return bytes;
}

typedef size_t (*demogobbler_input_read)(void* stream, void* dest, size_t bytes);
typedef int (*demogobbler_input_seek)(void* stream, long int offset);