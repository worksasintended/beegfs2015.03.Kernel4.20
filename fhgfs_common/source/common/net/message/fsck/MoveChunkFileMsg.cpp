#include "MoveChunkFileMsg.h"


bool MoveChunkFileMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   {
      // chunkName
      unsigned chunkNameBufLen;

      if ( !Serialization::deserializeStrAlign4(&buf[bufPos], bufLen - bufPos, &chunkNameLen,
         &chunkName, &chunkNameBufLen) )
         return false;

      bufPos += chunkNameBufLen;
   }

   {
      // oldPath
      unsigned oldPathBufLen;

      if ( !Serialization::deserializeStrAlign4(&buf[bufPos], bufLen - bufPos, &oldPathLen,
         &oldPath, &oldPathBufLen) )
         return false;

      bufPos += oldPathBufLen;
   }

   {
      // newPath
      unsigned newPathBufLen;

      if ( !Serialization::deserializeStrAlign4(&buf[bufPos], bufLen - bufPos, &newPathLen,
         &newPath, &newPathBufLen) )
         return false;

      bufPos += newPathBufLen;
   }

   {
      // targetID
      unsigned targetIDBufLen;

      if ( !Serialization::deserializeUInt16(&buf[bufPos], bufLen - bufPos, &targetID,
         &targetIDBufLen) )
         return false;

      bufPos += targetIDBufLen;
   }

   {
      // mirroredFromTargetID
      unsigned mirroredFromTargetIDBufLen;

      if ( !Serialization::deserializeUInt16(&buf[bufPos], bufLen - bufPos, &mirroredFromTargetID,
         &mirroredFromTargetIDBufLen) )
         return false;

      bufPos += mirroredFromTargetIDBufLen;
   }

   {
      // overwriteExisting
      unsigned overwriteExistingBufLen;

      if ( !Serialization::deserializeBool(&buf[bufPos], bufLen - bufPos, &overwriteExisting,
         &overwriteExistingBufLen) )
         return false;

      bufPos += overwriteExistingBufLen;
   }

   return true;
}

void MoveChunkFileMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // chunkName
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], chunkNameLen, chunkName);

   // oldPath
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], oldPathLen, oldPath);

   // newPath
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], newPathLen, newPath);

   // targetID
   bufPos += Serialization::serializeUInt16(&buf[bufPos], targetID);

   // mirroredFromTargetID
   bufPos += Serialization::serializeUInt16(&buf[bufPos], mirroredFromTargetID);

   // overwriteExisting
   bufPos += Serialization::serializeBool(&buf[bufPos], overwriteExisting);
}

