#include <common/components/streamlistenerv2/IncomingPreprocessedMsgWork.h>
#include <common/net/message/control/GenericResponseMsg.h>
#include <common/net/message/storage/attribs/SetAttrRespMsg.h>
#include <common/net/message/storage/attribs/SetLocalAttrMsg.h>
#include <common/net/message/storage/attribs/SetLocalAttrRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/VersionTk.h>
#include <components/worker/SetChunkFileAttribsWork.h>
#include <program/Program.h>
#include "SetAttrMsgEx.h"


bool SetAttrMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   #ifdef BEEGFS_DEBUG
      const char* logContext = "SetAttrMsgEx incoming";

      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, Log_DEBUG, std::string("Received a SetAttrMsg from: ") + peer);
   #endif // BEEGFS_DEBUG

   App* app = Program::getApp();
   Config* cfg = app->getConfig();
   MetaStore* metaStore = app->getMetaStore();

   FhgfsOpsErr setAttrRes;
   bool earlyResponse = false;
   
   EntryInfo* entryInfo = getEntryInfo();

   // update operation counters (here on top because we have an early sock release in this msg)
   app->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_SETATTR,
      getMsgHeaderUserID() );


   if ( entryInfo->getParentEntryID().empty() )
   { // special case: setAttr for root directory
      setAttrRes = setAttrRoot();
   }
   else if ( !DirEntryType_ISFILE(entryInfo->getEntryType()) )
   { // a directory
      setAttrRes = metaStore->setAttr(entryInfo, getValidAttribs(), getAttribs());
   }
   else
   { // a file
     // we need to reference the inode first, as we want to use it several times
      FileInode* inode = metaStore->referenceFile(entryInfo);
      if (inode)
      {
         // in the following we need to distinguish between several cases.
         // 1. if times shall be updated we need to send the update to the storage servers first
         //    because, we need to rely on storage server's attrib version to prevent races with
         //    other messages that update the times (e.g. Close)
         // 2. if times shall not be updated (must be chmod or chown then) and quota is enabled
         //    we first set the local attributes and then send the update to the storage server.
         //    if an early response optimization is set in this case we send the response between
         //    these two steps
         // 3. no times update (i.e. chmod or chown) and quota is enabled => only update locally,
         //    as we don't have a reason to waste time with contacting the storage servers

         bool timeUpdate =
            getValidAttribs() & (SETATTR_CHANGE_MODIFICATIONTIME | SETATTR_CHANGE_LASTACCESSTIME);

         if (timeUpdate) // time update and mode/owner update can never be at the same time
         {
            setAttrRes = setChunkFileAttribs(inode, true);

            if (setAttrRes == FhgfsOpsErr_SUCCESS)
            {
               setAttrRes = metaStore->setAttr(entryInfo, getValidAttribs(), getAttribs());
            }
         }
         else if (isMsgHeaderFeatureFlagSet(SETATTRMSG_FLAG_USE_QUOTA))
         {
            setAttrRes = metaStore->setAttr(entryInfo, getValidAttribs(), getAttribs());

            if (cfg->getQuotaEarlyChownResponse())
            { // allowed only if early chown respnse for quota is set
               SetAttrRespMsg respMsg(setAttrRes);
               respMsg.serialize(respBuf, bufLen);
               sock->sendto(respBuf, respMsg.getMsgLength(), 0, (struct sockaddr*) fromAddr,
                  sizeof(struct sockaddr_in));

               IncomingPreprocessedMsgWork::releaseSocket(app, &sock, this);

               earlyResponse = true;
            }

            if (setAttrRes == FhgfsOpsErr_SUCCESS)
               setAttrRes = setChunkFileAttribs(inode, false);
         }
         else
         {
            setAttrRes = metaStore->setAttr(entryInfo, getValidAttribs(), getAttribs());
         }

         metaStore->releaseFile(entryInfo->getParentEntryID(), inode);
      }
      else
         setAttrRes = FhgfsOpsErr_PATHNOTEXISTS;
   }

   if ( earlyResponse )
      return true;

   // send response

   if ( unlikely(setAttrRes == FhgfsOpsErr_COMMUNICATION) )
   { // forward comm error as indirect communication error to client
      GenericResponseMsg respMsg(GenericRespMsgCode_INDIRECTCOMMERR,
         "Communication with storage target failed");
      respMsg.serialize(respBuf, bufLen);
      sock->sendto(respBuf, respMsg.getMsgLength(), 0, (struct sockaddr*) fromAddr,
         sizeof(struct sockaddr_in));
   }
   else
   { // normal response
      SetAttrRespMsg respMsg(setAttrRes);
      respMsg.serialize(respBuf, bufLen);
      sock->sendto(respBuf, respMsg.getMsgLength(), 0, (struct sockaddr*) fromAddr,
         sizeof(struct sockaddr_in));
   }

   return true;
}


