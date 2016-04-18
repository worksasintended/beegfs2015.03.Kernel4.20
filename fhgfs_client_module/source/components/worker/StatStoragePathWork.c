#include <app/log/Logger.h>
#include <app/App.h>
#include <common/net/message/storage/StatStoragePathMsg.h>
#include <common/net/message/storage/StatStoragePathRespMsg.h>
#include <common/net/message/NetMessage.h>
#include <common/nodes/TargetStateStore.h>
#include <common/threading/Thread.h>
#include <common/toolkit/MessagingTk.h>
#include <common/components/worker/WorkQueue.h>
#include <nodes/NodeStoreEx.h>
#include <toolkit/NoAllocBufferStore.h>
#include "StatStoragePathWork.h"


void StatStoragePathWork_process(Work* this, char* bufIn, unsigned bufInLen,
   char* bufOut, unsigned bufOutLen)
{
   StatStoragePathWork* thisCast = (StatStoragePathWork*)this;

   StatStoragePathData* statData = thisCast->statData;
   TargetMapper* targetMapper = App_getTargetMapper(thisCast->app);
   NodeStoreEx* nodes = App_getStorageNodes(thisCast->app);
   Node* node;

   FhgfsOpsErr resolveErr;


   { // check if target is offline
      TargetStateStore* targetStates = App_getTargetStateStore(thisCast->app);
      CombinedTargetState targetState;
      bool getStateRes = TargetStateStore_getState(targetStates, statData->targetID, &targetState);

      if(!getStateRes || (targetState.reachabilityState == TargetReachabilityState_OFFLINE) )
      { // don't try to communicate (and don't do retries) if target is offline
         statData->outResult = FhgfsOpsErr_COMMUNICATION;
         goto inc_counter_and_exit;
      }
   }

   node = NodeStoreEx_referenceNodeByTargetID(nodes, statData->targetID, targetMapper,
      &resolveErr);
   if(unlikely(!node) )
   { // unable to resolve targetID
      statData->outResult = resolveErr;
   }
   else
   { // got node reference => begin communication
      FhgfsOpsErr commRes = __StatStoragePathWork_communicate(
         thisCast, node, bufIn, bufInLen, bufOut, bufOutLen);

      NodeStoreEx_releaseNode(nodes, &node);

      // handle retry
      if(unlikely(commRes == FhgfsOpsErr_COMMUNICATION) )
      { // comm error occurred => prepare retry (if allowed)
         if(__StatStoragePathWork_prepareRetry(thisCast) )
         {
            // retry prepared => don't inc the done-counter and don't touch any shared variables
            return;
         }
      }
   }


inc_counter_and_exit:
   SynchronizedCounter_incCount(thisCast->counter);

   // note: result value is provided through outResult, which is assigned by _communicate()
}

FhgfsOpsErr __StatStoragePathWork_communicate(StatStoragePathWork* this, Node* node,
   char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen)
{
   StatStoragePathData* statData = this->statData;

   StatStoragePathMsg requestMsg;
   char* respBuf;
   NetMessage* respMsg;
   StatStoragePathRespMsg* statResp;

   // prepare request message
   StatStoragePathMsg_initFromTarget(&requestMsg, statData->targetID);

   NetMessage_setMsgHeaderTargetID( (NetMessage*)&requestMsg, statData->targetID);

   // connect & communicate

   statData->outResult = MessagingTk_requestResponse(this->app,
      node, (NetMessage*)&requestMsg, NETMSGTYPE_StatStoragePathResp, &respBuf, &respMsg);

   if(likely(statData->outResult == FhgfsOpsErr_SUCCESS) )
   {
      NoAllocBufferStore* bufStore = App_getMsgBufStore(this->app);

      // handle result
      statResp = (StatStoragePathRespMsg*)respMsg;
      statData->outResult = StatStoragePathRespMsg_getResult(statResp);
      statData->outSizeTotal = StatStoragePathRespMsg_getSizeTotal(statResp);
      statData->outSizeFree = StatStoragePathRespMsg_getSizeFree(statResp);

      // clean up
      NetMessage_virtualDestruct(respMsg);
      NoAllocBufferStore_addBuf(bufStore, respBuf);
   }

   // clean up
   StatStoragePathMsg_uninit( (NetMessage*)&requestMsg);

   return statData->outResult;
}

/**
 * @return fhgfs_true if a retry was prepared (e.g. caller shouldn't notify waiters), fhgfs_false
 * if no retry allowed and we're done
 */
fhgfs_bool __StatStoragePathWork_prepareRetry(StatStoragePathWork* this)
{
   RetryableWork* thisRetryWork = (RetryableWork*)this;
   Config* cfg = App_getConfig(this->app);
   unsigned maxNumRetries = Config_getConnNumCommRetries(cfg);

   if(RetryableWork_getIsInterrupted(thisRetryWork) )
   { // interrupted by initiator => no retry
      this->statData->outResult = FhgfsOpsErr_INTERRUPTED;
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

      StatStoragePathWork* newWork = StatStoragePathWork_construct(this->app, this->statData,
         this->counter);

      RetryableWork* newRetryWork = (RetryableWork*)newWork;
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
