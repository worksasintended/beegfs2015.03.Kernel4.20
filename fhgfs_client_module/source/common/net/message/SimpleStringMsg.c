#include "SimpleStringMsg.h"

void SimpleStringMsg_serializePayload(NetMessage* this, char* buf)
{
   SimpleStringMsg* thisCast = (SimpleStringMsg*)this;
   
   Serialization_serializeStr(buf, thisCast->valueLen, thisCast->value);
}

fhgfs_bool SimpleStringMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   SimpleStringMsg* thisCast = (SimpleStringMsg*)this;
   
   unsigned valLen;

   return Serialization_deserializeStr(buf, bufLen, &thisCast->valueLen, &thisCast->value, &valLen);
}

unsigned SimpleStringMsg_calcMessageLength(NetMessage* this)
{
   SimpleStringMsg* thisCast = (SimpleStringMsg*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenStr(thisCast->valueLen); // value
}



