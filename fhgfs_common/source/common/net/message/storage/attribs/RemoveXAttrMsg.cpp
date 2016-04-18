#include "RemoveXAttrMsg.h"

bool RemoveXAttrMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   // entryInfo
   unsigned entryInfoBufLen;

   if(!this->entryInfo.deserialize(&buf[bufPos], bufLen-bufPos, &entryInfoBufLen) )
      return false;

   bufPos += entryInfoBufLen;

   // name
   unsigned nameBufLen;

   if(!Serialization::deserializeStr(&buf[bufPos], bufLen-bufPos, &this->name, &nameBufLen) )
      return false;

   bufPos += nameBufLen;

   return true;
}

void RemoveXAttrMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // entryInfo
   bufPos += this->entryInfoPtr->serialize(&buf[bufPos]);

   // name
   bufPos += Serialization::serializeStr(&buf[bufPos], this->name.length(), this->name.c_str() );
}
