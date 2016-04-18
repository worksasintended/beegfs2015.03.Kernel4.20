#include "FetchFsckChunkListRespMsg.h"

bool FetchFsckChunkListRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   {
      unsigned chunkListBufLen;

      // chunkList
      if ( !SerializationFsck::deserializeFsckChunkListPreprocess(&buf[bufPos], bufLen - bufPos,
         &chunkListStart, &chunkListElemNum, &chunkListBufLen) )
         return false;

      bufPos += chunkListBufLen;
   }

   {
      // status
      unsigned statusBufLen;
      unsigned status;

      if ( !Serialization::deserializeUInt(&buf[bufPos], bufLen - bufPos, &status, &statusBufLen) )
         return false;

      this->status = (FetchFsckChunkListStatus)status;

      bufPos += statusBufLen;
   }

   return true;
}

void FetchFsckChunkListRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // chunksList
   bufPos += SerializationFsck::serializeFsckChunkList(&buf[bufPos], this->chunkList);

   // status
   bufPos += Serialization::serializeUInt(&buf[bufPos], this->status);
}
