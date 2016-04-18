#include <common/net/msghelpers/MsgHelperAck.h>
#include <common/toolkit/MessagingTk.h>
#include <program/Program.h>

#include "RefreshTargetStatesMsgEx.h"


bool RefreshTargetStatesMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("RefreshTargetStatesMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG_CONTEXT(log, Log_DEBUG, "Received a RefreshTargetStatesMsg from: " + peer);

   App* app = Program::getApp();
   InternodeSyncer* syncer = app->getInternodeSyncer();
   HeartbeatManager* heartbeatMgr = app->getHeartbeatMgr();

   // force update of target states
   syncer->setForceTargetStatesUpdate();
   heartbeatMgr->setForceTargetStatesUpdate();

   // send response

   MsgHelperAck::respondToAckRequest(this, fromAddr, sock, respBuf, bufLen,
      app->getDatagramListener() );

   return true;
}

