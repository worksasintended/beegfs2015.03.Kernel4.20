#include <app/log/Logger.h>
#include <app/App.h>
#include <toolkit/NoAllocBufferStore.h>
#include <common/net/message/session/FSyncLocalFileMsg.h>
#include <common/net/message/session/FSyncLocalFileRespMsg.h>
#include <common/net/message/NetMessage.h>
#include <common/nodes/MirrorBuddyGroupMapper.h>
#include <common/nodes/TargetMapper.h>
#include <common/nodes/TargetStateStore.h>
#include <common/threading/Thread.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/VersionTk.h>
#include <common/components/worker/WorkQueue.h>
#include <nodes/NodeStoreEx.h>
#include "FSyncChunkFileWork.h"


void FSyncChunkFileWork_process(Work* this, char* bufIn, unsigned bufInLen,
   char* bufOut, unsigned bufOutLen)
{
   FSyncChunkFileWork* thisCast = (FSyncChunkFileWork*)this;

   const char* logContext = "fsync chunk work";

   Logger* log = App_getLogger(thisCast->app);
   TargetMapper* targetMapper = App_getTargetMapper(thisCast->app);
   NodeStoreEx* nodes = App_getStorageNodes(thisCast->app);
   uint16_t targetID = UInt16Vec_at(thisCast->targetIDs, thisCast->nodeIndex);

   Node* node;
   FhgfsOpsErr resolveErr;

   // select the right targetID

   if(StripePattern_getPatternType(thisCast->pattern) == STRIPEPATTERN_BuddyMirror)
   { // given targetID refers to a buddy mirror group
      uint16_t givenTargetID = targetID; // backup given targetID for log msg
      MirrorBuddyGroupMapper* mirrorBuddies = App_getMirrorBuddyGroupMapper(thisCast->app);

      targetID = thisCast->useBuddyMirrorSecond ?
         MirrorBuddyGroupMapper_getSecondaryTargetID(mirrorBuddies, targetID) :
         MirrorBuddyGroupMapper_getPrimaryTargetID(mirrorBuddies, targetID);

      // note: only log message here, error handling will happen below through invalid targetFD
      if(unlikely(!targetID) )
         Logger_logErrFormatted(log, logContext, "Invalid mirror buddy group ID: %hu",
            givenTargetID);
   }

   { // check if target is offline
      TargetStateStore* targetStates = App_getTargetStateStore(thisCast->app);
      CombinedTargetState targetState;
      bool getStateRes = TargetStateStore_getState(targetStates, targetID, &targetState);

      if(!getStateRes || (targetState.reachabilityState == TargetReachabilityState_OFFLINE) )
      { // don't try to communicate (and don't do retries) if target is offline
         // don't report error for for secondary mirror if it is offline
         if( (StripePattern_getPatternType(thisCast->pattern) == STRIPEPATTERN_BuddyMirror) &&
             (thisCast->useBuddyMirrorSecond) )
            (thisCast->nodeResults)[thisCast->nodeIndex] = FhgfsOpsErr_SUCCESS;
         else
            (thisCast->nodeResults)[thisCast->nodeIndex] = FhgfsOpsErr_COMMUNICATION;

         goto inc_counter_and_exit;
      }
   }

   node = NodeStoreEx_referenceNodeByTargetID(nodes, targetID, targetMapper, &resolveErr);

   if(unlikely(!node) )
   { // unable to resolve targetID
      (thisCast->nodeResults)[thisCast->nodeIndex] = resolveErr;
      goto inc_counter_and_exit;
   }

   if(thisCast->forceRemoteFlush || thisCast->checkSession || thisCast->doSyncOnClose)
   { // check if communication is required, got node reference => begin communication
      FhgfsOpsErr commRes = __FSyncChunkFileWork_communicate(
         thisCast, node, bufIn, bufInLen, bufOut, bufOutLen);

      NodeStoreEx_releaseNode(nodes, &node);

      // handle retry
      if(unlikely(commRes == FhgfsOpsErr_COMMUNICATION) )
      { // comm error occurred => prepare retry (if allowed)
         if(__FSyncChunkFileWork_prepareRetry(thisCast) )
         {
            // retry prepared => don't inc the done-counter and don't touch any member variables
            return;
         }
      }
   }
   else
   { // old server and no force remote flush set
      (thisCast->nodeResults)[thisCast->nodeIndex] = FhgfsOpsErr_SUCCESS;
      NodeStoreEx_releaseNode(nodes, &node);
   }

inc_counter_and_exit:
   SynchronizedCounter_incCount(thisCast->counter);
}

