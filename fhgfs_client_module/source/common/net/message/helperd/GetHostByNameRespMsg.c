#include <app/App.h>
#include <common/toolkit/SocketTk.h>
#include "GetHostByNameRespMsg.h"

void GetHostByNameRespMsg_serializePayload(NetMessage* this, char* buf)
{
   GetHostByNameRespMsg* thisCast = (GetHostByNameRespMsg*)this;

   size_t bufPos = 0;

   // hostAddr
   bufPos += Serialization_serializeStr(&buf[bufPos], thisCast->hostAddrLen, thisCast->hostAddr);
}

fhgfs_bool GetHostByNameRespMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   GetHostByNameRespMsg* thisCast = (GetHostByNameRespMsg*)this;

   size_t bufPos = 0;

   unsigned hostAddrBufLen;

   // hostAddr

   if(!Serialization_deserializeStr(&buf[bufPos], bufLen-bufPos,
      &thisCast->hostAddrLen, &thisCast->hostAddr, &hostAddrBufLen) )
      return fhgfs_false;

   bufPos += hostAddrBufLen;

   return fhgfs_true;
}

unsigned GetHostByNameRespMsg_calcMessageLength(NetMessage* this)
{
   GetHostByNameRespMsg* thisCast = (GetHostByNameRespMsg*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenStr(thisCast->hostAddrLen);
}



