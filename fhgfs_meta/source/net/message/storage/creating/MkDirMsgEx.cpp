#include <common/net/message/control/GenericResponseMsg.h>
#include <common/net/message/storage/creating/MkLocalDirMsg.h>
#include <common/net/message/storage/creating/MkLocalDirRespMsg.h>
#include <common/net/message/storage/creating/MkDirRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <components/ModificationEventFlusher.h>
#include <program/Program.h>
#include <storage/PosixACL.h>
#include "RmDirMsgEx.h"
#include "MkDirMsgEx.h"


bool MkDirMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   #ifdef BEEGFS_DEBUG
      const char* logContext = "MkDirMsg incoming";

      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, Log_DEBUG, std::string("Received a MkDirMsg from: ") + peer);
   #endif // BEEGFS_DEBUG

   App* app = Program::getApp();
   ModificationEventFlusher* modEventFlusher = app->getModificationEventFlusher();
   bool modEventLoggingEnabled = modEventFlusher->isLoggingEnabled();

   EntryInfo* parentInfo = getParentInfo();
   std::string newDirName = getNewDirName();

   EntryInfo newEntryInfo;

   FhgfsOpsErr mkRes = mkDir(parentInfo, newDirName, &newEntryInfo);

   if ( modEventLoggingEnabled )
   {
      std::string entryID = newEntryInfo.getEntryID();
      modEventFlusher->add(ModificationEvent_DIRCREATED, entryID);
   }

   // send response

   if(unlikely(mkRes == FhgfsOpsErr_COMMUNICATION) )
   { // forward comm error as indirect communication error to client
      GenericResponseMsg respMsg(GenericRespMsgCode_INDIRECTCOMMERR,
         "Communication with other metadata server failed");
      respMsg.serialize(respBuf, bufLen);
      sock->sendto(respBuf, respMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );
   }
   else
   { // normal response
      MkDirRespMsg respMsg(mkRes, &newEntryInfo );
      respMsg.serialize(respBuf, bufLen);
      sock->sendto(respBuf, respMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );
   }

   #ifdef BEEGFS_DEBUG
      if (mkRes != FhgfsOpsErr_SUCCESS)
      {
         LogContext(logContext).log(Log_DEBUG,
            "Failed to create directory. "
            "ParentID: " + parentInfo->getEntryID() + "; "
            "newDirName: " + newDirName + "; "
            "Error: " + FhgfsOpsErrTk::toErrString(mkRes) );
      }
   #endif

   // update operation counters
   app->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_MKDIR,
      getMsgHeaderUserID() );

   return true;
}


/**
 * @param parentInfo current directory
 * @param newName name of new entry
 * @param outEntryInfo newly created entry
 */
