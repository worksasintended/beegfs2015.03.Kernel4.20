#include "FetchFsckChunkListMsgEx.h"

#include <program/Program.h>

bool FetchFsckChunkListMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock, char* respBuf,
   size_t bufLen, HighResolutionStats* stats)
{
#ifdef BEEGFS_DEBUG
   const char* logContext = "FetchFsckChunkListMsg incoming";
   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG(logContext, 4, std::string("Received a FetchFsckChunkListMsg from: ") + peer);
#endif // BEEGFS_DEBUG

   App* app = Program::getApp();
   ChunkFetcher* chunkFetcher = app->getChunkFetcher();

   if (this->getLastStatus() == FetchFsckChunkListStatus_NOTSTARTED)
      chunkFetcher->startFetching();

   FsckChunkList chunkList;
   FetchFsckChunkListStatus status;

   if (chunkFetcher->getNumRunning() == 0)
      status = FetchFsckChunkListStatus_FINISHED;
   else
      status = FetchFsckChunkListStatus_RUNNING;

   chunkFetcher->getAndDeleteChunks(chunkList, this->getMaxNumChunks());

   FetchFsckChunkListRespMsg respMsg(&chunkList, status);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0, (struct sockaddr*) fromAddr,
      sizeof(struct sockaddr_in));

   return true;
}
