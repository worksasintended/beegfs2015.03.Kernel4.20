#include <common/net/message/control/GenericResponseMsg.h>
#include <common/net/message/storage/attribs/SetLocalAttrRespMsg.h>
#include <common/storage/StorageDefinitions.h>
#include <common/toolkit/MessagingTk.h>
#include <net/msghelpers/MsgHelperIO.h>
#include <program/Program.h>
#include <toolkit/StorageTkEx.h>
#include "SetLocalAttrMsgEx.h"

#include <utime.h>


bool SetLocalAttrMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   const char* logContext = "SetLocalAttrMsgEx incoming";

   #ifdef BEEGFS_DEBUG
      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, Log_DEBUG, "Received a SetLocalAttrMsg from: " + peer);
   #endif // BEEGFS_DEBUG

   App* app = Program::getApp();

   const SettableFileAttribs* attribs = getAttribs();
   int validAttribs = getValidAttribs();

   uint16_t targetID;
   bool chunkLocked = false;
   int targetFD;
   FhgfsOpsErr clientErrRes = FhgfsOpsErr_SUCCESS;
   DynamicFileAttribs currentDynAttribs(0, 0, 0, 0, 0);

   // select the right targetID

   targetID = getTargetID();

   if(isMsgHeaderFeatureFlagSet(SETLOCALATTRMSG_FLAG_BUDDYMIRROR) )
   { // given targetID refers to a buddy mirror group
      MirrorBuddyGroupMapper* mirrorBuddies = app->getMirrorBuddyGroupMapper();

      targetID = isMsgHeaderFeatureFlagSet(SETLOCALATTRMSG_FLAG_BUDDYMIRROR_SECOND) ?
         mirrorBuddies->getSecondaryTargetID(targetID) :
         mirrorBuddies->getPrimaryTargetID(targetID);

      if(unlikely(!targetID) )
      { // unknown group ID
         LogContext(logContext).logErr("Invalid mirror buddy group ID: " +
            StringTk::uintToStr(getTargetID() ) );
         clientErrRes = FhgfsOpsErr_UNKNOWNTARGET;
         goto send_response;
      }
   }

   { // get targetFD and check consistency state
      bool skipResponse = false;

      targetFD = getTargetFD(fromAddr, sock, respBuf, bufLen, targetID, &skipResponse);
      if(unlikely(targetFD == -1) )
      { // failed => either unknown targetID or consistency state not good
         if(skipResponse)
            goto skip_response; // GenericResponseMsg sent

         clientErrRes = FhgfsOpsErr_UNKNOWNTARGET;
         goto send_response;
      }
   }

   // forward to secondary (if appropriate)
   clientErrRes = forwardToSecondary(fromAddr, sock, respBuf, bufLen, targetID, &chunkLocked);
   if(unlikely(clientErrRes != FhgfsOpsErr_SUCCESS) )
   {
      if(clientErrRes == FhgfsOpsErr_COMMUNICATION)
         goto skip_response; // GenericResponseMsg sent

      goto send_response;
   }

   if(validAttribs & (SETATTR_CHANGE_MODIFICATIONTIME | SETATTR_CHANGE_LASTACCESSTIME) )
   { // we only handle access and modification time updates here
      struct timespec times[2] = {};

      if (validAttribs & SETATTR_CHANGE_LASTACCESSTIME)
      {
         times[MsgHelperIO_ATIME_POS].tv_sec  = attribs->lastAccessTimeSecs;
         times[MsgHelperIO_ATIME_POS].tv_nsec = 0;
      }
      else
         times[MsgHelperIO_ATIME_POS].tv_nsec  = UTIME_OMIT;

      if (validAttribs & SETATTR_CHANGE_MODIFICATIONTIME)
      {
         times[MsgHelperIO_MTIME_POS].tv_sec  = attribs->modificationTimeSecs;
         times[MsgHelperIO_MTIME_POS].tv_nsec = 0;
      }
      else
         times[MsgHelperIO_MTIME_POS].tv_nsec = UTIME_OMIT;

      // generate path to chunk file...

      std::string pathStr;

      pathStr = StorageTk::getFileChunkPath(getPathInfo(), getEntryID() );

      // update timestamps...

      // in case we need this extra information on the metadata server, generate a storageVersion,
      // set the new times while holding the lock and return the current attribs and a
      // storageVersion in response later
      if (isMsgHeaderCompatFeatureFlagSet (SETLOCALATTRMSG_COMPAT_FLAG_EXTEND_REPLY))
      {
         uint64_t storageVersion = Program::getApp()->getSyncedStoragePaths()->lockPath(
            getEntryID(), targetID);

         int utimeRes = MsgHelperIO::utimensat(targetFD, pathStr.c_str(), times, 0);

         if (utimeRes == 0)
         {
            bool getDynAttribsRes = StorageTkEx::getDynamicFileAttribs(targetFD, pathStr.c_str(),
               &currentDynAttribs.fileSize, &currentDynAttribs.numBlocks,
               &currentDynAttribs.modificationTimeSecs, &currentDynAttribs.lastAccessTimeSecs);

            // If stat failed (after utimensat worked!), something really bad happened, so the
            // attribs are definitely invalid. Otherwise set storageVersion in dynAttribs
            if (getDynAttribsRes)
            {
               currentDynAttribs.storageVersion = storageVersion;
            }
         }
         else if (errno == ENOENT)
         {
            // Entry doesn't exist. Not an error, but we need to return fake dynamic attributes for
            // the metadata server to calc the values (fake in this sense means, we send the
            // timestamps back that we tried to set, but have real filesize and numBlocks, i.e. 0
            currentDynAttribs.storageVersion = storageVersion;
            currentDynAttribs.fileSize = 0;
            currentDynAttribs.numBlocks = 0;
            currentDynAttribs.modificationTimeSecs = attribs->modificationTimeSecs;
            currentDynAttribs.lastAccessTimeSecs = attribs->lastAccessTimeSecs;
         }
         else
         {
            int errCode = errno;

            LogContext(logContext).logErr("Unable to change file time: " + pathStr + ". "
               "SysErr: " + System::getErrString());

            clientErrRes = fhgfsErrFromSysErr(errCode);
         }

         Program::getApp()->getSyncedStoragePaths()->unlockPath(getEntryID(), targetID);
      }
      else
      {
         int utimeRes = MsgHelperIO::utimensat(targetFD, pathStr.c_str(), times, 0);

         if (utimeRes == -1)
         {
            int errCode = errno;

            if (errCode != ENOENT)
            {
               LogContext(logContext).logErr("Unable to change file time: " + pathStr + ". "
                  "SysErr: " + System::getErrString() );

               clientErrRes = fhgfsErrFromSysErr(errCode);
            }
         }
      }
   }

   if(isMsgHeaderFeatureFlagSet(SETLOCALATTRMSG_FLAG_USE_QUOTA) &&
      (validAttribs & (SETATTR_CHANGE_USERID | SETATTR_CHANGE_GROUPID) ) )
   { // we only handle UID and GID updates here
      uid_t uid = -1;
      gid_t gid = -1;

      if(validAttribs & SETATTR_CHANGE_USERID)
         uid = attribs->userID;

      if(validAttribs & SETATTR_CHANGE_GROUPID)
         gid = attribs->groupID;

      // generate path to chunk file...

      std::string pathStr;

      pathStr = StorageTk::getFileChunkPath(getPathInfo(), getEntryID() );

      // update UID and GID...

      int chownRes = fchownat(targetFD, pathStr.c_str(), uid, gid, 0);
      if(chownRes == -1)
      { // could be an error
         int errCode = errno;

         if(errCode != ENOENT)
         { // unhandled chown() error
            LogContext(logContext).logErr("Unable to change file owner: " + pathStr + ". "
               "SysErr: " + System::getErrString() );

            clientErrRes = fhgfsErrFromSysErr(errCode);
         }
      }
   }

   

