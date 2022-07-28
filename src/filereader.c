#include "filereader.h"
#include "streams.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>

void filereader_readchunk(filereader *thisptr) {
  size_t rval = thisptr->input_funcs.read(thisptr->stream, thisptr->buffer, thisptr->buffer_size);

  if (rval <= 0) {
    thisptr->eof = true;
    return;
  }

  thisptr->ibuffer_offset = 0;
  thisptr->ibytes_available = rval;
  thisptr->ufile_offset += rval;
}

int64_t filereader_bytesleftinbuffer(filereader *thisptr) {
  if (thisptr->ibytes_available < thisptr->ibuffer_offset) {
    return 0;
  } else {
    return thisptr->ibytes_available - thisptr->ibuffer_offset;
  }
}

void filereader_init(filereader *thisptr, void* buffer, size_t buffer_size, void* stream, input_interface funcs) {
  memset(thisptr, 0, sizeof(filereader));
  thisptr->buffer = buffer;
  thisptr->buffer_size = buffer_size;
  thisptr->stream = stream;
  thisptr->input_funcs = funcs;
}

size_t filereader_readdata(filereader *thisptr, void *buffer, int bytes) {
  char *dest = buffer;
  int bytesLeftToRead = bytes;
  size_t totalBytesRead = 0;

  do {
    char *src = (char *)thisptr->buffer + thisptr->ibuffer_offset;
    int64_t bytesLeftInBuffer = (thisptr->ibytes_available - thisptr->ibuffer_offset);
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

  int64_t bytesLeftInBuffer = (thisptr->ibytes_available - thisptr->ibuffer_offset);

  if (bytes < bytesLeftInBuffer) {
    thisptr->ibuffer_offset += bytes;
  } else {
    thisptr->ibuffer_offset = thisptr->ibytes_available;
    bytes -= bytesLeftInBuffer;
    thisptr->input_funcs.seek(thisptr->stream, bytes);
    thisptr->ufile_offset += bytes;
  }
}

void filereader_skipto(filereader *thisptr, uint64_t offset) {
  int64_t curOffset = filereader_current_position(thisptr);
  filereader_skipbytes(thisptr, offset - curOffset);
}

int64_t filereader_current_position(filereader *thisptr) {
  int64_t bytesLeftInBuffer = (thisptr->ibytes_available - thisptr->ibuffer_offset);
  return thisptr->ufile_offset - bytesLeftInBuffer;
}