FhgfsOpsErr SetAttrMsgEx::setAttrRoot()
{
   DirInode* rootDir = Program::getApp()->getRootDir();

   uint16_t localNodeID = Program::getApp()->getLocalNode()->getNumID();

   if(localNodeID != rootDir->getOwnerNodeID() )
   {
      return FhgfsOpsErr_NOTOWNER;
   }

   if(!rootDir->setAttrData(getValidAttribs(), getAttribs() ) )
      return FhgfsOpsErr_INTERNAL;

   
   return FhgfsOpsErr_SUCCESS;
}

FhgfsOpsErr SetAttrMsgEx::setChunkFileAttribs(FileInode* file, bool requestDynamicAttribs)
{
   StripePattern* pattern = file->getStripePattern();

   if( (pattern->getStripeTargetIDs()->size() > 1) ||
       (pattern->getPatternType() == STRIPEPATTERN_BuddyMirror) )
      return setChunkFileAttribsParallel(file, requestDynamicAttribs);
   else
      return setChunkFileAttribsSequential(file, requestDynamicAttribs);
}

/**
 * Note: This method does not work for mirrored files; use setChunkFileAttribsParallel() for those.
 */
FhgfsOpsErr SetAttrMsgEx::setChunkFileAttribsSequential(FileInode* inode,
   bool requestDynamicAttribs)
{
   const char* logContext = "Set chunk file attribs S";
   
   StripePattern* pattern = inode->getStripePattern();
   const UInt16Vector* targetIDs = pattern->getStripeTargetIDs();
   TargetMapper* targetMapper = Program::getApp()->getTargetMapper();
   TargetStateStore* targetStates = Program::getApp()->getTargetStateStore();
   NodeStore* nodes = Program::getApp()->getStorageNodes();

   FhgfsOpsErr retVal = FhgfsOpsErr_SUCCESS;
   
   std::string fileID(inode->getEntryID() );

   PathInfo pathInfo;
   inode->getPathInfo(&pathInfo);

   // send request to each node and receive the response message
   unsigned currentTargetIndex = 0;
   for(UInt16VectorConstIter iter = targetIDs->begin();
      iter != targetIDs->end();
      iter++, currentTargetIndex++)
   {
      uint16_t targetID = *iter;
      bool enableFileCreation = (currentTargetIndex == 0); // enable inode creation of first node

      SetLocalAttrMsg setAttrMsg(fileID, targetID, &pathInfo, getValidAttribs(), getAttribs(),
         enableFileCreation);

      setAttrMsg.setMsgHeaderUserID(inode->getUserID() );

      if(isMsgHeaderFeatureFlagSet(SETATTRMSG_FLAG_USE_QUOTA))
         setAttrMsg.addMsgHeaderFeatureFlag(SETLOCALATTRMSG_FLAG_USE_QUOTA);

      if(requestDynamicAttribs)
         setAttrMsg.addMsgHeaderCompatFeatureFlag(SETLOCALATTRMSG_COMPAT_FLAG_EXTEND_REPLY);

      RequestResponseArgs rrArgs(NULL, &setAttrMsg, NETMSGTYPE_SetLocalAttrResp);
      RequestResponseTarget rrTarget(targetID, targetMapper, nodes);

      rrTarget.setTargetStates(targetStates);

      // send request to node and receive response
      FhgfsOpsErr requestRes = MessagingTk::requestResponseTarget(&rrTarget, &rrArgs);

      if(requestRes != FhgfsOpsErr_SUCCESS)
      { // communication error
         LogContext(logContext).log(Log_WARNING,
            "Communication with storage target failed: " + StringTk::uintToStr(targetID) + "; "
            "fileID: " + inode->getEntryID() + "; "
            "Error: " + FhgfsOpsErrTk::toErrString(requestRes) );

         if(retVal == FhgfsOpsErr_SUCCESS)
            retVal = requestRes;
         
         continue;
      }
      
      // correct response type received
      SetLocalAttrRespMsg* setRespMsg = (SetLocalAttrRespMsg*)rrArgs.outRespMsg;
      
      FhgfsOpsErr setRespResult = setRespMsg->getResult();
      if (setRespResult != FhgfsOpsErr_SUCCESS)
      { // error: local inode attribs not set
         LogContext(logContext).log(Log_WARNING,
            "Target failed to set attribs of chunk file: " + StringTk::uintToStr(targetID) + "; "
            "fileID: " + inode->getEntryID() );
         
         if(retVal == FhgfsOpsErr_SUCCESS)
            retVal = setRespResult;

         continue;
      }
      
      // success: local inode attribs set
      if (setRespMsg->isMsgHeaderCompatFeatureFlagSet(SETLOCALATTRRESPMSG_COMPAT_FLAG_HAS_ATTRS))
      {
         DynamicFileAttribsVec dynAttribsVec(1);
         setRespMsg->getDynamicAttribs(&(dynAttribsVec[0]));
         inode->setDynAttribs(dynAttribsVec);
      }

      LOG_DEBUG(logContext, Log_DEBUG,
         "Target has set attribs of chunk file: " + StringTk::uintToStr(targetID) + "; " +
         "fileID: " + inode->getEntryID() );
   }

   if(unlikely(retVal != FhgfsOpsErr_SUCCESS) )
      LogContext(logContext).log(Log_WARNING,
         "Problems occurred during setting of chunk file attribs. "
         "fileID: " + inode->getEntryID() );

   return retVal;
}

