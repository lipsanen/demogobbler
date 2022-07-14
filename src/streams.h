#pragma once

#include <stdlib.h>

size_t fstream_read(void* stream, void* dest, size_t bytes);
int fstream_seek(void* stream, long int offset);
size_t fstream_write(void* stream, const void* src, size_t bytes);
