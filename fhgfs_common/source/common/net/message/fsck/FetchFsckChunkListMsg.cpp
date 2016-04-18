#include "FetchFsckChunkListMsg.h"

bool FetchFsckChunkListMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   {
      // maxNumChunks
      unsigned maxNumChunksBufLen;

      if ( !Serialization::deserializeUInt(&buf[bufPos], bufLen - bufPos, &(this->maxNumChunks),
         &maxNumChunksBufLen) )
         return false;

      bufPos += maxNumChunksBufLen;
   }

   {
      // lastStatus
      unsigned lastStatusBufLen;

      unsigned lastStatus;

      if ( !Serialization::deserializeUInt(&buf[bufPos], bufLen - bufPos, &lastStatus,
         &lastStatusBufLen) )
         return false;

      this->lastStatus = (FetchFsckChunkListStatus)lastStatus;

      bufPos += lastStatusBufLen;
   }

   return true;
}

void FetchFsckChunkListMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // maxNumChunks
   bufPos += Serialization::serializeUInt(&buf[bufPos], this->maxNumChunks);

   // lastStatus
   bufPos += Serialization::serializeUInt(&buf[bufPos], this->lastStatus);
}