FhgfsOpsErr __FSyncChunkFileWork_communicate(FSyncChunkFileWork* this, Node* node,
   char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen)
{
   Node* localNode = App_getLocalNode(this->app);
   char* localNodeID = Node_getID(localNode);

   FSyncLocalFileMsg requestMsg;
   char* respBuf;
   NetMessage* respMsg;
   FSyncLocalFileRespMsg* fsyncResp;

   FhgfsOpsErr* nodeResult = &(this->nodeResults)[this->nodeIndex];
   uint16_t targetID = UInt16Vec_at(this->targetIDs, this->nodeIndex);

   // prepare request message
   FSyncLocalFileMsg_initFromSession(&requestMsg, localNodeID, this->fileHandleID, targetID);

   NetMessage_setMsgHeaderUserID( (NetMessage*)&requestMsg, this->msgUserID);
   NetMessage_setMsgHeaderTargetID( (NetMessage*)&requestMsg, targetID);

   if(StripePattern_getPatternType(this->pattern) == STRIPEPATTERN_BuddyMirror)
   {
      NetMessage_addMsgHeaderFeatureFlag( (NetMessage*)&requestMsg,
         FSYNCLOCALFILEMSG_FLAG_BUDDYMIRROR);

      if(this->useBuddyMirrorSecond)
         NetMessage_addMsgHeaderFeatureFlag( (NetMessage*)&requestMsg,
            FSYNCLOCALFILEMSG_FLAG_BUDDYMIRROR_SECOND);
   }

   // required version is checked in FSyncChunkFileWork_process and if session check is not
   // supported by the server, the value of this->checkSession was set to false
   if(this->checkSession && this->firstWriteDoneForTarget)
      NetMessage_addMsgHeaderFeatureFlag( (NetMessage*)&requestMsg,
         FSYNCLOCALFILEMSG_FLAG_SESSION_CHECK);

   // required version is checked in FSyncChunkFileWork_process and if syncOnClose is not
   // supported by the server, the value of this->doSyncOnClose was set to false
   if(!this->forceRemoteFlush && !this->doSyncOnClose)
      NetMessage_addMsgHeaderFeatureFlag( (NetMessage*)&requestMsg,
         FSYNCLOCALFILEMSG_FLAG_NO_SYNC);

   // connect & communicate

   *nodeResult = MessagingTk_requestResponse(this->app,
      node, (NetMessage*)&requestMsg, NETMSGTYPE_FSyncLocalFileResp, &respBuf, &respMsg);

   if(likely(*nodeResult == FhgfsOpsErr_SUCCESS) )
   {
      NoAllocBufferStore* bufStore = App_getMsgBufStore(this->app);

      // handle result
      fsyncResp = (FSyncLocalFileRespMsg*)respMsg;
      *nodeResult = FSyncLocalFileRespMsg_getValue(fsyncResp);

      // clean up
      NetMessage_virtualDestruct(respMsg);
      NoAllocBufferStore_addBuf(bufStore, respBuf);
   }

   // clean up
   FSyncLocalFileMsg_uninit( (NetMessage*)&requestMsg);

   return *nodeResult;
}

/**
 * @return fhgfs_true if a retry was prepared (e.g. caller shouldn't notify waiters), fhgfs_false
 * if no retry allowed and we're done
 */
fhgfs_bool __FSyncChunkFileWork_prepareRetry(FSyncChunkFileWork* this)
{
   RetryableWork* thisRetryWork = (RetryableWork*)this;
   Config* cfg = App_getConfig(this->app);
   unsigned maxNumRetries = Config_getConnNumCommRetries(cfg);

   if(RetryableWork_getIsInterrupted(thisRetryWork) )
   { // interrupted by initiator => no retry
      (this->nodeResults)[this->nodeIndex] = FhgfsOpsErr_INTERRUPTED;
   }
   else
   if( App_getConnRetriesEnabled(this->app) &&
       ( (!maxNumRetries) ||
         (thisRetryWork->currentRetryNum < maxNumRetries) ) )
   { // we have retries left => prepare and enqueue new (retry-)work

      // note: we have to create new work here because the worker will destroy this instance
      // note: we prefer to construct a new work instance and init it via the constructors
      //    (instead of copying from this instance) to be saver wrt side-effects

      WorkQueue* retryQ = App_getRetryWorkQueue(this->app);

      FSyncChunkFileWork* newWork;
      RetryableWork* newRetryWork;

      newWork = FSyncChunkFileWork_construct(this->app, this->nodeIndex,
         this->fileHandleID, this->pattern, this->targetIDs, this->nodeResults, this->counter,
         this->forceRemoteFlush, this->firstWriteDoneForTarget, this->checkSession,
         this->doSyncOnClose);
      FSyncChunkFileWork_setMsgUserID(newWork, this->msgUserID);
      FSyncChunkFileWork_setUseBuddyMirrorSecond(newWork, this->useBuddyMirrorSecond);

      newRetryWork = (RetryableWork*)newWork;
      RetryableWork_setInterruptor(newRetryWork, thisRetryWork->isInterrupted);

      newRetryWork->retryMS = MessagingTk_getRetryWaitMS(thisRetryWork->currentRetryNum);
      if(newRetryWork->retryMS)
      { // "retryWaitMS != 0" means we have to init the time object
         Time_init(&newRetryWork->lastRetryT);
      }

      newRetryWork->currentRetryNum = thisRetryWork->currentRetryNum + 1;

      WorkQueue_addWork(retryQ, (Work*)newWork);

      return fhgfs_true;
   }

   return fhgfs_false;
}
