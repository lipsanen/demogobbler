#pragma once

#include "demogobbler_io.h"
#include "packettypes.h"
#include "stdbool.h"
#include "stdint.h"
#include "stdio.h"

struct filereader {
  void *m_pBuffer;
  int64_t m_uFileOffset;
  int64_t m_iBytesAvailable;
  int32_t m_iBufferOffset;
  bool eof;
  void *stream;
  input_interface input_funcs;
};

typedef struct filereader filereader;

void filereader_init(filereader *thisptr, void* stream, input_interface funcs);
void filereader_free(filereader *reader);
void filereader_readchunk(filereader *reader);
size_t filereader_readdata(filereader *thisptr, void *buffer, int bytes);
void filereader_skipbytes(filereader *thisptr, int bytes);
void filereader_skipto(filereader *thisptr, uint64_t offset);
int64_t filereader_current_position(filereader *thisptr);

static inline int32_t filereader_readint32(filereader *thisptr)
{
  int32_t val;
  int64_t bytesLeftInBuffer = (thisptr->m_iBytesAvailable - thisptr->m_iBufferOffset);
  
  // If enough bytes in buffer, take fast track
  if(bytesLeftInBuffer >= 4)
  {
    int32_t *src = (int32_t*)((uint8_t *)thisptr->m_pBuffer + thisptr->m_iBufferOffset);
    val = *src;
    thisptr->m_iBufferOffset += 4;
  }
  else
  {
    filereader_readdata(thisptr, &val, 4);
  }

  return val;
}

static inline uint8_t filereader_readbyte(filereader *thisptr)
{
  uint8_t val;
  int64_t bytesLeftInBuffer = (thisptr->m_iBytesAvailable - thisptr->m_iBufferOffset);
  
  // If enough bytes in buffer, take fast track
  if(bytesLeftInBuffer >= 1)
  {
    uint8_t *src = (uint8_t *)thisptr->m_pBuffer + thisptr->m_iBufferOffset;
    val = *src;
    thisptr->m_iBufferOffset += 1;
  }
  else
  {
    filereader_readdata(thisptr, &val, 1);
  }

  return val;
}

static inline float filereader_readfloat(filereader *thisptr) {
  float val;
  int64_t bytesLeftInBuffer = (thisptr->m_iBytesAvailable - thisptr->m_iBufferOffset);

  // If enough bytes in buffer, take fast track
  if(bytesLeftInBuffer >= 4)
  {
    float *src = (float*)((uint8_t *)thisptr->m_pBuffer + thisptr->m_iBufferOffset);
    val = *src;
    thisptr->m_iBufferOffset += 4;
  }
  else
  {
    filereader_readdata(thisptr, &val, 4);
  }
  return val;
}

static inline vector filereader_readvector(filereader *thisptr)
{
  vector val;
  val.x = filereader_readfloat(thisptr);
  val.y = filereader_readfloat(thisptr);
  val.z = filereader_readfloat(thisptr);

  return val;
}