send_response:

   if(chunkLocked) // unlock chunk
      app->getChunkLockStore()->unlockChunk(targetID, getEntryID() );

   { // send response...
      SetLocalAttrRespMsg respMsg(clientErrRes, currentDynAttribs);
      respMsg.serialize(respBuf, bufLen);
      sock->sendto(respBuf, respMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );
   }

skip_response:

   // update operation counters...
   app->getNodeOpStats()->updateNodeOp(
      sock->getPeerIP(), StorageOpCounter_SETLOCALATTR, getMsgHeaderUserID() );

   return true;      
}

/**
 * @param outResponseSent true if a response was sent from within this method; can only be true if
 * -1 is returned.
 * @return -1 if no such target exists or if consistency state was not good (in which case a special
 * response is sent within this method), otherwise the file descriptor to chunks dir (or mirror
 * dir).
 */
int SetLocalAttrMsgEx::getTargetFD(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, uint16_t actualTargetID, bool* outResponseSent)
{
   const char* logContext = "TruncChunkFileMsg (get target FD)";

   App* app = Program::getApp();

   bool isBuddyMirrorChunk = isMsgHeaderFeatureFlagSet(SETLOCALATTRMSG_FLAG_BUDDYMIRROR);
   TargetConsistencyState consistencyState = TargetConsistencyState_BAD; // silence warning

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
      !isMsgHeaderFeatureFlagSet(SETLOCALATTRMSG_FLAG_BUDDYMIRROR_SECOND) )
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

/**
 * If this is a buddy mirror msg and we are the primary, forward this msg to secondary.
 *
 * @return _COMMUNICATION if forwarding to buddy failed and buddy is not marked offline (in which
 *    case *outChunkLocked==false is guaranteed).
 * @throw SocketException if sending of GenericResponseMsg fails.
 */
