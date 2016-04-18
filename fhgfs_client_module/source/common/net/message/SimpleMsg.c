#include "SimpleMsg.h"

void SimpleMsg_serializePayload(NetMessage* this, char* buf)
{
   // nothing to be done here for simple messages
}

fhgfs_bool SimpleMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   // nothing to be done here for simple messages
   return fhgfs_true;
}

unsigned SimpleMsg_calcMessageLength(NetMessage* this)
{
   return NETMSG_HEADER_LENGTH;
}


