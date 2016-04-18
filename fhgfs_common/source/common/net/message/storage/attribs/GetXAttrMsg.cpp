#include "GetXAttrMsg.h"

bool GetXAttrMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   // entryInfo
   unsigned entryBufLen;

   if(!this->entryInfo.deserialize(&buf[bufPos], bufLen-bufPos, &entryBufLen) )
      return false;

   bufPos += entryBufLen;

   // name
   unsigned nameBufLen;

   if(!Serialization::deserializeStr(&buf[bufPos], bufLen-bufPos, &this->name, &nameBufLen) )
      return false;

   bufPos += nameBufLen;

   // size
   unsigned sizeBufLen;

   if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos, &this->size, &sizeBufLen) )
      return false;

   bufPos += sizeBufLen;

   return true;
}

void GetXAttrMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // entryInfo
   bufPos += this->entryInfoPtr->serialize(&buf[bufPos]);

   // name
   bufPos += Serialization::serializeStr(&buf[bufPos], this->name.length(), this->name.c_str() );

   // size
   bufPos += Serialization::serializeInt(&buf[bufPos], this->size);
}
