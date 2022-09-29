#pragma once

#include <cstdlib>

struct memory_stream
{
  size_t offset = 0;
  size_t buffer_size = 0;
  size_t file_size = 0;
  void* buffer = nullptr;
  bool agrees = true;
  memory_stream* ground_truth = nullptr;

  void* get_ptr();
  size_t get_bytes_left();
  void allocate_space(size_t bytes);
  void fill_with_file(const char* filepath);
  ~memory_stream();
};

// Function for interfacing with C
std::size_t memory_stream_read(void* stream, void* dest, size_t bytes);
int memory_stream_seek(void* stream, long int offset);
std::size_t memory_stream_write(void* stream, const void* src, size_t bytes);

typedef size_t (*demogobbler_input_read)(void* stream, void* dest, size_t bytes);
typedef int (*demogobbler_input_seek)(void* stream, long int offset);