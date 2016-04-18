#include "SimpleInt64Msg.h"

void SimpleInt64Msg_serializePayload(NetMessage* this, char* buf)
{
   SimpleInt64Msg* thisCast = (SimpleInt64Msg*)this;
   
   Serialization_serializeInt64(buf, thisCast->value);
}

fhgfs_bool SimpleInt64Msg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   SimpleInt64Msg* thisCast = (SimpleInt64Msg*)this;
   
   unsigned valLen;

   return Serialization_deserializeInt64(buf, bufLen, &thisCast->value, &valLen);
}

unsigned SimpleInt64Msg_calcMessageLength(NetMessage* this)
{
   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenInt64(); // value
}



