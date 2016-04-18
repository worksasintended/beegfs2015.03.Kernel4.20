#include "SimpleUInt16Msg.h"

void SimpleUInt16Msg_serializePayload(NetMessage* this, char* buf)
{
   SimpleUInt16Msg* thisCast = (SimpleUInt16Msg*)this;

   Serialization_serializeUShort(buf, thisCast->value);
}

fhgfs_bool SimpleUInt16Msg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   SimpleUInt16Msg* thisCast = (SimpleUInt16Msg*)this;

   unsigned valLen;

   return Serialization_deserializeUShort(buf, bufLen, &thisCast->value, &valLen);
}

unsigned SimpleUInt16Msg_calcMessageLength(NetMessage* this)
{
   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenUShort(); // value
}


