#pragma once

#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"

struct filereader {
  void *m_pBuffer;
  int64_t m_uFileOffset;
  int64_t m_iBytesAvailable;
  int32_t m_iBufferOffset;
  bool eof;
  FILE *m_pStream;
};

typedef struct filereader filereader;

void filereader_init(filereader *reader, FILE *stream);
void filereader_free(filereader *reader);
void filereader_readchunk(filereader *reader);
void filereader_readdata(filereader *thisptr, void *buffer, int bytes);
float filereader_readfloat(filereader *thisptr);
int32_t filereader_readint32(filereader *thisptr);
uint8_t filereader_readbyte(filereader *thisptr);
void filereader_skipbytes(filereader *thisptr, int bytes);
void filereader_skipto(filereader *thisptr, uint64_t offset);
int64_t filereader_current_position(filereader *thisptr);
