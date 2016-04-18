#include "SetLastBuddyCommOverrideMsgEx.h"

#include <common/net/message/storage/mirroring/SetLastBuddyCommOverrideRespMsg.h>
#include <program/Program.h>

bool SetLastBuddyCommOverrideMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   #ifdef BEEGFS_DEBUG
      const char* logContext = "SetLastBuddyCommOverrideMsg incoming";

      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, Log_DEBUG,
         std::string("Received a SetLastBuddyCommOverrideMsg from: ") + peer);
   #endif // BEEGFS_DEBUG

   FhgfsOpsErr result;
   uint16_t targetID = getTargetID();
   int64_t timestamp = getTimestamp();
   bool abortResync = getAbortResync();

   App* app = Program::getApp();
   StorageTargets* storageTargets = app->getStorageTargets();

   result = storageTargets->overrideLastBuddyComm(targetID, timestamp);

   if (abortResync)
   {
      BuddyResyncJob* resyncJob = app->getBuddyResyncer()->getResyncJob(targetID);
      if (resyncJob)
         resyncJob->abort();
   }

   SetLastBuddyCommOverrideRespMsg respMsg(result);
   respMsg.serialize(respBuf, bufLen);
   sock->send(respBuf, respMsg.getMsgLength(), 0);

   return true;
}
