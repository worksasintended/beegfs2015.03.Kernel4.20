#include <app/App.h>
#include <common/toolkit/SocketTk.h>
#include <common/net/msghelpers/MsgHelperAck.h>
#include <components/InternodeSyncer.h>

#include "RefreshTargetStatesMsgEx.h"


fhgfs_bool RefreshTargetStatesMsgEx_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   RefreshTargetStatesMsgEx* thisCast = (RefreshTargetStatesMsgEx*)this;

   size_t bufPos = 0;

   // ackID
   unsigned ackBufLen;

   if(!Serialization_deserializeStr(&buf[bufPos], bufLen-bufPos,
      &thisCast->ackIDLen, &thisCast->ackID, &ackBufLen) )
      return fhgfs_false;

   bufPos += ackBufLen;


   return fhgfs_true;
}

unsigned RefreshTargetStatesMsgEx_calcMessageLength(NetMessage* this)
{
   RefreshTargetStatesMsgEx* thisCast = (RefreshTargetStatesMsgEx*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenStr(thisCast->ackIDLen);
}

fhgfs_bool __RefreshTargetStatesMsgEx_processIncoming(NetMessage* this, struct App* app,
   fhgfs_sockaddr_in* fromAddr, struct Socket* sock, char* respBuf, size_t bufLen)
{
   Logger* log = App_getLogger(app);
   const char* logContext = "RefreshTargetStatesMsg incoming";

   RefreshTargetStatesMsgEx* thisCast = (RefreshTargetStatesMsgEx*)this;

   const char* peer;

   InternodeSyncer* internodeSyncer = App_getInternodeSyncer(app);
   InternodeSyncer_setForceTargetStatesUpdate(internodeSyncer);

   peer = fromAddr ?
      SocketTk_ipaddrToStr(&fromAddr->addr) : StringTk_strDup(Socket_getPeername(sock) );
   LOG_DEBUG_FORMATTED(log, 4, logContext, "Received a RefreshTargetStatesMsg from: %s", peer);
   os_kfree(peer);

   IGNORE_UNUSED_VARIABLE(log);
   IGNORE_UNUSED_VARIABLE(logContext);

   // send ack
   MsgHelperAck_respondToAckRequest(app, RefreshTargetStatesMsgEx_getAckID(thisCast), fromAddr,
      sock, respBuf, bufLen);

   return fhgfs_true;
}
