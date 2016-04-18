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


   if(entryInfo->getParentEntryID().empty() )
   { // special case: setAttr for root directory
      setAttrRes = setAttrRoot();
   }
   else
   if(!DirEntryType_ISFILE(entryInfo->getEntryType() ) )
   { // a directory
      setAttrRes = metaStore->setAttr(entryInfo, getValidAttribs(), getAttribs() );
   }
   else
   { // a file
      // we need to reference the inode first, as we want to use it several times
      FileInode* inode = metaStore->referenceFile(entryInfo);
      if (inode)
      {
         setAttrRes = metaStore->setAttr(entryInfo, getValidAttribs(), getAttribs() );

         bool timeUpdate =
            getValidAttribs() & (SETATTR_CHANGE_MODIFICATIONTIME | SETATTR_CHANGE_LASTACCESSTIME);

         if( (setAttrRes == FhgfsOpsErr_SUCCESS) &&
            (isMsgHeaderFeatureFlagSet(SETATTRMSG_FLAG_USE_QUOTA) || timeUpdate) )
         { // we have set timestamp or owner attribs => send info to storage targets

            if(cfg->getQuotaEarlyChownResponse() && !timeUpdate)
            { // allowed only if chown for quota is set (and not if other things set as well)
               SetAttrRespMsg respMsg(setAttrRes);
               respMsg.serialize(respBuf, bufLen);
               sock->sendto(respBuf, respMsg.getMsgLength(), 0,
                  (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

               IncomingPreprocessedMsgWork::releaseSocket(app, &sock, this);

               earlyResponse = true;
            }

            setAttrRes = setChunkFileAttribs(inode);
         }

         metaStore->releaseFile(entryInfo->getParentEntryID(), inode);
      }
      else
         setAttrRes = FhgfsOpsErr_PATHNOTEXISTS;
   }

   if(earlyResponse)
      return true;

   // send response

   if(unlikely(setAttrRes == FhgfsOpsErr_COMMUNICATION) )
   { // forward comm error as indirect communication error to client
      GenericResponseMsg respMsg(GenericRespMsgCode_INDIRECTCOMMERR,
         "Communication with storage target failed");
      respMsg.serialize(respBuf, bufLen);
      sock->sendto(respBuf, respMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );
   }
   else
   { // normal response
      SetAttrRespMsg respMsg(setAttrRes);
      respMsg.serialize(respBuf, bufLen);
      sock->sendto(respBuf, respMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );
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

FhgfsOpsErr SetAttrMsgEx::setChunkFileAttribs(FileInode* file)
{
   StripePattern* pattern = file->getStripePattern();

   if( (pattern->getStripeTargetIDs()->size() > 1) ||
       (pattern->getPatternType() == STRIPEPATTERN_BuddyMirror) )
      return setChunkFileAttribsParallel(file);
   else
      return setChunkFileAttribsSequential(file);
}

/**
 * Note: This method does not work for mirrored files; use setChunkFileAttribsParallel() for those.
 */
FhgfsOpsErr SetAttrMsgEx::setChunkFileAttribsSequential(FileInode* inode)
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
      
      if(setRespMsg->getValue() != FhgfsOpsErr_SUCCESS)
      { // error: local inode attribs not set
         LogContext(logContext).log(Log_WARNING,
            "Target failed to set attribs of chunk file: " + StringTk::uintToStr(targetID) + "; "
            "fileID: " + inode->getEntryID() );
         
         if(retVal == FhgfsOpsErr_SUCCESS)
            retVal = setRespMsg->getResult();

         continue;
      }
      
      // success: local inode attribs set
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

FhgfsOpsErr SetAttrMsgEx::setChunkFileAttribsParallel(FileInode* inode)
{
   const char* logContext = "Set chunk file attribs";

   App* app = Program::getApp();
   MultiWorkQueue* slaveQ = app->getCommSlaveQueue();
   StripePattern* pattern = inode->getStripePattern();
   const UInt16Vector* targetIDs = pattern->getStripeTargetIDs();

   size_t numTargetWorks = targetIDs->size();

   FhgfsOpsErr retVal = FhgfsOpsErr_SUCCESS;
   FhgfsOpsErrVec nodeResults(numTargetWorks);
   SynchronizedCounter counter;

   PathInfo pathInfo;
   inode->getPathInfo(&pathInfo);

   // generate work for storage targets...

   for(size_t i=0; i < numTargetWorks; i++)
   {
      bool enableFileCreation = (i == 0); // enable inode creation on first target

      SetChunkFileAttribsWork* work = new SetChunkFileAttribsWork(
         inode->getEntryID(), getValidAttribs(), getAttribs(), enableFileCreation, pattern,
         (*targetIDs)[i], &pathInfo, &(nodeResults[i]), &counter);

      work->setQuotaChown(isMsgHeaderFeatureFlagSet(SETATTRMSG_FLAG_USE_QUOTA) );
      work->setMsgUserID(getMsgHeaderUserID() );

      slaveQ->addDirectWork(work);
   }

   // wait for work completion...

   counter.waitForCount(numTargetWorks);

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
