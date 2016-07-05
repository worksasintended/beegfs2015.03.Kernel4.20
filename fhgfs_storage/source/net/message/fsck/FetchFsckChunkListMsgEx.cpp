#include "FetchFsckChunkListMsgEx.h"

#include <program/Program.h>

bool FetchFsckChunkListMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
      char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   const char* logContext = "FetchFsckChunkListMsg incoming";
#ifdef BEEGFS_DEBUG
   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG(logContext, 4, std::string("Received a FetchFsckChunkListMsg from: ") + peer);
#endif // BEEGFS_DEBUG

   App* app = Program::getApp();
   ChunkFetcher* chunkFetcher = app->getChunkFetcher();


   FsckChunkList chunkList;
   FetchFsckChunkListStatus status;

   if (getLastStatus() == FetchFsckChunkListStatus_NOTSTARTED)
   {
      // This is the first message of a new Fsck run
      if (chunkFetcher->getNumRunning() != 0 || !chunkFetcher->isQueueEmpty())
      {
         // Another fsck is already in progress
         if (!getForceRestart())
         {
            LogContext(logContext).log(Log_NOTICE,
                  "Received request to start fsck although previous run is not finished. "
                  "Not starting.");

            FetchFsckChunkListRespMsg respMsg(&chunkList, FetchFsckChunkListStatus_NOTSTARTED);
            respMsg.serialize(respBuf, bufLen);
            sock->sendto(respBuf, respMsg.getMsgLength(), 0, (struct sockaddr*) fromAddr,
                  sizeof(struct sockaddr_in));

            return true;
         }
         else
         {
            LogContext(logContext).log(Log_NOTICE,
                  "Aborting previous fsck cunk fetcher run by user request.");

            chunkFetcher->stopFetching();
            chunkFetcher->waitForStopFetching();
         }
      }

      chunkFetcher->startFetching();
   }

   if(chunkFetcher->getNumRunning() == 0)
      status = FetchFsckChunkListStatus_FINISHED;
   else
      status = FetchFsckChunkListStatus_RUNNING;

   chunkFetcher->getAndDeleteChunks(chunkList, getMaxNumChunks());

   FetchFsckChunkListRespMsg respMsg(&chunkList, status);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0, (struct sockaddr*) fromAddr,
      sizeof(struct sockaddr_in));

   return true;
}
