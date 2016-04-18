#include <common/net/message/storage/attribs/UpdateBacklinkRespMsg.h>
#include <common/storage/StorageErrors.h>
#include <common/toolkit/MessagingTk.h>
#include <toolkit/StorageTkEx.h>
#include <program/Program.h>

#include "UpdateBacklinkMsgEx.h"

bool UpdateBacklinkMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   const char* logContext = "UpdateBacklinkMsgEx incoming";

   #ifdef BEEGFS_DEBUG
      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, Log_DEBUG, std::string("Received a UpdateBacklinkMsg from: ") + peer);
   #endif // BEEGFS_DEBUG

   App* app = Program::getApp();

   FhgfsOpsErr clientErrRes = FhgfsOpsErr_SUCCESS;

   // select the right targetID

   uint16_t targetID = getTargetID();

   if(isMsgHeaderFeatureFlagSet(UPDATEBACKLINKSMSG_FLAG_BUDDYMIRROR) )
   { // given targetID refers to a buddy mirror group
      MirrorBuddyGroupMapper* mirrorBuddies = app->getMirrorBuddyGroupMapper();

      targetID = isMsgHeaderFeatureFlagSet(UPDATEBACKLINKSMSG_FLAG_BUDDYMIRROR_SECOND) ?
         mirrorBuddies->getSecondaryTargetID(targetID) :
         mirrorBuddies->getPrimaryTargetID(targetID);

      // note: only log messe here, error handling will happen below through invalid targetFD
      if(unlikely(!targetID) )
         LogContext(logContext).logErr("Invalid mirror buddy group ID: " +
            StringTk::uintToStr(getTargetID() ) );
   }

   // get targetFD

   int targetFD = app->getTargetFD(targetID,
      isMsgHeaderFeatureFlagSet(UPDATEBACKLINKSMSG_FLAG_BUDDYMIRROR) );
   if(unlikely(targetFD == -1) )
   { // unknown targetID
      LogContext(logContext).logErr("Unknown targetID: " + StringTk::uintToStr(targetID) );
      clientErrRes = FhgfsOpsErr_UNKNOWNTARGET;
   }
   else
   { // valid targetID
      bool setRes = StorageTkEx::attachEntryInfoToChunk(targetFD, getPathInfo(), getEntryID(),
         getEntryInfoBuf(), getEntryInfoBufLen(), true);

      if(unlikely(!setRes) )
         clientErrRes = FhgfsOpsErr_INTERNAL;
   }

   UpdateBacklinkRespMsg respMsg(clientErrRes);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return true;
}
