#include "RemoveBuddyGroupMsg.h"

#include <common/toolkit/serialization/Serialization.h>

void RemoveBuddyGroupMsg::serializePayload(char* buf)
{
   buf += Serialization::serializeInt(buf, int(type));
   buf += Serialization::serializeUInt16(buf, groupID);
   buf += Serialization::serializeBool(buf, checkOnly);
}

bool RemoveBuddyGroupMsg::deserializePayload(const char* buf, size_t bufLen)
{
   unsigned fieldLen;

   int type;
   if (!Serialization::deserializeInt(buf, bufLen, &type, &fieldLen))
      return false;

   this->type = NodeType(type);
   buf += fieldLen;
   bufLen -= fieldLen;

   if (!Serialization::deserializeUInt16(buf, bufLen, &groupID, &fieldLen))
      return false;

   buf += fieldLen;
   bufLen -= fieldLen;

   if (!Serialization::deserializeBool(buf, bufLen, &checkOnly, &fieldLen))
      return false;

   buf += fieldLen;
   bufLen -= fieldLen;

   return true;
}

unsigned RemoveBuddyGroupMsg::calcMessageLength()
{
   return NETMSG_HEADER_LENGTH
      + Serialization::serialLenInt() // type
      + Serialization::serialLenUInt16() // groupID
      + Serialization::serialLenBool(); // checkOnly
}
