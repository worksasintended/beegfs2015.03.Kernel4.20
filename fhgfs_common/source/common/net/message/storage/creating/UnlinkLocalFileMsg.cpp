#include "UnlinkLocalFileMsg.h"

bool UnlinkLocalFileMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   {  // entryID
      unsigned entryBufLen;

      if(!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &entryIDLen, &entryID, &entryBufLen) )
         return false;

      bufPos += entryBufLen;
   }

   {  // PathInfo
      unsigned pathInfoBufLen;

      if (!this->pathInfo.deserialize(&buf[bufPos], bufLen-bufPos, &pathInfoBufLen) )
         return false;

      this->pathInfoPtr = &this->pathInfo; // (to allow re-serialization of this msg)

      bufPos += pathInfoBufLen;
   }

   {  // targetID
      unsigned targetBufLen;

      if(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos,
         &targetID, &targetBufLen) )
         return false;

      bufPos += targetBufLen;
   }


   return true;
}

void UnlinkLocalFileMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // entryID
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], entryIDLen, entryID);

   // pathInfo
   bufPos += this->pathInfoPtr->serialize(&buf[bufPos]);

   // targetID
   bufPos += Serialization::serializeUShort(&buf[bufPos], targetID);
}