FhgfsOpsErr SetAttrMsgEx::setChunkFileAttribsParallel(FileInode* inode, bool requestDynamicAttribs)
{
   const char* logContext = "Set chunk file attribs";

   App* app = Program::getApp();
   MultiWorkQueue* slaveQ = app->getCommSlaveQueue();
   StripePattern* pattern = inode->getStripePattern();
   const UInt16Vector* targetIDs = pattern->getStripeTargetIDs();

   size_t numTargetWorks = targetIDs->size();

   FhgfsOpsErr retVal = FhgfsOpsErr_SUCCESS;
   DynamicFileAttribsVec dynAttribsVec(numTargetWorks);
   FhgfsOpsErrVec nodeResults(numTargetWorks);
   SynchronizedCounter counter;

   PathInfo pathInfo;
   inode->getPathInfo(&pathInfo);

   // generate work for storage targets...

   for(size_t i=0; i < numTargetWorks; i++)
   {
      bool enableFileCreation = (i == 0); // enable inode creation on first target

      SetChunkFileAttribsWork* work = NULL;
      if (requestDynamicAttribs) // we are interested in the chunk's dynamic attributes, because we
      {                          // modify timestamps and this operation might race with others
         work = new SetChunkFileAttribsWork(inode->getEntryID(), getValidAttribs(), getAttribs(),
            enableFileCreation, pattern, (*targetIDs)[i], &pathInfo, &(dynAttribsVec[i]),
            &(nodeResults[i]), &counter);
      }
      else
      {
         work = new SetChunkFileAttribsWork(inode->getEntryID(), getValidAttribs(), getAttribs(),
            enableFileCreation, pattern, (*targetIDs)[i], &pathInfo, NULL, &(nodeResults[i]),
            &counter);
      }

      work->setQuotaChown(isMsgHeaderFeatureFlagSet(SETATTRMSG_FLAG_USE_QUOTA) );
      work->setMsgUserID(getMsgHeaderUserID() );

      slaveQ->addDirectWork(work);
   }

   // wait for work completion...
   counter.waitForCount(numTargetWorks);

   // we set the dynamic attribs here, no matter if the remote operation suceeded or not. If it
   // did not, storageVersion will be zero and the corresponding data will be ignored
   // note: if the chunk's attributes were not requested from server at all, this is also OK here,
   // because the storageVersion will be 0
   inode->setDynAttribs(dynAttribsVec);

   // check target results...
   for(size_t i=0; i < numTargetWorks; i++)
   {
      if(unlikely(nodeResults[i] != FhgfsOpsErr_SUCCESS) )
      {
         LogContext(logContext).log(Log_WARNING,
            "Problems occurred during setting of chunk file attribs. "
            "fileID: " + inode->getEntryID() );

         retVal = nodeResults[i];
         goto error_exit;
      }
   }

error_exit:
   return retVal;
}
