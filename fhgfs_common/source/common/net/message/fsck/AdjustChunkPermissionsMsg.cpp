#include "AdjustChunkPermissionsMsg.h"


bool AdjustChunkPermissionsMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   {
      // currentContDirName
      unsigned currentContDirNameBufLen;

      if ( !Serialization::deserializeStrAlign4(&buf[bufPos], bufLen - bufPos, &currentContDirIDLen,
         &currentContDirID, &currentContDirNameBufLen) )
         return false;

      bufPos += currentContDirNameBufLen;
   }

   {
      // hashDirNum
      unsigned hashDirNumBufLen;

      if ( !Serialization::deserializeUInt(&buf[bufPos], bufLen - bufPos, &hashDirNum,
         &hashDirNumBufLen) )
         return false;

      bufPos += hashDirNumBufLen;
   }

   {
      // maxEntries
      unsigned maxEntriesBufLen;

      if ( !Serialization::deserializeUInt(&buf[bufPos], bufLen - bufPos, &maxEntries,
         &maxEntriesBufLen) )
         return false;

      bufPos += maxEntriesBufLen;
   }

   {
      // lastHashDirOffset
      unsigned lastHashDirOffsetBufLen;

      if ( !Serialization::deserializeInt64(&buf[bufPos], bufLen - bufPos, &lastHashDirOffset,
         &lastHashDirOffsetBufLen) )
         return false;

      bufPos += lastHashDirOffsetBufLen;
   }

   {
      // lastContDirOffset
      unsigned lastContDirOffsetBufLen;

      if ( !Serialization::deserializeInt64(&buf[bufPos], bufLen - bufPos, &lastContDirOffset,
         &lastContDirOffsetBufLen) )
         return false;

      bufPos += lastContDirOffsetBufLen;
   }

   return true;
}

void AdjustChunkPermissionsMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // currentContDirName
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], currentContDirIDLen, currentContDirID);

   // hashDirNum
   bufPos += Serialization::serializeUInt(&buf[bufPos], hashDirNum);

   // maxEntries
   bufPos += Serialization::serializeUInt(&buf[bufPos], maxEntries);

   // lastHashDirOffset
   bufPos += Serialization::serializeInt64(&buf[bufPos], lastHashDirOffset);

   // lastContDirOffset
   bufPos += Serialization::serializeInt64(&buf[bufPos], lastContDirOffset);
}

