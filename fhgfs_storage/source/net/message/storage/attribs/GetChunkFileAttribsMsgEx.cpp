#include <common/net/message/control/GenericResponseMsg.h>
#include <common/net/message/storage/attribs/GetChunkFileAttribsRespMsg.h>
#include <program/Program.h>
#include <toolkit/StorageTkEx.h>
#include "GetChunkFileAttribsMsgEx.h"


bool GetChunkFileAttribsMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   const char* logContext = "GetChunkFileAttribsMsg incoming";

   #ifdef BEEGFS_DEBUG
      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, Log_DEBUG, "Received a GetChunkFileAttribsMsg from: " + peer);
   #endif // BEEGFS_DEBUG
   
   App* app = Program::getApp();

   std::string entryID(getEntryID() );

   FhgfsOpsErr clientErrRes = FhgfsOpsErr_SUCCESS;
   int targetFD;
   struct stat statbuf;
   uint64_t storageVersion = 0;

   // select the right targetID

   uint16_t targetID = getTargetID();

   if(isMsgHeaderFeatureFlagSet(GETCHUNKFILEATTRSMSG_FLAG_BUDDYMIRROR) )
   { // given targetID refers to a buddy mirror group
      MirrorBuddyGroupMapper* mirrorBuddies = app->getMirrorBuddyGroupMapper();

      targetID = isMsgHeaderFeatureFlagSet(GETCHUNKFILEATTRSMSG_FLAG_BUDDYMIRROR_SECOND) ?
         mirrorBuddies->getSecondaryTargetID(targetID) :
         mirrorBuddies->getPrimaryTargetID(targetID);

      // note: only log message here, error handling will happen below through invalid targetFD
      if(unlikely(!targetID) )
         LogContext(logContext).logErr("Invalid mirror buddy group ID: " +
            StringTk::uintToStr(getTargetID() ) );
   }

   { // get targetFD and check consistency state
      bool skipResponse = false;

      targetFD = getTargetFD(fromAddr, sock, respBuf, bufLen, targetID, &skipResponse);
      if(unlikely(targetFD == -1) )
      { // failed => either unknown targetID or consistency state not good
         memset(&statbuf, 0, sizeof(statbuf) ); // (just to mute clang warning)

         if(skipResponse)
            goto skip_response; // GenericResponseMsg sent

         clientErrRes = FhgfsOpsErr_UNKNOWNTARGET;
         goto send_response;
      }
   }

   { // valid targetID
      SyncedStoragePaths* syncedPaths = app->getSyncedStoragePaths();

      int statErrCode = 0;

      std::string chunkPath = StorageTk::getFileChunkPath(getPathInfo(), entryID);

      uint64_t newStorageVersion = syncedPaths->lockPath(entryID, targetID); // L O C K path

      int statRes = fstatat(targetFD, chunkPath.c_str(), &statbuf, 0);
      if(statRes)
      { // file not exists or error
         statErrCode = errno;
      }
      else
      {
         storageVersion = newStorageVersion;

         #ifdef BEEGFS_HSM
            // Grau HSM system might return a 0 size, if a file release was just issued, we can
            // prevent this by holding the file open; so if we got 0 size, we open the file and
            // double-check
            if ( statbuf.st_size == 0)
            {
               int openFlags = O_RDONLY | O_LARGEFILE | O_NOATIME;

               int fd = openat(targetFD, chunkPath.c_str(), openFlags);

               if ( fd >= 0 )
               {

                  int statRes = fstatat(targetFD, chunkPath.c_str(), &statbuf, 0);
                  if(statRes)
                  { // file not exists or error
                     statErrCode = errno;
                  }

                  close(fd);
               }
               else
                  statErrCode = errno;
            }
         #endif
      }

      syncedPaths->unlockPath(entryID, targetID); // U N L O C K path

      // note: non-existing file is not an error (storage version is 0, so nothing will be
      //    updated at the metadata node)

      if((statRes == -1) && (statErrCode != ENOENT))
      { // error
         clientErrRes = FhgfsOpsErr_INTERNAL;

         LogContext(logContext).logErr(
            "Unable to stat file: " + chunkPath + ". " + "SysErr: "
               + System::getErrString(statErrCode));
      }
   }

send_response:

   { // send response
      GetChunkFileAttribsRespMsg respMsg(clientErrRes, statbuf.st_size, statbuf.st_blocks,
         statbuf.st_mtime, statbuf.st_atime, storageVersion);
      respMsg.serialize(respBuf, bufLen);
      sock->sendto(respBuf, respMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );
   }


skip_response:

   app->getNodeOpStats()->updateNodeOp(
      sock->getPeerIP(), StorageOpCounter_GETLOCALFILESIZE, getMsgHeaderUserID() );

   return true;      
}

/**
 * @param outResponseSent true if a response was sent from within this method; can only be true if
 * -1 is returned.
 * @return -1 if no such target exists or if consistency state was not good (in which case a special
 * response is sent within this method), otherwise the file descriptor to chunks dir (or mirror
 * dir).
 */
int GetChunkFileAttribsMsgEx::getTargetFD(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, uint16_t actualTargetID, bool* outResponseSent)
{
   const char* logContext = "TruncChunkFileMsg (get target FD)";

   App* app = Program::getApp();

   bool isBuddyMirrorChunk = isMsgHeaderFeatureFlagSet(GETCHUNKFILEATTRSMSG_FLAG_BUDDYMIRROR);
   TargetConsistencyState consistencyState;

   *outResponseSent = false;

   // get targetFD and check consistency state

   int targetFD = app->getTargetFDAndConsistencyState(actualTargetID, isBuddyMirrorChunk,
      &consistencyState);

   if(unlikely(targetFD == -1) )
   { // unknown targetID
      if(isBuddyMirrorChunk)
      { /* buddy mirrored file => fail with GenericResp to make the caller retry.
           mgmt will mark this target as (p)offline in a few moments. */
         std::string respMsgLogStr = "Refusing request. "
            "Unknown targetID: " + StringTk::uintToStr(actualTargetID);

         GenericResponseMsg respMsg(GenericRespMsgCode_INDIRECTCOMMERR, respMsgLogStr.c_str() );
         respMsg.serialize(respBuf, bufLen);
         sock->sendto(respBuf, respMsg.getMsgLength(), 0,
            (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

         *outResponseSent = true;
         return -1;
      }

      LogContext(logContext).logErr("Unknown targetID: " + StringTk::uintToStr(actualTargetID) );

      return -1;
   }

   if(unlikely(consistencyState != TargetConsistencyState_GOOD) &&
      isBuddyMirrorChunk &&
      !isMsgHeaderFeatureFlagSet(GETCHUNKFILEATTRSMSG_FLAG_BUDDYMIRROR_SECOND) )
   { // this is a msg to a non-good primary
      std::string respMsgLogStr = "Refusing request. Target consistency is not good. "
         "targetID: " + StringTk::uintToStr(actualTargetID);

      GenericResponseMsg respMsg(GenericRespMsgCode_INDIRECTCOMMERR, respMsgLogStr.c_str() );
      respMsg.serialize(respBuf, bufLen);
      sock->sendto(respBuf, respMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

      *outResponseSent = true;
      return -1;
   }

   return targetFD;
}
