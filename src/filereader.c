#include "filereader.h"
#include "utils.h"
#include "string.h"
#include "stdlib.h"

const int64_t FILEREADER_BUFFER_SIZE = 4096;

void filereader_readchunk(filereader* thisptr)
{
    size_t rval = fread(thisptr->m_pBuffer, 1, FILEREADER_BUFFER_SIZE, thisptr->m_pStream);
    
    if(rval <= 0)
    {
        thisptr->eof = true;
        return;
    }
    
    thisptr->m_iBufferOffset = 0;
    thisptr->m_iBytesAvailable = rval;
}

int64_t filereader_bytesleftinbuffer(filereader* thisptr)
{
    if(thisptr->m_iBytesAvailable < thisptr->m_iBufferOffset)
    {
        return 0;
    }
    else
    {
        return thisptr->m_iBytesAvailable - thisptr->m_iBufferOffset;
    }
}

void filereader_init(filereader* thisptr, FILE* stream)
{
    thisptr->m_iBufferOffset = 0;
    thisptr->m_uFileOffset = 0;
    thisptr->m_pBuffer = malloc(FILEREADER_BUFFER_SIZE);
    thisptr->m_pStream = stream;
    thisptr->eof = false;
}

void filereader_free(filereader* thisptr)
{
    fclose(thisptr->m_pStream);
    free(thisptr->m_pBuffer);
}

void filereader_readdata(filereader* thisptr, void* buffer, int bytes)
{
    char* dest = buffer;
    char* src = (char*)thisptr->m_pBuffer + thisptr->m_iBufferOffset;
    int64_t bytesLeftInBuffer = (thisptr->m_iBytesAvailable - thisptr->m_iBufferOffset);
    int bytesRead = MIN(bytes, bytesLeftInBuffer);

    if(bytesRead > 0)
    {
        memcpy(dest, src, bytesRead);
    }

    if(bytesRead != bytes)
    {
        filereader_readchunk(thisptr);
        dest += bytesRead;
        src += bytesRead;
        bytesRead = bytes - bytesRead;
    }

    // In theory we should always have enough bytes for this read if the demo is well-formed
    memcpy(dest, src, bytesRead);
    filereader_skipbytes(thisptr, bytes);
}

float filereader_readfloat(filereader* thisptr)
{
    float val;
    filereader_readdata(thisptr, &val, 4);
    return val;
}

int32_t filereader_readint32(filereader* thisptr)
{
    int32_t val;
    filereader_readdata(thisptr, &val, 4);
    return val;
}

uint8_t filereader_readbyte(filereader* thisptr)
{
    uint8_t val;
    filereader_readdata(thisptr, &val, 1);
    return val;
}

void filereader_skipbytes(filereader* thisptr, int bytes)
{
    // Going backwards not supported
    if(bytes < 0)
    {
        return;
    }

    int64_t bytesLeftInBuffer = (thisptr->m_iBytesAvailable - thisptr->m_iBufferOffset);
    
    if(bytes < bytesLeftInBuffer)
    {
        thisptr->m_iBufferOffset += bytes;
    }
    else
    {
        thisptr->m_iBufferOffset = thisptr->m_iBytesAvailable;
        bytes -= bytesLeftInBuffer;
    }

    fseek(thisptr->m_pStream, bytes, SEEK_CUR);
}

void filereader_skipto(filereader* thisptr, uint64_t offset)
{
    int64_t curOffset = filereader_current_position(thisptr);
    filereader_skipbytes(thisptr, offset - curOffset);
}

int64_t filereader_current_position(filereader* thisptr)
{
    int64_t bytesLeftInBuffer = (thisptr->m_iBytesAvailable - thisptr->m_iBufferOffset);
    return thisptr->m_uFileOffset - bytesLeftInBuffer;
}
