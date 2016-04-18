#include "SimpleIntMsg.h"

void SimpleIntMsg_serializePayload(NetMessage* this, char* buf)
{
   SimpleIntMsg* thisCast = (SimpleIntMsg*)this;
   
   Serialization_serializeInt(buf, thisCast->value);
}

fhgfs_bool SimpleIntMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   SimpleIntMsg* thisCast = (SimpleIntMsg*)this;
   
   unsigned valLen;

   return Serialization_deserializeInt(buf, bufLen, &thisCast->value, &valLen);
}

unsigned SimpleIntMsg_calcMessageLength(NetMessage* this)
{
   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenInt(); // value
}