FhgfsOpsErr MkDirMsgEx::mkDir(EntryInfo* parentInfo, std::string newName,
   EntryInfo* outEntryInfo)
{
   const char* logContext = "MkDirMsg";

   App* app =  Program::getApp();
   MetaStore* metaStore = app->getMetaStore();
   Config* config = app->getConfig();
   NodeCapacityPools* metaCapacityPools = app->getMetaCapacityPools();

   FhgfsOpsErr retVal;

   // reference parent
   DirInode* parentDir = metaStore->referenceDir(parentInfo->getEntryID(), true);
   if(!parentDir)
      return FhgfsOpsErr_PATHNOTEXISTS;

   // check whether localNode owns this (parent) directory
   uint16_t localNodeID = app->getLocalNodeNumID();

   if(parentDir->getOwnerNodeID() != localNodeID)
   { // this node doesn't own the parent dir
      LogContext(logContext).logErr(std::string("Dir-owner mismatch: \"") +
         StringTk::int64ToStr(parentDir->getOwnerNodeID() )  + "\" vs. \"" +
         StringTk::int64ToStr(localNodeID) + "\"");
      metaStore->releaseDir(parentInfo->getEntryID() );
      return FhgfsOpsErr_NOTOWNER;
   }

   // choose new directory owner...

   bool useMirroring =
      ( (parentDir->getFeatureFlags() & DIRINODE_FEATURE_MIRRORED) && /* => parent has mirroring */
       !isMsgHeaderFeatureFlagSet(MKDIRMSG_FLAG_NOMIRROR) ); /* => no sender mirroring override */

   UInt16List preferredNodes;
   UInt16Vector newOwnerNodes;

   parsePreferredNodes(&preferredNodes);

   unsigned numDesiredTargets = useMirroring ? 2 : 1;
   unsigned minNumRequiredTargets = numDesiredTargets;

   metaCapacityPools->chooseStorageTargets(
      numDesiredTargets, minNumRequiredTargets, &preferredNodes, &newOwnerNodes);

   if(unlikely(newOwnerNodes.size() < minNumRequiredTargets) )
   { // (might be caused by a bad list of preferred targets)
      LogContext(logContext).logErr("No metadata servers available for new directory: " + newName);

      metaStore->releaseDir(parentInfo->getEntryID() );
      return FhgfsOpsErr_UNKNOWNNODE;
   }

   uint16_t ownerNodeID = newOwnerNodes[0];
   uint16_t mirrorNodeID = useMirroring ? newOwnerNodes[1] : 0;
   std::string entryID = StorageTk::generateFileID(localNodeID);
   std::string parentEntryID = parentInfo->getEntryID();

   int mode = getMode();
   int umask = getUmask();
   CharVector parentDefaultACLXAttr;
   CharVector accessACLXAttr;

   if (config->getStoreClientACLs() )
   {
      // Determine the ACLs of the new directory.
      bool needsACL;
      FhgfsOpsErr parentDefaultACLRes = metaStore->getDirXAttr(parentDir,
         PosixACL::defaultACLXAttrName, parentDefaultACLXAttr);

      if (parentDefaultACLRes == FhgfsOpsErr_SUCCESS)
      {
         // parent has a default ACL
         PosixACL parentDefaultACL;
         if (!parentDefaultACL.deserializeXAttr(parentDefaultACLXAttr) )
         {
            LogContext(logContext).log(Log_ERR,
               "Error deserializing directory default ACL for directory ID " + parentDir->getID() );
            retVal = FhgfsOpsErr_INTERNAL;
            goto clean_up;
         }

         // Note: This modifies the mode bits as well as the ACL itself.
         FhgfsOpsErr modeRes = parentDefaultACL.modifyModeBits(mode, needsACL);
         setMode(mode, 0);

         if (modeRes != FhgfsOpsErr_SUCCESS)
         {
            LogContext(logContext).log(Log_ERR, "Error generating access ACL for new directory "
               + newName);
            retVal = FhgfsOpsErr_INTERNAL;
            goto clean_up;
         }

         if (needsACL)
         {
            accessACLXAttr.resize(parentDefaultACL.serialLenXAttr() );
            parentDefaultACL.serializeXAttr(&accessACLXAttr.front() );
         }
      }
      else
      if (parentDefaultACLRes == FhgfsOpsErr_NODATA)
      {
         // containing dir has no ACL, so we can continue without one
         if (isMsgHeaderFeatureFlagSet(MKDIRMSG_FLAG_UMASK) )
         {
            mode &= ~umask;
            setMode(mode, umask);
         }
      }
      else
      {
         LogContext(logContext).log(Log_ERR,
            "Error loading default ACL for directory ID " + parentDir->getID() );
         retVal = parentDefaultACLRes;
         goto clean_up;
      }
   }

   outEntryInfo->set(ownerNodeID, parentEntryID, entryID, newName, DirEntryType_DIRECTORY, 0);

   // create remote dir metadata
   // (we create this before the dentry to reduce the risk dangling dentries)
   retVal = mkRemoteDirInode(parentDir, newName, outEntryInfo, mirrorNodeID, parentDefaultACLXAttr,
      accessACLXAttr);
   if(likely(retVal == FhgfsOpsErr_SUCCESS) )
   { // remote dir created => create dentry in parent dir

      retVal = mkDirDentry(parentDir, newName, outEntryInfo, mirrorNodeID);
      if(retVal != FhgfsOpsErr_SUCCESS)
      { // error (or maybe name just existed already) => compensate metaDir creation
         mkRemoteDirCompensate(outEntryInfo);
      }
   }

clean_up:
   metaStore->releaseDir(parentInfo->getEntryID() );

   return retVal;
}

/**
 * @param mirrorNodeID 0 for disabled mirroring
 */
