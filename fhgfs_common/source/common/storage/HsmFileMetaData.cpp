#include <common/toolkit/serialization/Serialization.h>

#include "HsmFileMetaData.h"

size_t HsmFileMetaData::serialize(char* outBuf)
{
   size_t bufPos = 0;

   // offlineChunkCount
   bufPos += Serialization::serializeUShort(&outBuf[bufPos], this->offlineChunkCount);

   // collocationID
   bufPos += Serialization::serializeUShort(&outBuf[bufPos], this->collocationID);

   return bufPos;
}


bool HsmFileMetaData::deserialize(const char* buf, size_t bufLen, unsigned* outLen)
{
   unsigned bufPos = 0;

   { // offlineChunkCount
      unsigned offlineChunkCountLen;

      if ( !Serialization::deserializeUShort(&buf[bufPos], bufLen - bufPos,
         &(this->offlineChunkCount), &offlineChunkCountLen) )
         return false;

      bufPos += offlineChunkCountLen;
   }

   { // collocationID
      unsigned collocationIDLen;

      if ( !Serialization::deserializeUShort(&buf[bufPos], bufLen - bufPos,
         &(this->collocationID), &collocationIDLen) )
         return false;

      bufPos += collocationIDLen;
   }

   *outLen = bufPos;
   return true;
}

unsigned HsmFileMetaData::serialLen(void)
{
   unsigned length =
      Serialization::serialLenUShort()  + // offlineChunkCount
      Serialization::serialLenUShort(); // collocationID

   return length;
}
