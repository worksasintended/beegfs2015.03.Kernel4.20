#include "DeleteChunksMsgEx.h"

#include <program/Program.h>
#include <toolkit/StorageTkEx.h>

bool DeleteChunksMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock, char* respBuf,
   size_t bufLen, HighResolutionStats* stats)
{
   const char* logContext = "DeleteChunksMsg incoming";

#ifdef BEEGFS_DEBUG
   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG(logContext, 4, std::string("Received a DeleteChunksMsg from: ") + peer);
#endif // BEEGFS_DEBUG

   App* app = Program::getApp();
   ChunkStore* chunkDirStore = app->getChunkDirStore();

   FsckChunkList chunks;
   FsckChunkList failedDeletes;
   this->parseChunks(&chunks);

   // todo christian: buddymirror - does this need to be adapted for buddymirroring? (probably not) -> done

   for ( FsckChunkListIter iter = chunks.begin(); iter != chunks.end(); iter++ )
   {
      std::string chunkDirRelative;
      std::string delPathStrRelative;
      bool isMirrorFD;

      if (iter->getBuddyGroupID()) // it's a mirror chunk
         isMirrorFD = true;
      else
         isMirrorFD = false;

      chunkDirRelative = iter->getSavedPath()->getPathAsStr();

      delPathStrRelative = chunkDirRelative + "/" + iter->getID();

      int targetFD = app->getTargetFD(iter->getTargetID(), isMirrorFD);

      if ( unlikely(targetFD == -1) )
      { // unknown targetID
         LogContext(logContext).logErr(std::string("Unknown targetID: ") +
            StringTk::uintToStr(iter->getTargetID()));
         failedDeletes.push_back(*iter);
      }
      else
      { // valid targetID
         int unlinkRes = unlinkat(targetFD, delPathStrRelative.c_str(), 0);
         if ( (unlinkRes == -1) && (errno != ENOENT) )
         { // error
            LogContext(logContext).logErr(
               "Unable to unlink file: " + delPathStrRelative + ". " + "SysErr: "
                  + System::getErrString());

            failedDeletes.push_back(*iter);
         }

         // Now try to rmdir chunkDirPath (checks if it is empty)
         if (unlinkRes == 0)
         {
            PathVec chunkDirRelativeVec(chunkDirRelative);
            chunkDirStore->rmdirChunkDirPath(targetFD, &chunkDirRelativeVec);
         }

      }
   }

   DeleteChunksRespMsg respMsg(&failedDeletes);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0, (struct sockaddr*) fromAddr,
      sizeof(struct sockaddr_in));

   return true;
}
