#include "GetStorageResyncStatsMsgEx.h"

#include <common/net/message/storage/mirroring/GetStorageResyncStatsRespMsg.h>
#include <program/Program.h>

bool GetStorageResyncStatsMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   #ifdef BEEGFS_DEBUG
      const char* logContext = "GetStorageResyncStatsMsg incoming";

      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, Log_DEBUG,
         std::string("Received a GetStorageResyncStatsMsg from: ") + peer);
   #endif // BEEGFS_DEBUG

   App* app = Program::getApp();
   BuddyResyncer* buddyResyncer = app->getBuddyResyncer();
   uint16_t targetID = getTargetID();
   BuddyResyncJobStats jobStats;

   BuddyResyncJob* resyncJob = buddyResyncer->getResyncJob(targetID);

   if (resyncJob)
      resyncJob->getJobStats(jobStats);

   GetStorageResyncStatsRespMsg respMsg(&jobStats);
   respMsg.serialize(respBuf, bufLen);
   sock->send(respBuf, respMsg.getMsgLength(), 0);

   return true;
}
