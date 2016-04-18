#include <app/App.h>
#include <common/toolkit/SocketTk.h>
#include "GetHostByNameMsg.h"

void GetHostByNameMsg_serializePayload(NetMessage* this, char* buf)
{
   GetHostByNameMsg* thisCast = (GetHostByNameMsg*)this;

   size_t bufPos = 0;

   // hostname
   bufPos += Serialization_serializeStr(&buf[bufPos], thisCast->hostnameLen, thisCast->hostname);
}

fhgfs_bool GetHostByNameMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   GetHostByNameMsg* thisCast = (GetHostByNameMsg*)this;

   size_t bufPos = 0;

   unsigned hostnameBufLen;

   // hostname

   if(!Serialization_deserializeStr(&buf[bufPos], bufLen-bufPos,
      &thisCast->hostnameLen, &thisCast->hostname, &hostnameBufLen) )
      return fhgfs_false;

   bufPos += hostnameBufLen;

   return fhgfs_true;
}

unsigned GetHostByNameMsg_calcMessageLength(NetMessage* this)
{
   GetHostByNameMsg* thisCast = (GetHostByNameMsg*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenStr(thisCast->hostnameLen);
}



