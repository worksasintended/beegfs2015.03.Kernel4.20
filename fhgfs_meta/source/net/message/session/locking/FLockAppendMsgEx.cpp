#include <common/net/message/session/locking/FLockAppendRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/SessionTk.h>
#include <net/msghelpers/MsgHelperLocking.h>
#include <program/Program.h>
#include <session/SessionStore.h>
#include <storage/MetaStore.h>
#include "FLockAppendMsgEx.h"


bool FLockAppendMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   /* note: this code is very similar to FLockRangeMsgEx::processIncoming(), so if you change
      something here, you probably want to change it there, too. */

   #ifdef BEEGFS_DEBUG
      const char* logContext = "FLockAppendMsg incoming";

      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, Log_DEBUG, std::string("Received a FLockAppendMsg from: ") + peer);
   #endif // BEEGFS_DEBUG


   unsigned ownerFD = SessionTk::ownerFDFromHandleID(getFileHandleID() );

   EntryLockDetails lockDetails(getClientID(), getClientFD(), getOwnerPID(), getLockAckID(),
      getLockTypeFlags() );

   LOG_DEBUG(logContext, Log_SPAM, lockDetails.toString() );

   EntryInfo* entryInfo = getEntryInfo();

   FhgfsOpsErr clientResult = MsgHelperLocking::flockAppend(entryInfo, ownerFD, lockDetails);

   LOG_DEBUG(logContext, Log_SPAM, std::string("Client result: ") +
      FhgfsOpsErrTk::toErrString(clientResult) );

   // send response
   FLockAppendRespMsg respMsg(clientResult);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   Program::getApp()->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_FLOCKAPPEND,
      getMsgHeaderUserID() );

   return true;
}

