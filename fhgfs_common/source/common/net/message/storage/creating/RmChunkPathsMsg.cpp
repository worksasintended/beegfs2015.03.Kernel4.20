#include "RmChunkPathsMsg.h"

bool RmChunkPathsMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   {
      // targetID
      unsigned targetIDBufLen;
      if ( !Serialization::deserializeUInt16(&buf[bufPos], bufLen - bufPos, &targetID,
         &targetIDBufLen) )
         return false;

      bufPos += targetIDBufLen;
   }

   {
      // relativePaths
      if ( !Serialization::deserializeStringListPreprocess(&buf[bufPos], bufLen - bufPos,
         &relativePathsElemNum, &relativePathsStart, &relativePathsBufLen) )
         return false;

      bufPos += relativePathsBufLen;
   }

   return true;
}

void RmChunkPathsMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // targetID
   bufPos += Serialization::serializeUInt16(&buf[bufPos], targetID);

   // relativePaths
   bufPos += Serialization::serializeStringList(&buf[bufPos], relativePaths);
}
