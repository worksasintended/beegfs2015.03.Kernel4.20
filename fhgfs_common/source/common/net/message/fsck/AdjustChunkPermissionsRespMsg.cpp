#include "AdjustChunkPermissionsRespMsg.h"

void AdjustChunkPermissionsRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // currentContDirID
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], currentContDirIDLen,
      currentContDirID);

   // count
   bufPos += Serialization::serializeUInt(&buf[bufPos], count);

   // newHashDirOffset
   bufPos += Serialization::serializeInt64(&buf[bufPos], newHashDirOffset);

   // newContDirOffset
   bufPos += Serialization::serializeInt64(&buf[bufPos], newContDirOffset);

   // errorCount
   bufPos += Serialization::serializeUInt(&buf[bufPos], errorCount);
}

bool AdjustChunkPermissionsRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   // currentContDirName
   {
      unsigned currentContDirNameBufLen;
      if ( !Serialization::deserializeStrAlign4(&buf[bufPos], bufLen - bufPos,
         &currentContDirIDLen, &currentContDirID, &currentContDirNameBufLen) )
         return false;

      bufPos += currentContDirNameBufLen;
   }

   {
      // count
      unsigned countBufLen;

      if ( !Serialization::deserializeUInt(&buf[bufPos], bufLen - bufPos, &count,
         &countBufLen) )
         return false;

      bufPos += countBufLen;
   }

   {
      // newHashDirOffset
      unsigned newHashDirOffsetBufLen;

      if ( !Serialization::deserializeInt64(&buf[bufPos], bufLen - bufPos, &newHashDirOffset,
         &newHashDirOffsetBufLen) )
         return false;

      bufPos += newHashDirOffsetBufLen;
   }

   {
      // newContDirOffset
      unsigned newContDirOffsetBufLen;

      if ( !Serialization::deserializeInt64(&buf[bufPos], bufLen - bufPos, &newContDirOffset,
         &newContDirOffsetBufLen) )
         return false;

      bufPos += newContDirOffsetBufLen;
   }
   
   {
      // errorCount
      unsigned errorCountBufLen;

      if ( !Serialization::deserializeUInt(&buf[bufPos], bufLen - bufPos, &errorCount,
         &errorCountBufLen) )
         return false;

      bufPos += errorCountBufLen;
   }

   return true;
}

