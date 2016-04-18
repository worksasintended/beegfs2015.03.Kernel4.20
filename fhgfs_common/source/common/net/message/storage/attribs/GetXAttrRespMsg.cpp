#include "GetXAttrRespMsg.h"

void GetXAttrRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // value
   bufPos += Serialization::serializeCharVector(&buf[bufPos], &this->value);

   // size
   bufPos += Serialization::serializeInt(&buf[bufPos], this->size);

   // returnCode
   bufPos += Serialization::serializeInt(&buf[bufPos], this->returnCode);
}

bool GetXAttrRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   // not needed, since we're only receiving this message in the client
   return false;
}

