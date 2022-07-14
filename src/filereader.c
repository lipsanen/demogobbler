#include "filereader.h"
#include "stdlib.h"
#include "string.h"
#include "utils.h"

const int64_t FILEREADER_BUFFER_SIZE = 1 << 15;

void filereader_readchunk(filereader *thisptr) {
  size_t rval = fread(thisptr->m_pBuffer, 1, FILEREADER_BUFFER_SIZE, thisptr->m_pStream);

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

void filereader_init(filereader *thisptr, FILE *stream) {
  thisptr->m_iBytesAvailable = 0;
  thisptr->m_iBufferOffset = 0;
  thisptr->m_uFileOffset = 0;
  thisptr->m_pBuffer = malloc(FILEREADER_BUFFER_SIZE);
  thisptr->m_pStream = stream;
  thisptr->eof = false;
}

void filereader_free(filereader *thisptr) {
  fclose(thisptr->m_pStream);
  free(thisptr->m_pBuffer);
}

void filereader_readdata(filereader *thisptr, void *buffer, int bytes) {
  char *dest = buffer;
  int bytesLeftToRead = bytes;

  do {
    char *src = (char *)thisptr->m_pBuffer + thisptr->m_iBufferOffset;
    int64_t bytesLeftInBuffer = (thisptr->m_iBytesAvailable - thisptr->m_iBufferOffset);
    int bytesRead = MIN(bytesLeftToRead, bytesLeftInBuffer);

    if (bytesRead > 0) {
      memcpy(dest, src, bytesRead);
      dest += bytesRead;
      bytesLeftToRead -= bytesRead;
      filereader_skipbytes(thisptr, bytesRead);
    }

    if (bytesLeftToRead > 0) {
      filereader_readchunk(thisptr);
    }

  } while (bytesLeftToRead > 0 && !thisptr->eof);
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
    fseek(thisptr->m_pStream, bytes, SEEK_CUR);
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

int64_t filereader_get_bytes_left(filereader* thisptr)
{
  fseek(thisptr->m_pStream, 0, SEEK_END);
  int64_t offset = ftell(thisptr->m_pStream);
  fseek(thisptr->m_pStream, thisptr->m_uFileOffset, SEEK_SET);

  return offset - filereader_current_position(thisptr);
}
