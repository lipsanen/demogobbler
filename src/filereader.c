#include "filereader.h"
#include "streams.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>

const int64_t FILEREADER_BUFFER_SIZE = 1 << 15;

void filereader_readchunk(filereader *thisptr) {
  size_t rval = thisptr->input_funcs.read(thisptr->stream, thisptr->m_pBuffer, FILEREADER_BUFFER_SIZE);

  if (rval <= 0) {
    thisptr->eof = true;
    return;
  }

  thisptr->m_iBufferOffset = 0;
  thisptr->m_iBytesAvailable = rval;
  thisptr->m_uFileOffset += rval;
}

int64_t filereader_bytesleftinbuffer(filereader *thisptr) {
  if (thisptr->m_iBytesAvailable < thisptr->m_iBufferOffset) {
    return 0;
  } else {
    return thisptr->m_iBytesAvailable - thisptr->m_iBufferOffset;
  }
}

void filereader_init(filereader *thisptr, void* stream, input_interface funcs) {
  memset(thisptr, 0, sizeof(filereader));
  thisptr->m_pBuffer = malloc(FILEREADER_BUFFER_SIZE);
  thisptr->stream = stream;
  thisptr->input_funcs = funcs;
}

void filereader_free(filereader *thisptr) {
  free(thisptr->m_pBuffer);
}

size_t filereader_readdata(filereader *thisptr, void *buffer, int bytes) {
  char *dest = buffer;
  int bytesLeftToRead = bytes;
  size_t totalBytesRead = 0;

  do {
    char *src = (char *)thisptr->m_pBuffer + thisptr->m_iBufferOffset;
    int64_t bytesLeftInBuffer = (thisptr->m_iBytesAvailable - thisptr->m_iBufferOffset);
    int bytesRead = MIN(bytesLeftToRead, bytesLeftInBuffer);

    if (bytesRead > 0) {
      memcpy(dest, src, bytesRead);
      dest += bytesRead;
      bytesLeftToRead -= bytesRead;
      filereader_skipbytes(thisptr, bytesRead);
      totalBytesRead += bytesRead;
    }

    if (bytesLeftToRead > 0) {
      filereader_readchunk(thisptr);
    }

  } while (bytesLeftToRead > 0 && !thisptr->eof);

  return totalBytesRead;
}

void filereader_skipbytes(filereader *thisptr, int bytes) {
  // Going backwards not supported
  if (bytes < 0) {
    return;
  }

  int64_t bytesLeftInBuffer = (thisptr->m_iBytesAvailable - thisptr->m_iBufferOffset);

  if (bytes < bytesLeftInBuffer) {
    thisptr->m_iBufferOffset += bytes;
  } else {
    thisptr->m_iBufferOffset = thisptr->m_iBytesAvailable;
    bytes -= bytesLeftInBuffer;
    thisptr->input_funcs.seek(thisptr->stream, bytes);
    thisptr->m_uFileOffset += bytes;
  }
}

void filereader_skipto(filereader *thisptr, uint64_t offset) {
  int64_t curOffset = filereader_current_position(thisptr);
  filereader_skipbytes(thisptr, offset - curOffset);
}

int64_t filereader_current_position(filereader *thisptr) {
  int64_t bytesLeftInBuffer = (thisptr->m_iBytesAvailable - thisptr->m_iBufferOffset);
  return thisptr->m_uFileOffset - bytesLeftInBuffer;
}
