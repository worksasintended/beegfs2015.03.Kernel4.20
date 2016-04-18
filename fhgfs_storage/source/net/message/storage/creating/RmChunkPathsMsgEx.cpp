#include <common/net/message/storage/creating/RmChunkPathsRespMsg.h>
#include <toolkit/StorageTkEx.h>
#include <program/Program.h>

#include "RmChunkPathsMsgEx.h"

bool RmChunkPathsMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock, char* respBuf,
   size_t bufLen, HighResolutionStats* stats)
{
   const char* logContext = "RmChunkPathsMsg incoming";

   #ifdef BEEGFS_DEBUG
      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, Log_DEBUG, std::string("Received a RmChunkPathsMsg from: ") + peer);
   #endif // BEEGFS_DEBUG

   App* app = Program::getApp();
   ChunkStore* chunkStore = app->getChunkDirStore();

   uint16_t targetID;
   StringList relativePaths;
   StringList failedPaths;

   targetID = getTargetID();
   parseRelativePaths(&relativePaths);

   // get targetFD
   int targetFD = app->getTargetFD(targetID,
      isMsgHeaderFeatureFlagSet(RMCHUNKPATHSMSG_FLAG_BUDDYMIRROR));
   if(unlikely(targetFD == -1))
   { // unknown targetID
      LogContext(logContext).logErr("Unknown targetID: " + StringTk::uintToStr(targetID));
      failedPaths = relativePaths;
   }
   else
   { // valid targetID
      for(StringListIter iter = relativePaths.begin(); iter != relativePaths.end(); iter++)
      {
         // remove chunk
         int unlinkRes = unlinkat(targetFD, (*iter).c_str(), 0);

         if ( (unlinkRes != 0)  && (errno != ENOENT) )
         {
            LogContext(logContext).logErr(
               "Unable to remove entry: " + *iter + "; error: " + System::getErrString());
            failedPaths.push_back(*iter);

            continue;
         }

         // removal succeeded; this might have been the last entry => try to remove parent directory
         PathVec parentDirPathVec(StorageTk::getPathDirname(*iter));

         chunkStore->rmdirChunkDirPath(targetFD, &parentDirPathVec);
      }
   }

   // send response...
   RmChunkPathsRespMsg respMsg(&failedPaths);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0, (struct sockaddr*) fromAddr,
      sizeof(struct sockaddr_in));

   return true;
}

