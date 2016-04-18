#include "SetXAttrMsg.h"

bool SetXAttrMsg::deserializePayload(const char* buf, size_t bufLen)
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

   // value
   unsigned valueBufLen;

   unsigned valueElemNum;
   const char* valueVecStart;
   if(!Serialization::deserializeCharVectorPreprocess(&buf[bufPos], bufLen-bufPos,
         &valueElemNum, &valueVecStart, &valueBufLen) )
      return false;

   if(!Serialization::deserializeCharVector(valueBufLen, valueElemNum, valueVecStart,
         &this->value) )
      return false;

   bufPos += valueBufLen;

   // flags
   unsigned flagsBufLen;

   if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos, &this->flags, &flagsBufLen) )
      return false;

   bufPos += flagsBufLen;

   return true;
}

void SetXAttrMsg::serializePayload(char* buf)
{
   // not needed here because this message is only received by server.
}