FhgfsOpsErr MkDirMsgEx::mkDirDentry(DirInode* parentDir, std::string& name, EntryInfo* entryInfo,
   uint16_t mirrorNodeID)
{
   std::string entryID     = entryInfo->getEntryID();
   uint16_t ownerNodeID    = entryInfo->getOwnerNodeID();

   DirEntry* newDirDentry = new DirEntry(DirEntryType_DIRECTORY, name, entryID, ownerNodeID);

   if(mirrorNodeID)
      newDirDentry->setMirrorNodeID(mirrorNodeID);

   return parentDir->makeDirEntry(newDirDentry);
}

/**
 * Create dir inode on a remote server.
 *
 * @param name only used for logging
 * @param mirrorNodeID 0 for disabled mirroring
 */
FhgfsOpsErr MkDirMsgEx::mkRemoteDirInode(DirInode* parentDir, std::string& name,
   EntryInfo* entryInfo, uint16_t mirrorNodeID, const CharVector& defaultACLXAttr,
   const CharVector& accessACLXAttr)
{
   const char* logContext = "MkDirMsg (mk dir inode)";

   App* app = Program::getApp();

   FhgfsOpsErr retVal = FhgfsOpsErr_SUCCESS;

   StripePattern* pattern = parentDir->getStripePatternClone();
   uint16_t ownerNodeID = entryInfo->getOwnerNodeID();

   LOG_DEBUG(logContext, Log_DEBUG,
      "Creating dir inode at metadata node: " + StringTk::uintToStr(ownerNodeID) + "; "
      "dirname: " + name);

   // prepare request

   uint16_t parentNodeID = app->getLocalNode()->getNumID();
   MkLocalDirMsg mkMsg(entryInfo, getUserID(), getGroupID(), getMode(), pattern, parentNodeID,
      defaultACLXAttr, accessACLXAttr);

   if(mirrorNodeID)
      mkMsg.setMirrorNodeID(mirrorNodeID);

#ifdef BEEGFS_HSM_DEPRECATED
   mkMsg.setHsmCollocationID(parentDir->getHsmCollocationID());
#endif

   RequestResponseArgs rrArgs(NULL, &mkMsg, NETMSGTYPE_MkLocalDirResp);

   RequestResponseNode rrNode(ownerNodeID, app->getMetaNodes() );
   rrNode.setTargetStates(app->getMetaStateStore() );

   do // (this loop just exists to enable the "break"-jump, so it's not really a loop)
   {
      // send request to other mds and receive response

      FhgfsOpsErr requestRes = MessagingTk::requestResponseNode(&rrNode, &rrArgs);

      if(unlikely(requestRes != FhgfsOpsErr_SUCCESS) )
      { // communication error
         LogContext(logContext).log(Log_WARNING,
            "Communication with metadata server failed. "
            "nodeID: " + StringTk::uintToStr(ownerNodeID) + "; " +
            "dirname: " + name);
         retVal = requestRes;
         break;
      }

      // correct response type received
      MkLocalDirRespMsg* mkRespMsg = (MkLocalDirRespMsg*)rrArgs.outRespMsg;

      FhgfsOpsErr mkRemoteInodeRes = mkRespMsg->getResult();
      if(mkRemoteInodeRes != FhgfsOpsErr_SUCCESS)
      { // error: remote dir inode not created
         LogContext(logContext).log(Log_WARNING,
            "Metadata server failed to create dir inode. "
            "nodeID: " + StringTk::uintToStr(ownerNodeID) + "; " +
            "dirname: " + name);

         retVal = mkRemoteInodeRes;
         break;
      }

      // success: remote dir inode created
      LOG_DEBUG(logContext, Log_DEBUG,
         "Metadata server created dir inode. "
         "nodeID: " + StringTk::uintToStr(ownerNodeID) + "; "
         "dirname: " + name);

   } while(0);


   delete(pattern);

   return retVal;
}

/**
 * Remove dir metadata on a remote server to compensate for creation.
 */
FhgfsOpsErr MkDirMsgEx::mkRemoteDirCompensate(EntryInfo* entryInfo)
{
   LogContext log("MkDirMsg (undo dir inode [" + entryInfo->getFileName() + "])");

   FhgfsOpsErr rmRes = RmDirMsgEx::rmRemoteDirInode(entryInfo);

   if(unlikely(rmRes != FhgfsOpsErr_SUCCESS) )
   { // error
      log.log(Log_WARNING, std::string("Compensation not completely successful. ") +
         "File system might contain (uncritical) inconsistencies.");

      return rmRes;
   }

   log.log(Log_SPAM, "Creation of dir inode compensated");

   return rmRes;
}
