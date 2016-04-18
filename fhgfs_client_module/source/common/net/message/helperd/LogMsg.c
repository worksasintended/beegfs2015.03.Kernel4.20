#include <app/App.h>
#include <common/nodes/Node.h>
#include <common/toolkit/SocketTk.h>
#include <common/toolkit/ListTk.h>
#include <nodes/NodeStoreEx.h>
#include "LogMsg.h"


void LogMsg_serializePayload(NetMessage* this, char* buf)
{
   LogMsg* thisCast = (LogMsg*)this;

   size_t bufPos = 0;

   // level
   bufPos += Serialization_serializeInt(&buf[bufPos], thisCast->level);

   // threadID
   bufPos += Serialization_serializeInt(&buf[bufPos], thisCast->threadID);

   // threadName
   bufPos += Serialization_serializeStr(
      &buf[bufPos], thisCast->threadNameLen, thisCast->threadName);

   // context
   bufPos += Serialization_serializeStr(&buf[bufPos], thisCast->contextLen, thisCast->context);

   // logMsg
   bufPos += Serialization_serializeStr(&buf[bufPos], thisCast->logMsgLen, thisCast->logMsg);
}

fhgfs_bool LogMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   LogMsg* thisCast = (LogMsg*)this;

   size_t bufPos = 0;

   unsigned levelBufLen;
   unsigned threadIDBufLen;
   unsigned threadNameBufLen;
   unsigned contextBufLen;
   unsigned logMsgBufLen;

   // level

   if(!Serialization_deserializeInt(&buf[bufPos], bufLen-bufPos,
      &thisCast->level, &levelBufLen) )
      return fhgfs_false;

   bufPos += levelBufLen;

   // threadID

   if(!Serialization_deserializeInt(&buf[bufPos], bufLen-bufPos,
      &thisCast->threadID, &threadIDBufLen) )
      return fhgfs_false;

   bufPos += threadIDBufLen;

   // threadName

   if(!Serialization_deserializeStr(&buf[bufPos], bufLen-bufPos,
      &thisCast->threadNameLen, &thisCast->threadName, &threadNameBufLen) )
      return fhgfs_false;

   bufPos += threadNameBufLen;

   // context

   if(!Serialization_deserializeStr(&buf[bufPos], bufLen-bufPos,
      &thisCast->contextLen, &thisCast->context, &contextBufLen) )
      return fhgfs_false;

   bufPos += contextBufLen;

   // logMsg

   if(!Serialization_deserializeStr(&buf[bufPos], bufLen-bufPos,
      &thisCast->logMsgLen, &thisCast->logMsg, &logMsgBufLen) )
      return fhgfs_false;

   bufPos += logMsgBufLen;


   return fhgfs_true;
}

unsigned LogMsg_calcMessageLength(NetMessage* this)
{
   LogMsg* thisCast = (LogMsg*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenInt() + // level
      Serialization_serialLenInt() + // threadID
      Serialization_serialLenStr(thisCast->threadNameLen) +
      Serialization_serialLenStr(thisCast->contextLen) +
      Serialization_serialLenStr(thisCast->logMsgLen);
}


