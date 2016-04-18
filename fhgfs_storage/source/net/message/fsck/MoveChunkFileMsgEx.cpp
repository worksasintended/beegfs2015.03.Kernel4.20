#include "MoveChunkFileMsgEx.h"

#include <program/Program.h>

bool MoveChunkFileMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock, char* respBuf,
   size_t bufLen, HighResolutionStats* stats)
{
   const char* logContext = "MoveChunkFileMsg incoming";

#ifdef BEEGFS_DEBUG
   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG(logContext, 4, std::string("Received a MoveChunkFileMsg from: ") + peer);
#endif // BEEGFS_DEBUG

   unsigned result = 0;

   App* app = Program::getApp();

   std::string chunkName = this->getChunkName();
   std::string oldPath = this->getOldPath(); // relative path to chunks dir
   std::string newPath = this->getNewPath(); // relative path to chunks dir
   uint16_t targetID = this->getTargetID();
   uint16_t mirroredFromTargetID = this->getMirroredFromTargetID();
   bool overwriteExisting = this->getOverwriteExisting();

   std::string targetPath;
   int renameRes;

   // todo christian: buddymirror - needs to be adapted for buddymirroring

   app->getStorageTargets()->getPath(targetID, &targetPath);

   bool isMirrorFD;

   if (mirroredFromTargetID) // it's a mirror chunk
   {
      isMirrorFD = true;

      // add mirror component to relative path
      targetPath += "/" CONFIG_BUDDYMIRROR_SUBDIR_NAME;
   }
   else
   {
      isMirrorFD = false;

      targetPath += "/" CONFIG_CHUNK_SUBDIR_NAME;
   }

   std::string moveFrom = oldPath + "/" + chunkName;
   std::string moveTo = newPath + "/" + chunkName;

   int targetFD = app->getTargetFD(targetID, isMirrorFD);

   if ( unlikely(targetFD == -1) )
   {
      LogContext(logContext).log(Log_CRITICAL, "Could not open path for target ID; targetID: "
         + StringTk::uintToStr(targetID));
      result = 1;
      goto sendResp;
   }

   // if overwriteExisting set to false, make sure, that output file does not exist
   if (!overwriteExisting)
   {
      bool pathExists = StorageTk::pathExists(targetFD, moveTo);
      if (pathExists)
      {
         LogContext(logContext).log(Log_CRITICAL,
            "Could not move chunk file. Destination file does already exist; chunkID: " + chunkName
               + "; targetID: " + StringTk::uintToStr(targetID) + "; oldChunkPath: " + oldPath
               + "; newChunkPath: " + newPath);
         result = 1;
         goto sendResp;
      }
   }

   {
      // create the parent directory (perhaps it didn't exist)
      // can be more efficient if we write a createPathOnDisk that uses mkdirat
      Path moveToPath = Path(targetPath + "/" + moveTo);
      mode_t dirMode = S_IRWXU | S_IRWXG | S_IRWXO;
      bool mkdirRes = StorageTk::createPathOnDisk(moveToPath, true, &dirMode);

      if(!mkdirRes)
      {
         LogContext(logContext).log(Log_CRITICAL,
            "Could not create parent directory for chunk; chunkID: " + chunkName + "; targetID: "
               + StringTk::uintToStr(targetID) + "; oldChunkPath: " + oldPath + "; newChunkPath: "
               + newPath);
         result = 1;
         goto sendResp;
      }
   }

   // perform the actual move
   renameRes = renameat(targetFD, moveFrom.c_str(), targetFD, moveTo.c_str() );
   if ( renameRes != 0 )
   {
      LogContext(logContext).log(Log_CRITICAL,
         "Could not perform move; chunkID: " + chunkName + "; targetID: "
            + StringTk::uintToStr(targetID) + "; oldChunkPath: " + oldPath + "; newChunkPath: "
            + newPath + "; SysErr: " + System::getErrString());
      result = 1;
   }

   sendResp:

   MoveChunkFileRespMsg respMsg(result);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0, (struct sockaddr*) fromAddr,
      sizeof(struct sockaddr_in));

   return true;
}
