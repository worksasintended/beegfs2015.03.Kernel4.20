#include "UpdateBacklinkMsg.h"

void UpdateBacklinkMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // entryID
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], entryIDLen, entryID);

   // pathInfo
   bufPos += this->pathInfoPtr->serialize(&buf[bufPos]);

   // targetID
   bufPos += Serialization::serializeUShort(&buf[bufPos], targetID);

   // entryInfoBufLen
   bufPos += Serialization::serializeUInt(&buf[bufPos], entryInfoBufLen);

   // entryInfoBuf
   memcpy(&buf[bufPos], entryInfoBuf, entryInfoBufLen);
   bufPos += entryInfoBufLen;
}

bool UpdateBacklinkMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   { // entryID
      unsigned entryIDBufLen;

      if ( !Serialization::deserializeStrAlign4(&buf[bufPos], bufLen - bufPos, &entryIDLen,
         &entryID, &entryIDBufLen) )
         return false;

      bufPos += entryIDBufLen;
   }

   {  // PathInfo
      unsigned pathInfoBufLen;

      if (!this->pathInfo.deserialize(&buf[bufPos], bufLen-bufPos, &pathInfoBufLen) )
         return false;

      bufPos += pathInfoBufLen;
   }

   { // targetID
      unsigned targetIDBufLen;

      if ( !Serialization::deserializeUShort(&buf[bufPos], bufLen - bufPos, &targetID,
         &targetIDBufLen) )
         return false;

      bufPos += targetIDBufLen;
   }

   { // entryInfoBufLen
      unsigned entryInfoLenBufLen;

      if ( !Serialization::deserializeUInt(&buf[bufPos], bufLen - bufPos, &entryInfoBufLen,
         &entryInfoLenBufLen) )
         return false;

      bufPos += entryInfoLenBufLen;
   }

   { // entryInfoBuf
      this->entryInfoBuf = &buf[bufPos];

      bufPos += this->entryInfoBufLen;
   }

   return true;
}


