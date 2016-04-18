#include "ListXAttrRespMsg.h"

void ListXAttrRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // value
   bufPos += Serialization::serializeStringVec(&buf[bufPos], &value);

   // size
   bufPos += Serialization::serializeInt(&buf[bufPos], size);

   // returnCode
   bufPos += Serialization::serializeInt(&buf[bufPos], returnCode);
}

bool ListXAttrRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   /* not needed, since we're only receiving this message in the client at the moment */
   return false;
}
