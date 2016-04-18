#include "ListXAttrMsg.h"

bool ListXAttrMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   // entryInfo
   unsigned entryBufLen;

   if(!this->entryInfo.deserialize(&buf[bufPos], bufLen-bufPos, &entryBufLen) )
      return false;

   bufPos += entryBufLen;

   // size
   unsigned sizeBufLen;

   if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos, &size, &sizeBufLen) )
      return false;

   bufPos += sizeBufLen;

   return true;
}

void ListXAttrMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // entryInfo
   bufPos += this->entryInfoPtr->serialize(&buf[bufPos]);

   // size
   bufPos += Serialization::serializeInt(&buf[bufPos], size);
}
