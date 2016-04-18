#include "RmChunkPathsRespMsg.h"

bool RmChunkPathsRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   {
      // failedPaths
      if ( !Serialization::deserializeStringListPreprocess(&buf[bufPos], bufLen - bufPos,
         &failedPathsElemNum, &failedPathsStart, &failedPathsBufLen) )
         return false;

      bufPos += failedPathsBufLen;
   }

   return true;
}

void RmChunkPathsRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // failedPaths
   bufPos += Serialization::serializeStringList(&buf[bufPos], failedPaths);
}