FhgfsOpsErr SetLocalAttrMsgEx::forwardToSecondary(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, uint16_t actualTargetID, bool* outChunkLocked)
{
   const char* logContext = "SetLocalAttrMsg incoming (forward to secondary)";

   App* app = Program::getApp();
   ChunkLockStore* chunkLockStore = app->getChunkLockStore();
   StorageTargets* storageTargets = app->getStorageTargets();

   *outChunkLocked = false;

   if(!isMsgHeaderFeatureFlagSet(SETLOCALATTRMSG_FLAG_BUDDYMIRROR) ||
       isMsgHeaderFeatureFlagSet(SETLOCALATTRMSG_FLAG_BUDDYMIRROR_SECOND) )
      return FhgfsOpsErr_SUCCESS; // nothing to do

   // mirrored chunk should be modified, check if resync is in progress and lock chunk
   *outChunkLocked = storageTargets->isBuddyResyncInProgress(actualTargetID);
   if(*outChunkLocked)
      chunkLockStore->lockChunk(actualTargetID, getEntryID() ); // lock chunk

   // instead of creating a new msg object, we just re-use "this" with "buddymirror second" flag
   addMsgHeaderFeatureFlag(SETLOCALATTRMSG_FLAG_BUDDYMIRROR_SECOND);

   RequestResponseArgs rrArgs(NULL, this, NETMSGTYPE_SetLocalAttrResp);
   RequestResponseTarget rrTarget(getTargetID(), app->getTargetMapper(), app->getStorageNodes(),
      app->getTargetStateStore(), app->getMirrorBuddyGroupMapper(), true);

   FhgfsOpsErr commRes = MessagingTk::requestResponseTarget(&rrTarget, &rrArgs);

   // remove the flag that we just added for secondary
   unsetMsgHeaderFeatureFlag(SETLOCALATTRMSG_FLAG_BUDDYMIRROR_SECOND);

   if(unlikely(
      (commRes == FhgfsOpsErr_COMMUNICATION) &&
      (rrTarget.outTargetReachabilityState == TargetReachabilityState_OFFLINE) ) )
   {
      LOG_DEBUG(logContext, Log_DEBUG, std::string("Secondary is offline and will need resync. ") +
         "mirror buddy group ID: " + StringTk::uintToStr(getTargetID() ) );

      // buddy is marked offline, so local msg processing will be done and buddy needs resync
      storageTargets->setBuddyNeedsResync(actualTargetID, true);

      return FhgfsOpsErr_SUCCESS; // go ahead with local msg processing
   }

   if(unlikely(commRes != FhgfsOpsErr_SUCCESS) )
   {
      LogContext(logContext).log(Log_DEBUG, "Forwarding failed: "
         "mirror buddy group ID: " + StringTk::uintToStr(getTargetID() ) + "; "
         "error: " + FhgfsOpsErrTk::toErrString(commRes) );

      if(*outChunkLocked)
      { // unlock chunk
         chunkLockStore->unlockChunk(actualTargetID, getEntryID() );
         *outChunkLocked = false;
      }

      std::string genericRespStr = "Communication with secondary failed. "
         "mirror buddy group ID: " + StringTk::uintToStr(getTargetID() );

      GenericResponseMsg respMsg(GenericRespMsgCode_INDIRECTCOMMERR, genericRespStr.c_str() );
      respMsg.serialize(respBuf, bufLen);
      sock->sendto(respBuf, respMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

      return FhgfsOpsErr_COMMUNICATION;
   }

   SetLocalAttrRespMsg* respMsg = (SetLocalAttrRespMsg*)rrArgs.outRespMsg;
   FhgfsOpsErr secondaryRes = respMsg->getResult();
   if(unlikely(secondaryRes != FhgfsOpsErr_SUCCESS) )
   {
      if(secondaryRes == FhgfsOpsErr_UNKNOWNTARGET)
      {
         /* local msg processing shall be done and buddy needs resync
            (this is normal when a storage is restarted without a broken secondary target, so we
            report success to a client in this case) */

         LogContext(logContext).log(Log_DEBUG,
            "Secondary reports unknown target error and will need resync. "
            "mirror buddy group ID: " + StringTk::uintToStr(getTargetID() ) );

         storageTargets->setBuddyNeedsResync(actualTargetID, true);

         return FhgfsOpsErr_SUCCESS;
      }

      LogContext(logContext).log(Log_NOTICE, std::string("Secondary reported error: ") +
         FhgfsOpsErrTk::toErrString(secondaryRes) + "; "
         "mirror buddy group ID: " + StringTk::uintToStr(getTargetID() ) );

      return secondaryRes;
   }

   return FhgfsOpsErr_SUCCESS;
}

/**
 * @param errCode positive(!) system error code
 */
FhgfsOpsErr SetLocalAttrMsgEx::fhgfsErrFromSysErr(int errCode)
{
   switch(errCode)
   {
      case 0:
      {
         return FhgfsOpsErr_SUCCESS;
      } break;
      
      case ENOSPC:
      {
         return FhgfsOpsErr_NOSPACE;
      } break;
   }

   return FhgfsOpsErr_INTERNAL;
}
