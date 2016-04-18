#include <app/App.h>
#include <common/net/message/session/rw/ReadLocalFileV2Msg.h>
#include <common/net/message/session/rw/WriteLocalFileMsg.h>
#include <common/net/message/session/rw/WriteLocalFileRespMsg.h>
#include <common/nodes/MirrorBuddyGroupMapper.h>
#include <common/nodes/TargetStateStore.h>
#include <common/storage/StorageErrors.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/VersionTk.h>
#include <common/threading/Thread.h>
#include "FhgfsOpsCommKit.h"


#define COMMKIT_RESET_SLEEP_MS        5000 /* how long to sleep if target state not good/offline */


/**
 * @return fhgfs_false on error (means 'break' for the surrounding switch statement)
 */
fhgfs_bool __FhgfsOpsCommKit_readfileV2bStagePREPARE(RWfileStatesHelper* statesHelper,
   ReadfileState* currentState)
{
   TargetMapper* targetMapper = App_getTargetMapper(statesHelper->app);
   NodeStoreEx* storageNodes = App_getStorageNodes(statesHelper->app);
   Node* localNode = App_getLocalNode(statesHelper->app);
   const char* localNodeID = Node_getID(localNode);

   FhgfsOpsErr resolveErr;
   NodeConnPool* connPool;
   ReadLocalFileV2Msg readMsg;
   fhgfs_bool allowWaitForConn = !statesHelper->numAcquiredConns; // don't wait if we got at least
      // one conn already (this is important to avoid a deadlock between racing commkit processes)

   uint16_t nodeReferenceTargetID = currentState->targetID; /* to avoid overriding original targetID
      in case of buddy-mirroring, because we need the mirror group ID for the msg and for retries */

   currentState->sock = NULL;
   currentState->nodeResult = -FhgfsOpsErr_COMMUNICATION;

   // select the right targetID and get target state

   if(StripePattern_getPatternType(statesHelper->ioInfo->pattern) == STRIPEPATTERN_BuddyMirror)
   { // given targetID refers to a buddy mirror group
      MirrorBuddyGroupMapper* mirrorBuddies = App_getMirrorBuddyGroupMapper(statesHelper->app);

      nodeReferenceTargetID = currentState->useBuddyMirrorSecond ?
         MirrorBuddyGroupMapper_getSecondaryTargetID(mirrorBuddies, currentState->targetID) :
         MirrorBuddyGroupMapper_getPrimaryTargetID(mirrorBuddies, currentState->targetID);

      if(unlikely(!nodeReferenceTargetID) )
      { // invalid mirror group ID
         Logger_logErrFormatted(statesHelper->log, statesHelper->logContext,
            "Invalid mirror buddy group ID: %hu", currentState->targetID);
         currentState->nodeResult = -FhgfsOpsErr_UNKNOWNTARGET;
         currentState->stage = ReadfileStage_CLEANUP;
         return fhgfs_false;
      }
   }

   // check target state

   {
      TargetStateStore* stateStore = App_getTargetStateStore(statesHelper->app);
      CombinedTargetState targetState;
      fhgfs_bool getStateRes = TargetStateStore_getState(stateStore, nodeReferenceTargetID,
         &targetState);

      if(unlikely( !getStateRes ||
          (targetState.reachabilityState == TargetReachabilityState_OFFLINE) ||
          ( (StripePattern_getPatternType(statesHelper->ioInfo->pattern) ==
             STRIPEPATTERN_BuddyMirror) &&
            (targetState.consistencyState != TargetConsistencyState_GOOD) ) ) )
      { // unusable target state, retry details will be handled in _readfileV2bPrepareRetry()
         currentState->stage = ReadfileStage_CLEANUP;
         return fhgfs_false;
      }
   }

   // get the target-node reference
   currentState->node = NodeStoreEx_referenceNodeByTargetID(storageNodes,
      nodeReferenceTargetID, targetMapper, &resolveErr);
   if(unlikely(!currentState->node) )
   { // unable to resolve targetID
      currentState->nodeResult = -resolveErr;
      currentState->stage = ReadfileStage_CLEANUP;
      return fhgfs_false;
   }

   connPool = Node_getConnPool(currentState->node);

   // connect
   currentState->sock = NodeConnPool_acquireStreamSocketEx(connPool, allowWaitForConn);
   if(!currentState->sock)
   { // no conn available => error or didn't want to wait
      if(likely(!allowWaitForConn) )
      { // just didn't want to wait => keep stage and try again later
         NodeStoreEx_releaseNode(storageNodes, &currentState->node);

         statesHelper->numUnconnectable++;
      }
      else
      { // connection error
         if(!statesHelper->connFailedLogged)
         { // no conn error logged yet
            if(Thread_isSignalPending() )
               Logger_logFormatted(statesHelper->log, Log_DEBUG, statesHelper->logContext,
                  "Connect to server canceled by pending signal: %s",
                  Node_getNodeIDWithTypeStr(currentState->node) );
            else
               Logger_logFormatted(statesHelper->log, Log_WARNING, statesHelper->logContext,
                  "Unable to connect to server: %s",
                  Node_getNodeIDWithTypeStr(currentState->node) );
         }

         statesHelper->connFailedLogged = fhgfs_true;
         currentState->stage = ReadfileStage_CLEANUP;
      }

      // in either case return fhgfs_false to avoid falling through to send stage
      return fhgfs_false;
   }

   statesHelper->numAcquiredConns++;


   // prepare message
   ReadLocalFileV2Msg_initFromSession(&readMsg, localNodeID,
      statesHelper->ioInfo->fileHandleID, currentState->targetID, statesHelper->ioInfo->pathInfo,
      statesHelper->ioInfo->accessFlags, currentState->offset, currentState->data.count);

   NetMessage_setMsgHeaderTargetID( (NetMessage*)&readMsg, nodeReferenceTargetID);

   if (currentState->firstWriteDoneForTarget)
      NetMessage_addMsgHeaderFeatureFlag( (NetMessage*)&readMsg,
         READLOCALFILEMSG_FLAG_SESSION_CHECK);

   if(StripePattern_getPatternType(statesHelper->ioInfo->pattern) == STRIPEPATTERN_BuddyMirror)
   {
      NetMessage_addMsgHeaderFeatureFlag( (NetMessage*)&readMsg, READLOCALFILEMSG_FLAG_BUDDYMIRROR);

      if(currentState->useBuddyMirrorSecond)
         NetMessage_addMsgHeaderFeatureFlag( (NetMessage*)&readMsg,
            READLOCALFILEMSG_FLAG_BUDDYMIRROR_SECOND);
   }

   if(App_getNetBenchModeEnabled(statesHelper->app) )
      NetMessage_addMsgHeaderFeatureFlag( (NetMessage*)&readMsg, READLOCALFILEMSG_FLAG_DISABLE_IO);

   NetMessage_serialize( (NetMessage*)&readMsg, currentState->msgBuf, BEEGFS_COMMKIT_MSGBUF_SIZE);

   currentState->transmitted = 0;
   currentState->toBeTransmitted = NetMessage_getMsgLength( (NetMessage*)&readMsg);

   ReadLocalFileV2Msg_uninit( (NetMessage*)&readMsg);

   currentState->stage = ReadfileStage_SEND;

   return fhgfs_true;
}

void __FhgfsOpsCommKit_readfileV2bStageSEND(RWfileStatesHelper* statesHelper,
   ReadfileState* currentState)
{
   ssize_t sendRes;

   sendRes = currentState->sock->send(
      currentState->sock, currentState->msgBuf, currentState->toBeTransmitted, 0);

#if (BEEGFS_COMMKIT_DEBUG & COMMKIT_DEBUG_READ_SEND )
   if (sendRes > 0 && jiffies % CommKitErrorInjectRate == 0)
   {
      Logger_logTopFormatted(statesHelper->log, LogTopic_COMMKIT, Log_WARNING, __func__,
         "Injecting timeout error after (successfully) sending data.");
      sendRes = -ETIMEDOUT;
   }
#endif

   if(unlikely(sendRes <= 0) )
   {
      Logger_logFormatted(statesHelper->log, Log_WARNING, statesHelper->logContext,
         "Failed to send message to %s: %s", Node_getNodeIDWithTypeStr(currentState->node),
         Socket_getPeername(currentState->sock) );

      currentState->stage = ReadfileStage_SOCKETEXCEPTION;
      return;
   }

   currentState->stage = ReadfileStage_RECVHEADER;

   // prepare polling for the next stage
   __FhgfsOpsCommKit_readfileAddStateSockToPollSocks(statesHelper, currentState);

   // prepare transmission counters for the next stage
   currentState->transmitted = 0;
   currentState->toBeTransmitted = 0;
}

void __FhgfsOpsCommKit_readfileV2bStageRECVHEADER(RWfileStatesHelper* statesHelper,
   ReadfileState* currentState)
{
   ssize_t recvRes;
   char dataLenBuf[sizeof(int64_t)]; // length info in fhgfs network byte order
   int64_t lengthInfo; // length info in fhgfs host byte order
   unsigned lengthInfoBufLen; // deserialization buf length for lengthInfo (ignored)


   // check for incoming data
   if(!__FhgfsOpsCommKit_readfileV2bIsDataAvailable(statesHelper, currentState) )
      return;


   recvRes = Socket_recvExactT(
      currentState->sock, &dataLenBuf, sizeof(int64_t), 0, CONN_LONG_TIMEOUT);

#if (BEEGFS_COMMKIT_DEBUG & COMMKIT_DEBUG_READ_HEADER)
   if (recvRes > 0 && jiffies % CommKitErrorInjectRate == 0)
   {
      Logger_logTopFormatted(statesHelper->log, LogTopic_COMMKIT, Log_WARNING, __func__,
         "Injecting timeout error after (successfully) receiving data.");
      recvRes = -ETIMEDOUT;
   }
#endif

   if(unlikely(recvRes <= 0) )
   { // error
      Logger_logFormatted(statesHelper->log, Log_WARNING, statesHelper->logContext,
         "Failed to receive the length info response from %s: %s",
         Node_getNodeIDWithTypeStr(currentState->node), Socket_getPeername(currentState->sock) );

      currentState->stage = ReadfileStage_SOCKETEXCEPTION;
      return;
   }

   // got the length info response
   Serialization_deserializeInt64(dataLenBuf, sizeof(int64_t), &lengthInfo, &lengthInfoBufLen);

   //LOG_DEBUG_FORMATTED(statesHelper->log, 5, statesHelper->logContext,
   //   "Received lengthInfo from %s: %lld", currentState->nodeID, (long long)lengthInfo);

   if(lengthInfo <= 0)
   { // end of file data transmission
      NodeConnPool_releaseStreamSocket(Node_getConnPool(currentState->node), currentState->sock);
      statesHelper->numAcquiredConns--;

      if(unlikely(lengthInfo < 0) )
      { // error occurred
         currentState->nodeResult = lengthInfo;
      }
      else
      { // normal end of file data transmission
         currentState->nodeResult = currentState->transmitted;
      }

      currentState->stage = ReadfileStage_CLEANUP;

      return;
   }

   // buffer overflow check
   if(unlikely(lengthInfo > currentState->data.count) )
   {
      Logger_logErrFormatted(statesHelper->log, statesHelper->logContext,
         "Bug: Received a lengthInfo that would lead to a buffer overflow from %s: %lld %lld %zu",
         Node_getNodeIDWithTypeStr(currentState->node), (long long)lengthInfo,
         currentState->transmitted, currentState->data.count);

      currentState->nodeResult = -FhgfsOpsErr_INTERNAL;
      currentState->stage = ReadfileStage_SOCKETINVALIDATE;

      return;
   }

   // positive result => node is going to send some file data
   currentState->toBeTransmitted += lengthInfo;
   currentState->stage = ReadfileStage_RECVDATA;
}

void __FhgfsOpsCommKit_readfileV2bStageRECVDATA(RWfileStatesHelper* statesHelper,
   ReadfileState* currentState)
{
   size_t missingLength;
   ssize_t recvRes;
   struct iovec iov;


   // check for incoming data
   if(!__FhgfsOpsCommKit_readfileV2bIsDataAvailable(statesHelper, currentState) )
      return;

   iov = BEEGFS_IOV_ITER_IOVEC(&currentState->data);
   missingLength = MIN(iov.iov_len, currentState->toBeTransmitted - currentState->transmitted);

   // receive available dataPart
   recvRes = currentState->sock->recvT(currentState->sock,
      iov.iov_base, missingLength, 0, CONN_LONG_TIMEOUT);

#if (BEEGFS_COMMKIT_DEBUG & COMMKIT_DEBUG_RECV_DATA)
   if (recvRes > 0 && jiffies % CommKitErrorInjectRate == 0)
   {
      Logger_logTopFormatted(statesHelper->log, LogTopic_COMMKIT, Log_WARNING, __func__,
         "Injecting timeout error after (successfully) receiving data.");
      recvRes = -ETIMEDOUT;
   }
#endif

   if(unlikely(recvRes <= 0) )
   { // something went wrong
      if(recvRes == -EFAULT)
      { // bad buffer address given
         Logger_logFormatted(statesHelper->log, Log_DEBUG, statesHelper->logContext,
            "Bad buffer address");

         currentState->nodeResult = -FhgfsOpsErr_ADDRESSFAULT;
         currentState->stage = ReadfileStage_SOCKETINVALIDATE;
         return;
      }
      else
      if(recvRes == -ETIMEDOUT)
      { // timeout
         Logger_logErrFormatted(statesHelper->log, statesHelper->logContext,
            "Communication timeout in RECVDATA stage. Node: %s",
            Node_getNodeIDWithTypeStr(currentState->node) );
      }
      else
      { // error
         Logger_logErrFormatted(statesHelper->log, statesHelper->logContext,
            "Communication error in RECVDATA stage. Node: %s (recv result: %lld)",
            Node_getNodeIDWithTypeStr(currentState->node), (long long)recvRes);
      }

      Logger_logFormatted(statesHelper->log, Log_SPAM, statesHelper->logContext,
         "Request details: missingLength of %s: %lld",
         Node_getNodeIDWithTypeStr(currentState->node), (long long)missingLength);

      currentState->stage = ReadfileStage_SOCKETEXCEPTION;
      return;
   }

   currentState->transmitted += recvRes;
   iov_iter_advance(&currentState->data, recvRes);

   if(currentState->toBeTransmitted == currentState->transmitted)
   { // all of the data has been received => receive the next lengthInfo in the header stage
      currentState->stage = ReadfileStage_RECVHEADER;
   }

   // prepare polling for the next round (no matter if we received all of the data or not)
   __FhgfsOpsCommKit_readfileAddStateSockToPollSocks(statesHelper, currentState);
}

void __FhgfsOpsCommKit_readfileV2bStageSOCKETEXCEPTION(RWfileStatesHelper* statesHelper,
   ReadfileState* currentState)
{
   const char* logCommInterruptedTxt = "Communication interrupted by signal";
   const char* logCommErrTxt = "Communication error";

   if(Thread_isSignalPending() )
   { // interrupted by signal
      Logger_logFormatted(statesHelper->log, Log_NOTICE, statesHelper->logContext,
         "%s. Node: %s", logCommInterruptedTxt, Node_getNodeIDWithTypeStr(currentState->node) );
   }
   else
   { // "normal" connection error
      Logger_logErrFormatted(statesHelper->log, statesHelper->logContext,
         "%s. Node: %s", logCommErrTxt, Node_getNodeIDWithTypeStr(currentState->node) );

      Logger_logFormatted(statesHelper->log, Log_DEBUG, statesHelper->logContext,
         "Sent request: node: %s; fileHandleID: %s; offset: %lld; size: %lld",
         Node_getNodeIDWithTypeStr(currentState->node), statesHelper->ioInfo->fileHandleID,
         (long long)currentState->offset, (long long)currentState->data.count);
   }

   currentState->nodeResult = -FhgfsOpsErr_COMMUNICATION;

   currentState->stage = ReadfileStage_SOCKETINVALIDATE;
}

void __FhgfsOpsCommKit_readfileV2bStageSOCKETINVALIDATE(RWfileStatesHelper* statesHelper,
   ReadfileState* currentState)
{
   NodeConnPool_invalidateStreamSocket(Node_getConnPool(currentState->node), currentState->sock);
   statesHelper->numAcquiredConns--;

   currentState->stage = ReadfileStage_CLEANUP;
}

void __FhgfsOpsCommKit_readfileV2bStageCLEANUP(RWfileStatesHelper* statesHelper,
   ReadfileState* currentState)
{
   NodeStoreEx* storageNodes = App_getStorageNodes(statesHelper->app);

   // actual clean-up

   if(likely(currentState->node) )
      NodeStoreEx_releaseNode(storageNodes, &currentState->node);

   // prepare next stage

   if(unlikely(currentState->nodeResult == -FhgfsOpsErr_COMMUNICATION) )
   { // comm error occurred => check whether we can do a retry

      if(Thread_isSignalPending() )
      { // interrupted by signal => no retry
         currentState->nodeResult = -FhgfsOpsErr_INTERRUPTED;
      }
      else
      if( App_getConnRetriesEnabled(statesHelper->app) &&
          ( (!statesHelper->maxNumRetries) ||
            (statesHelper->currentRetryNum < statesHelper->maxNumRetries) ) )
      { // we have retries left
         __FhgfsOpsCommKit_readfileV2bPrepareRetry(statesHelper, currentState);

         return;
      }
   }

   // success or no retries left => done

   statesHelper->numDone++;
   currentState->stage = ReadfileStage_DONE;
}

/**
 * Checks for immediately available incoming data.
 *
 * @return fhgfs_true if incoming data is available for this state
 */
fhgfs_bool __FhgfsOpsCommKit_readfileV2bIsDataAvailable(RWfileStatesHelper* statesHelper,
   ReadfileState* currentState)
{
   // check for a poll() timeout or error (all states have to be cancelled in that case)

   if(unlikely(statesHelper->pollTimedOut) )
   {
      //Logger_logFormatted(statesHelper->log, 2, statesHelper->logContext,
      //   "Canceling due to communication error: %s (%s)",
      //   currentState->nodeID, Socket_getPeername(currentState->sock) );

      currentState->nodeResult = -FhgfsOpsErr_COMMUNICATION;
      currentState->stage = ReadfileStage_SOCKETINVALIDATE;
      return fhgfs_false;
   }


   // check for immediately available data

   if(!(statesHelper->pollSocks)[currentState->pollSocksIndex].revents)
   { // no data available yet => wait for the next round
      __FhgfsOpsCommKit_readfileAddStateSockToPollSocks(statesHelper, currentState);

      return fhgfs_false;
   }

   // incoming data is available

   return fhgfs_true;
}

void __FhgfsOpsCommKit_readfileV2bPrepareRetry(RWfileStatesHelper* statesHelper,
   ReadfileState* currentState)
{
   statesHelper->numRetryWaiters++;

   currentState->stage = ReadfileStage_RETRYWAIT;
}

void __FhgfsOpsCommKit_readfileV2bStartRetry(RWfileStatesHelper* statesHelper,
   ReadfileState* states, size_t numStates)
{
   size_t i;
   TargetStateStore* stateStore = App_getTargetStateStore(statesHelper->app);
   MirrorBuddyGroupMapper* mirrorBuddies = App_getMirrorBuddyGroupMapper(statesHelper->app);
   unsigned patternType = StripePattern_getPatternType(statesHelper->ioInfo->pattern);

   fhgfs_bool cancelRetries = fhgfs_false; // true if there are offline targets
   fhgfs_bool resetRetries = fhgfs_false; /* true to not deplete retries if there are
                                             unusable target states ("!good && !offline") */
   fhgfs_bool sleepOnResetRetries = fhgfs_true; // true to retry immediately without sleeping


   // reset statesHelper values for retry round

   statesHelper->numRetryWaiters = 0;
   statesHelper->pollTimedOut = fhgfs_false;

   // check for offline targets

   for(i = 0; i < numStates; i++)
   {
      ReadfileState* currentState = &states[i];

      if(currentState->stage == ReadfileStage_RETRYWAIT)
      {
         CombinedTargetState targetState;
         CombinedTargetState buddyTargetState;
         uint16_t targetID = currentState->targetID;
         uint16_t buddyTargetID = currentState->targetID;
         fhgfs_bool getTargetStateRes;
         fhgfs_bool getBuddyTargetStateRes = fhgfs_true;

         // resolve the actual targetID

         if(patternType == STRIPEPATTERN_BuddyMirror)
         {
            targetID = currentState->useBuddyMirrorSecond ?
               MirrorBuddyGroupMapper_getSecondaryTargetID(mirrorBuddies, currentState->targetID) :
               MirrorBuddyGroupMapper_getPrimaryTargetID(mirrorBuddies, currentState->targetID);
            buddyTargetID = currentState->useBuddyMirrorSecond ?
               MirrorBuddyGroupMapper_getPrimaryTargetID(mirrorBuddies, currentState->targetID) :
               MirrorBuddyGroupMapper_getSecondaryTargetID(mirrorBuddies, currentState->targetID);
         }

         // check current target state

         getTargetStateRes = TargetStateStore_getState(stateStore, targetID,
            &targetState);
         if (targetID == buddyTargetID)
            buddyTargetState = targetState;
         else
            getBuddyTargetStateRes = TargetStateStore_getState(stateStore, buddyTargetID,
               &buddyTargetState);

         if( (!getTargetStateRes && ( (targetID != buddyTargetID) && !getBuddyTargetStateRes) ) ||
            ( (targetState.reachabilityState == TargetReachabilityState_OFFLINE) &&
               (buddyTargetState.reachabilityState == TargetReachabilityState_OFFLINE) ) )
         { // no more retries when both buddies are offline
            LOG_DEBUG_FORMATTED(statesHelper->log, Log_SPAM, statesHelper->logContext,
               "Skipping communication with offline targetID: %hu",
               targetID);
            cancelRetries = fhgfs_true;
            break;
         }

         if( ( !getTargetStateRes ||
               (targetState.consistencyState != TargetConsistencyState_GOOD) ||
               (targetState.reachabilityState != TargetReachabilityState_ONLINE) )
            && ( getBuddyTargetStateRes &&
               (buddyTargetState.consistencyState == TargetConsistencyState_GOOD) &&
               (buddyTargetState.reachabilityState == TargetReachabilityState_ONLINE) ) )
         { // current target not good but buddy is good => switch to buddy
            LOG_DEBUG_FORMATTED(statesHelper->log, Log_SPAM, statesHelper->logContext,
               "Switching to buddy with good target state. "
               "targetID: %hu; target state: %s / %s",
               targetID, TargetStateStore_reachabilityStateToStr(targetState.reachabilityState),
               TargetStateStore_consistencyStateToStr(targetState.consistencyState) );
            currentState->useBuddyMirrorSecond = !currentState->useBuddyMirrorSecond;
            currentState->stage = ReadfileStage_PREPARE;
            resetRetries = fhgfs_true;
            sleepOnResetRetries = fhgfs_false;
            continue;
         }

         if( (patternType == STRIPEPATTERN_BuddyMirror) &&
            ( (targetState.reachabilityState != TargetReachabilityState_ONLINE) ||
            (targetState.consistencyState != TargetConsistencyState_GOOD) ) )
         { // both buddies not good, but at least one of them not offline => wait for clarification
            LOG_DEBUG_FORMATTED(statesHelper->log, Log_DEBUG, statesHelper->logContext,
               "Waiting because of target state. "
               "targetID: %hu; target state: %s / %s",
               targetID, TargetStateStore_reachabilityStateToStr(targetState.reachabilityState),
               TargetStateStore_consistencyStateToStr(targetState.consistencyState) );
            currentState->stage = ReadfileStage_PREPARE;
            resetRetries = fhgfs_true;
            continue;
         }

         // normal retry
         currentState->stage = ReadfileStage_PREPARE;
      }
   }

   // if we have offline targets, cancel all further retry waiters

   if(cancelRetries)
   {
      for(i = 0; i < numStates; i++)
      {
         ReadfileState* currentState = &states[i];

         if( (currentState->stage != ReadfileStage_RETRYWAIT) &&
             (currentState->stage != ReadfileStage_PREPARE ) )
            continue;

         statesHelper->numDone++;
         currentState->stage = ReadfileStage_DONE;
      }

      return;
   }


   // wait before we actually start the retry

   if(resetRetries)
   { // reset retries to not deplete them in case of non-good and non-offline targets
      if(sleepOnResetRetries)
         Thread_sleep(COMMKIT_RESET_SLEEP_MS);

      statesHelper->currentRetryNum = 0;
   }
   else
   { // normal retry
      MessagingTk_waitBeforeRetry(statesHelper->currentRetryNum);

      statesHelper->currentRetryNum++;
   }
}


/**
 * @return fhgfs_false on error (means 'break' for the surrounding switch statement)
 */
fhgfs_bool __FhgfsOpsCommKit_writefileV2bStagePREPARE(RWfileStatesHelper* statesHelper,
   WritefileState* currentState)
{
   Config* cfg = App_getConfig(statesHelper->app);
   TargetMapper* targetMapper = App_getTargetMapper(statesHelper->app);
   NodeStoreEx* storageNodes = App_getStorageNodes(statesHelper->app);
   Node* localNode = App_getLocalNode(statesHelper->app);
   const char* localNodeID = Node_getID(localNode);

   FhgfsOpsErr resolveErr;
   NodeConnPool* connPool;
   WriteLocalFileMsg writeMsg;
   fhgfs_bool allowWaitForConn = !statesHelper->numAcquiredConns; // don't wait if we got at least
      // one conn already (this is important to avoid a deadlock between racing commkit processes)

   uint16_t nodeReferenceTargetID = currentState->targetID; /* to avoid overriding original targetID
      in case of buddy-mirroring, because we need the mirror group ID for the msg and for retries */

   currentState->sock = NULL;
   currentState->nodeResult = -FhgfsOpsErr_COMMUNICATION;

   // select the right targetID and get target state

   if(StripePattern_getPatternType(statesHelper->ioInfo->pattern) == STRIPEPATTERN_BuddyMirror)
   { // given targetID refers to a buddy mirror group
      MirrorBuddyGroupMapper* mirrorBuddies = App_getMirrorBuddyGroupMapper(statesHelper->app);

      nodeReferenceTargetID = currentState->useBuddyMirrorSecond ?
         MirrorBuddyGroupMapper_getSecondaryTargetID(mirrorBuddies, currentState->targetID) :
         MirrorBuddyGroupMapper_getPrimaryTargetID(mirrorBuddies, currentState->targetID);

      if(unlikely(!nodeReferenceTargetID) )
      { // invalid mirror group ID
         Logger_logErrFormatted(statesHelper->log, statesHelper->logContext,
            "Invalid mirror buddy group ID: %hu", currentState->targetID);
         currentState->nodeResult = -FhgfsOpsErr_UNKNOWNTARGET;
         currentState->stage = WritefileStage_CLEANUP;
         return fhgfs_false;
      }
   }

   // check target state

   {
      TargetStateStore* stateStore = App_getTargetStateStore(statesHelper->app);
      CombinedTargetState targetState;
      fhgfs_bool getTargetStateRes = TargetStateStore_getState(stateStore, nodeReferenceTargetID,
         &targetState);

      if(unlikely( !getTargetStateRes ||
          (targetState.reachabilityState == TargetReachabilityState_OFFLINE) ||
          ( (StripePattern_getPatternType(statesHelper->ioInfo->pattern) ==
             STRIPEPATTERN_BuddyMirror) &&
            (targetState.consistencyState != TargetConsistencyState_GOOD) ) ) )
      { // unusable target state, retry details will be handled in _writefileV2bPrepareRetry()
         currentState->stage = WritefileStage_CLEANUP;
         return fhgfs_false;
      }
   }

   // get the target-node reference
   currentState->node = NodeStoreEx_referenceNodeByTargetID(storageNodes,
      nodeReferenceTargetID, targetMapper, &resolveErr);
   if(unlikely(!currentState->node) )
   { // unable to resolve targetID
      currentState->nodeResult = -resolveErr;
      currentState->stage = WritefileStage_CLEANUP;
      return fhgfs_false;
   }

   connPool = Node_getConnPool(currentState->node);

   // connect
   currentState->sock = NodeConnPool_acquireStreamSocketEx(connPool, allowWaitForConn);
   if(!currentState->sock)
   { // no conn available => error or didn't want to wait
      if(likely(!allowWaitForConn) )
      { // just didn't want to wait => keep stage and try again later
         NodeStoreEx_releaseNode(storageNodes, &currentState->node);

         statesHelper->numUnconnectable++;
      }
      else
      { // connection error
         if(!statesHelper->connFailedLogged)
         { // no conn error logged yet
            if(Thread_isSignalPending() )
               Logger_logFormatted(statesHelper->log, Log_DEBUG, statesHelper->logContext,
                  "Connect to server canceled by pending signal: %s",
                  Node_getNodeIDWithTypeStr(currentState->node) );
            else
               Logger_logFormatted(statesHelper->log, Log_WARNING, statesHelper->logContext,
                  "Unable to connect to server: %s",
                  Node_getNodeIDWithTypeStr(currentState->node) );
         }

         statesHelper->connFailedLogged = fhgfs_true;
         currentState->stage = WritefileStage_CLEANUP;
      }

      // in either case return fhgfs_false to avoid falling through to send stage
      return fhgfs_false;
   }

   statesHelper->numAcquiredConns++;

   // prepare message
   WriteLocalFileMsg_initFromSession(&writeMsg, localNodeID,
      statesHelper->ioInfo->fileHandleID, currentState->targetID, statesHelper->ioInfo->pathInfo,
      statesHelper->ioInfo->accessFlags, currentState->offset, currentState->data.count);

   NetMessage_setMsgHeaderTargetID( (NetMessage*)&writeMsg, nodeReferenceTargetID);

   if (Config_getQuotaEnabled(cfg) )
      WriteLocalFileMsg_setUserdataForQuota(&writeMsg, statesHelper->ioInfo->userID,
         statesHelper->ioInfo->groupID);

   if (currentState->firstWriteDoneForTarget)
      NetMessage_addMsgHeaderFeatureFlag( (NetMessage*)&writeMsg,
         WRITELOCALFILEMSG_FLAG_SESSION_CHECK);

   if(StripePattern_getPatternType(statesHelper->ioInfo->pattern) == STRIPEPATTERN_BuddyMirror)
   {
      NetMessage_addMsgHeaderFeatureFlag( (NetMessage*)&writeMsg,
         WRITELOCALFILEMSG_FLAG_BUDDYMIRROR);

      if(currentState->useServersideMirroring)
         NetMessage_addMsgHeaderFeatureFlag( (NetMessage*)&writeMsg,
            WRITELOCALFILEMSG_FLAG_BUDDYMIRROR_FORWARD);

      if(currentState->useBuddyMirrorSecond)
         NetMessage_addMsgHeaderFeatureFlag( (NetMessage*)&writeMsg,
            WRITELOCALFILEMSG_FLAG_BUDDYMIRROR_SECOND);
   }

   if(App_getNetBenchModeEnabled(statesHelper->app) )
      NetMessage_addMsgHeaderFeatureFlag( (NetMessage*)&writeMsg,
         WRITELOCALFILEMSG_FLAG_DISABLE_IO);

   NetMessage_serialize( (NetMessage*)&writeMsg, currentState->msgBuf, BEEGFS_COMMKIT_MSGBUF_SIZE);

   currentState->transmitted = 0;
   currentState->toBeTransmitted = NetMessage_getMsgLength( (NetMessage*)&writeMsg);

   WriteLocalFileMsg_uninit( (NetMessage*)&writeMsg);

   currentState->stage = WritefileStage_SENDHEADER;

   return fhgfs_true;
}

void __FhgfsOpsCommKit_writefileV2bStageSENDHEADER(RWfileStatesHelper* statesHelper,
   WritefileState* currentState)
{
   ssize_t sendRes;

   sendRes = currentState->sock->send(
      currentState->sock, currentState->msgBuf, currentState->toBeTransmitted, 0);

#if (BEEGFS_COMMKIT_DEBUG & COMMKIT_DEBUG_WRITE_HEADER )
      if (sendRes == FhgfsOpsErr_SUCCESS && jiffies % CommKitErrorInjectRate == 0)
      {
         Logger_logTopFormatted(statesHelper->log, LogTopic_COMMKIT, Log_WARNING, __func__,
            "Injecting timeout error after (successfully) sending data.");
         sendRes = -ETIMEDOUT;
      }
#endif

   if(unlikely(sendRes <= 0) )
   {
      Logger_logFormatted(statesHelper->log, Log_WARNING, statesHelper->logContext,
         "Failed to send message to %s: %s", Node_getNodeIDWithTypeStr(currentState->node),
         Socket_getPeername(currentState->sock) );

      currentState->stage = WritefileStage_SOCKETEXCEPTION;
      return;
   }

   // prepare transmission state vars
   currentState->transmitted = 0;
   currentState->toBeTransmitted = currentState->data.count;

   currentState->stage = WritefileStage_SENDDATA;

   // prepare polling for the next stage
   __FhgfsOpsCommKit_writefileAddStateSockToPollSocks(statesHelper, currentState, BEEGFS_POLLOUT);
}

void __FhgfsOpsCommKit_writefileV2bStageSENDDATA(RWfileStatesHelper* statesHelper,
   WritefileState* currentState)
{
   FhgfsOpsErr sendDataPartRes;

#if (BEEGFS_COMMKIT_DEBUG & COMMKIT_DEBUG_WRITE_SEND)
   if (jiffies % CommKitErrorInjectRate == 0)
   {
      Logger_logTopFormatted(statesHelper->log, LogTopic_COMMKIT, Log_WARNING, __func__,
         "Injecting timeout error before sending data.");
      statesHelper->pollTimedOut = fhgfs_true;
   }
#endif

   // check for a poll() timeout or error
   if(unlikely(statesHelper->pollTimedOut) )
   {
      //Logger_logFormatted(statesHelper->log, 2, statesHelper->logContext,
      //   "Canceling due to communication error: %s (%s)",
      //   currentState->nodeID, Socket_getPeername(currentState->sock) );

      currentState->nodeResult = -FhgfsOpsErr_COMMUNICATION;
      currentState->stage = WritefileStage_SOCKETINVALIDATE;
      return;
   }

   // check if we can send data non-blocking
   if(!(statesHelper->pollSocks)[currentState->pollSocksIndex].revents)
   { // not ready for sending yet => wait for the next round
      __FhgfsOpsCommKit_writefileAddStateSockToPollSocks(
         statesHelper, currentState, BEEGFS_POLLOUT);

      return;
   }

   // send dataPart non-blocking
   sendDataPartRes = WriteLocalFileMsg_sendDataPartInstant(statesHelper->app, &currentState->data,
      currentState->toBeTransmitted, &currentState->transmitted, currentState->sock);

#if (BEEGFS_COMMKIT_DEBUG & COMMKIT_DEBUG_WRITE_SEND)
      if (sendDataPartRes == FhgfsOpsErr_SUCCESS && jiffies % CommKitErrorInjectRate == 0)
      {
         Logger_logTopFormatted(statesHelper->log, LogTopic_COMMKIT, Log_WARNING, __func__,
            "Injecting timeout error after (successfully) sending data.");
         sendDataPartRes = -ETIMEDOUT;
      }
#endif

   if(unlikely(sendDataPartRes != FhgfsOpsErr_SUCCESS) )
   {
      if(sendDataPartRes == FhgfsOpsErr_ADDRESSFAULT)
      { // bad buffer address given
         Logger_logFormatted(statesHelper->log, Log_DEBUG, statesHelper->logContext,
            "Bad buffer address");

         currentState->nodeResult = -FhgfsOpsErr_ADDRESSFAULT;
         currentState->stage = WritefileStage_SOCKETINVALIDATE;
         return;
      }

      Logger_logErrFormatted(statesHelper->log, statesHelper->logContext,
         "Communication error in SENDDATA stage. Node: %s",
         Node_getNodeIDWithTypeStr(currentState->node) );
      Logger_logFormatted(statesHelper->log, Log_SPAM, statesHelper->logContext,
         "Request details: transmitted/toBeTransmitted: %lld/%lld",
         (long long)currentState->transmitted, (long long)currentState->toBeTransmitted);

      currentState->stage = WritefileStage_SOCKETEXCEPTION;
      return;
   }

   if(currentState->toBeTransmitted == currentState->transmitted)
   { // all of the data has been sent => proceed to the next stage
      currentState->stage = WritefileStage_RECV;

      __FhgfsOpsCommKit_writefileAddStateSockToPollSocks(
         statesHelper, currentState, BEEGFS_POLLIN);
   }
   else
   { // there is still data to be sent => prepare pollout for the next round
      __FhgfsOpsCommKit_writefileAddStateSockToPollSocks(
         statesHelper, currentState, BEEGFS_POLLOUT);
   }

}

void __FhgfsOpsCommKit_writefileV2bStageRECV(RWfileStatesHelper* statesHelper,
   WritefileState* currentState)
{
   ssize_t respRes;

   // check for incoming data
   if(!__FhgfsOpsCommKit_writefileV2bIsDataAvailable(statesHelper, currentState) )
      return;

   // incoming data available

   respRes = MessagingTk_recvMsgBuf(statesHelper->app, currentState->sock,
      currentState->msgBuf, BEEGFS_COMMKIT_MSGBUF_SIZE);

#if (BEEGFS_COMMKIT_DEBUG & COMMKIT_DEBUG_WRITE_RECV)
      if (respRes == FhgfsOpsErr_SUCCESS && jiffies % CommKitErrorInjectRate == 0)
      {
         Logger_logTopFormatted(statesHelper->log, LogTopic_COMMKIT, Log_WARNING, __func__,
            "Injecting timeout error after (successfully) sending data.");
         respRes = -ETIMEDOUT;
      }
#endif

   if(unlikely(respRes <= 0) )
   { // receive failed
      // note: signal pending log msg will be printed in stage SOCKETEXCEPTION, so no need here
      if(!Thread_isSignalPending() )
         Logger_logFormatted(statesHelper->log, Log_WARNING, statesHelper->logContext,
            "Receive failed from: %s @ %s", Node_getNodeIDWithTypeStr(currentState->node),
            Socket_getPeername(currentState->sock) );

      currentState->stage = WritefileStage_SOCKETEXCEPTION;
      return;
   }
   else
   { // got response => deserialize it
      fhgfs_bool deserRes;

      WriteLocalFileRespMsg_init(&currentState->writeRespMsg);

      deserRes = NetMessageFactory_deserializeFromBuf(statesHelper->app, currentState->msgBuf,
         respRes, (NetMessage*)&currentState->writeRespMsg, NETMSGTYPE_WriteLocalFileResp);

      if(unlikely(!deserRes) )
      { // response invalid
         Logger_logFormatted(statesHelper->log, Log_WARNING, statesHelper->logContext,
            "Received invalid response from %s. Expected type: %d. Disconnecting: %s",
            Node_getNodeIDWithTypeStr(currentState->node), NETMSGTYPE_WriteLocalFileResp,
            Socket_getPeername(currentState->sock) );

         currentState->stage = WritefileStage_SOCKETINVALIDATE;
      }
      else
      { // correct response
         int64_t writeRespValue = WriteLocalFileRespMsg_getValue(&currentState->writeRespMsg);

         if(unlikely(writeRespValue == -FhgfsOpsErr_COMMUNICATION) &&
            !statesHelper->connFailedLogged)
         { // server was unable to communicate with another server
            statesHelper->connFailedLogged = fhgfs_true;
            Logger_logFormatted(statesHelper->log, Log_WARNING, statesHelper->logContext,
               "Server reported indirect communication error: %s. targetID: %hu",
               Node_getNodeIDWithTypeStr(currentState->node), currentState->targetID);
         }

         NodeConnPool_releaseStreamSocket(Node_getConnPool(currentState->node), currentState->sock);
         statesHelper->numAcquiredConns--;

         currentState->nodeResult = writeRespValue;

         currentState->stage = WritefileStage_CLEANUP;
      }

      WriteLocalFileRespMsg_uninit( (NetMessage*)&currentState->writeRespMsg);
   }

}

void __FhgfsOpsCommKit_writefileV2bStageSOCKETEXCEPTION(RWfileStatesHelper* statesHelper,
   WritefileState* currentState)
{
   const char* logCommInterruptedTxt = "Communication interrupted by signal";
   const char* logCommErrTxt = "Communication error";

   if(Thread_isSignalPending() )
   { // interrupted by signal
      Logger_logFormatted(statesHelper->log, Log_NOTICE, statesHelper->logContext,
         "%s. Node: %s", logCommInterruptedTxt, Node_getNodeIDWithTypeStr(currentState->node) );
   }
   else
   { // "normal" connection error
      Logger_logErrFormatted(statesHelper->log, statesHelper->logContext,
         "%s. Node: %s", logCommErrTxt, Node_getNodeIDWithTypeStr(currentState->node) );

      Logger_logFormatted(statesHelper->log, Log_DEBUG, statesHelper->logContext,
         "Sent request: node: %s; fileHandleID: %s; offset: %lld; size: %lld",
         Node_getNodeIDWithTypeStr(currentState->node), statesHelper->ioInfo->fileHandleID,
         (long long)currentState->offset, (long long)currentState->toBeTransmitted);
   }

   currentState->nodeResult = -FhgfsOpsErr_COMMUNICATION;

   currentState->stage = WritefileStage_SOCKETINVALIDATE;
}

void __FhgfsOpsCommKit_writefileV2bStageSOCKETINVALIDATE(RWfileStatesHelper* statesHelper,
   WritefileState* currentState)
{
   NodeConnPool_invalidateStreamSocket(Node_getConnPool(currentState->node), currentState->sock);
   statesHelper->numAcquiredConns--;

   currentState->stage = WritefileStage_CLEANUP;
}

void __FhgfsOpsCommKit_writefileV2bStageCLEANUP(RWfileStatesHelper* statesHelper,
   WritefileState* currentState)
{
   NodeStoreEx* storageNodes = App_getStorageNodes(statesHelper->app);

   // actual clean-up

   if(likely(currentState->node) )
      NodeStoreEx_releaseNode(storageNodes, &currentState->node);

   // prepare next stage

   if(unlikely(
      (currentState->nodeResult == -FhgfsOpsErr_COMMUNICATION) ||
      (currentState->nodeResult == -FhgfsOpsErr_AGAIN) ) )
   { // comm error occurred => check whether we can do a retry

      if(Thread_isSignalPending() )
      { // interrupted by signal => no retry
         currentState->nodeResult = -FhgfsOpsErr_INTERRUPTED;
      }
      else
      if( App_getConnRetriesEnabled(statesHelper->app) &&
          ( (!statesHelper->maxNumRetries) ||
            (statesHelper->currentRetryNum < statesHelper->maxNumRetries) ) )
      { // we have retries left
         __FhgfsOpsCommKit_writefileV2bPrepareRetry(statesHelper, currentState);

         return;
      }
   }

   // success or no retries left => done
   statesHelper->numDone++;
   currentState->stage = WritefileStage_DONE;
}

/**
 * Checks for immediately available incoming data.
 *
 * @return fhgfs_true if incoming data is available for this state
 */
fhgfs_bool __FhgfsOpsCommKit_writefileV2bIsDataAvailable(RWfileStatesHelper* statesHelper,
   WritefileState* currentState)
{
   // check for a poll() timeout or error (all states have to be cancelled in that case)

   if(unlikely(statesHelper->pollTimedOut) )
   {
      //Logger_logFormatted(statesHelper->log, 2, statesHelper->logContext,
      //   "Canceling due to communication error: %s (%s)",
      //   currentState->nodeID, Socket_getPeername(currentState->sock) );

      currentState->nodeResult = -FhgfsOpsErr_COMMUNICATION;
      currentState->stage = WritefileStage_SOCKETINVALIDATE;
      return fhgfs_false;
   }


   // check for immediately available data

   if(!(statesHelper->pollSocks)[currentState->pollSocksIndex].revents)
   { // no data available yet => wait for the next round
      __FhgfsOpsCommKit_writefileAddStateSockToPollSocks(
         statesHelper, currentState, BEEGFS_POLLIN);

      return fhgfs_false;
   }

   // incoming data is available

   return fhgfs_true;
}

void __FhgfsOpsCommKit_writefileV2bPrepareRetry(RWfileStatesHelper* statesHelper,
   WritefileState* currentState)
{
   statesHelper->numRetryWaiters++;

   currentState->stage = WritefileStage_RETRYWAIT;
}

void __FhgfsOpsCommKit_writefileV2bStartRetry(RWfileStatesHelper* statesHelper,
   WritefileState* states, size_t numStates)
{
   size_t i;
   TargetStateStore* stateStore = App_getTargetStateStore(statesHelper->app);
   MirrorBuddyGroupMapper* mirrorBuddies = App_getMirrorBuddyGroupMapper(statesHelper->app);
   unsigned patternType = StripePattern_getPatternType(statesHelper->ioInfo->pattern);

   fhgfs_bool cancelRetries = fhgfs_false; // true if there are offline targets
   fhgfs_bool resetRetries = fhgfs_false; /* true to not deplete retries if there are
                                             unusable target states ("!good && !offline") */


   // reset statesHelper values for retry round

   statesHelper->numRetryWaiters = 0;
   statesHelper->pollTimedOut = fhgfs_false;

   // check for offline targets

   for(i = 0; i < numStates; i++)
   {
      WritefileState* currentState = &states[i];

      if(currentState->stage == WritefileStage_RETRYWAIT)
      {
         CombinedTargetState targetState;
         CombinedTargetState buddyTargetState;
         uint16_t targetID = currentState->targetID;
         uint16_t buddyTargetID = currentState->targetID;
         fhgfs_bool getTargetStateRes;
         fhgfs_bool getBuddyTargetStateRes = fhgfs_true;

         // resolve the actual targetID

         if(patternType == STRIPEPATTERN_BuddyMirror)
         {
            targetID = currentState->useBuddyMirrorSecond ?
               MirrorBuddyGroupMapper_getSecondaryTargetID(mirrorBuddies, currentState->targetID) :
               MirrorBuddyGroupMapper_getPrimaryTargetID(mirrorBuddies, currentState->targetID);
            buddyTargetID = currentState->useBuddyMirrorSecond ?
               MirrorBuddyGroupMapper_getPrimaryTargetID(mirrorBuddies, currentState->targetID) :
               MirrorBuddyGroupMapper_getSecondaryTargetID(mirrorBuddies, currentState->targetID);
         }

         // check current target state

         getTargetStateRes = TargetStateStore_getState(stateStore, targetID,
            &targetState);
         if (targetID == buddyTargetID)
            buddyTargetState = targetState;
         else
            getBuddyTargetStateRes = TargetStateStore_getState(stateStore, buddyTargetID,
               &buddyTargetState);

         if( (!getTargetStateRes && ( (targetID != buddyTargetID) && !getBuddyTargetStateRes) ) ||
               ( (targetState.reachabilityState == TargetReachabilityState_OFFLINE) &&
               (buddyTargetState.reachabilityState == TargetReachabilityState_OFFLINE) ) )
         { // no more retries when both buddies are offline
            LOG_DEBUG_FORMATTED(statesHelper->log, Log_SPAM, statesHelper->logContext,
               "Skipping communication with offline targetID: %hu",
               targetID);
            cancelRetries = fhgfs_true;
            break;
         }

         if( (patternType == STRIPEPATTERN_BuddyMirror) &&
            ( (targetState.reachabilityState != TargetReachabilityState_ONLINE) ||
            (targetState.consistencyState != TargetConsistencyState_GOOD) ) )
         { // both buddies not good, but at least one of them not offline => wait for clarification
            LOG_DEBUG_FORMATTED(statesHelper->log, Log_DEBUG, statesHelper->logContext,
               "Waiting because of target state. "
               "targetID: %hu; target state: %s / %s",
               targetID, TargetStateStore_reachabilityStateToStr(targetState.reachabilityState),
               TargetStateStore_consistencyStateToStr(targetState.consistencyState) );
            currentState->stage = WritefileStage_PREPARE;
            resetRetries = fhgfs_true;
            continue;
         }

         if(currentState->nodeResult == -FhgfsOpsErr_AGAIN)
         {
            Logger_logFormatted(statesHelper->log, Log_DEBUG, statesHelper->logContext,
               "Waiting because target asked for infinite retries. "
               "targetID: %hu",
               targetID);
            currentState->stage = WritefileStage_PREPARE;
            resetRetries = fhgfs_true;
            continue;
         }

         // normal retry
         currentState->stage = WritefileStage_PREPARE;
      }
   }

   // if we have offline targets, cancel all further retry waiters

   if(cancelRetries)
   {
      for(i = 0; i < numStates; i++)
      {
         WritefileState* currentState = &states[i];

         if( (currentState->stage != WritefileStage_RETRYWAIT) &&
             (currentState->stage != WritefileStage_PREPARE ) )
            continue;

         statesHelper->numDone++;
         currentState->stage = WritefileStage_DONE;
      }

      return;
   }


   // wait before we actually start the retry

   if(resetRetries)
   { // reset retries to not deplete them in case of non-good and non-offline targets
      Thread_sleep(COMMKIT_RESET_SLEEP_MS);

      statesHelper->currentRetryNum = 0;
   }
   else
   { // normal retry
      MessagingTk_waitBeforeRetry(statesHelper->currentRetryNum);

      statesHelper->currentRetryNum++;
   }
}


/**
 * Note: This method cannot fail (no return value), but the performed communication etc. can fail,
 * of course. Hence, read the results from the states.
 *
 * @param pollSocks array length must match numStates
 */
void FhgfsOpsCommKit_readfileV2bCommunicate(App* app, RemotingIOInfo* ioInfo,
   ReadfileState* states, size_t numStates, PollSocketEx* pollSocks)
{
   Config* cfg = App_getConfig(app);

   RWfileStatesHelper statesHelper =
   {
      .app = app,
      .log = App_getLogger(app),
      .logContext = "readfileV2 (communication)",

      .ioInfo = ioInfo,

      .numRetryWaiters = 0, // counter for states that encountered a comm error
      .numDone = 0, // counter for finished states
      .numAcquiredConns = 0, // counter for currently acquired conns (to wait only for first conn)

      .pollTimedOut = fhgfs_false,
      .pollTimeoutLogged = fhgfs_false,
      .connFailedLogged = fhgfs_false,

      .pollSocks = pollSocks,

      .currentRetryNum = 0,
      .maxNumRetries = Config_getConnNumCommRetries(cfg),
   };


   size_t i;

   // loop until all states increased the done-counter
   do
   {
      statesHelper.numUnconnectable = 0; // will be increased by states the didn't get a conn
      statesHelper.numPollSocks = 0; // will be increased by the states that need to poll

      // let each state do something (before we call poll)
      for(i = 0; i < numStates; i++)
      {
         ReadfileState* currentState = &states[i];

         switch(currentState->stage)
         {
            case ReadfileStage_PREPARE:
            {
               if(!__FhgfsOpsCommKit_readfileV2bStagePREPARE(&statesHelper, currentState) )
                  break;
            } /* fall through */

            case ReadfileStage_SEND:
            {
               __FhgfsOpsCommKit_readfileV2bStageSEND(&statesHelper, currentState);
            } break;

            case ReadfileStage_RECVHEADER:
            {
               __FhgfsOpsCommKit_readfileV2bStageRECVHEADER(&statesHelper, currentState);

               // check if we reached end of transmission
               if(currentState->stage != ReadfileStage_RECVDATA)
                  break;
            } /* fall through */

            case ReadfileStage_RECVDATA:
            {
               __FhgfsOpsCommKit_readfileV2bStageRECVDATA(&statesHelper, currentState);
            } break;

            case ReadfileStage_SOCKETEXCEPTION:
            {
               __FhgfsOpsCommKit_readfileV2bStageSOCKETEXCEPTION(&statesHelper, currentState);
            } /* fall through */

            case ReadfileStage_SOCKETINVALIDATE:
            {
               __FhgfsOpsCommKit_readfileV2bStageSOCKETINVALIDATE(&statesHelper, currentState);
            } /* fall through */

            case ReadfileStage_CLEANUP:
            {
               __FhgfsOpsCommKit_readfileV2bStageCLEANUP(&statesHelper, currentState);
            } break;

            case ReadfileStage_RETRYWAIT:
            {
               // nothing to be done here
            } break;

            case ReadfileStage_DONE:
            {
               // nothing to be done here
            } break;

            default:
            { // this should actually never be called
               // nothing to be done here
            } break;

         } // end of switch

      } // end of numStates loop


      if(statesHelper.numPollSocks)
         __FhgfsOpsCommKitCommon_pollStateSocks(&statesHelper, numStates);
      else
      if(unlikely(
         statesHelper.numRetryWaiters &&
         ( (statesHelper.numDone + statesHelper.numRetryWaiters) == numStates) ) )
      { // we have some retry waiters and the rest is done
         // note: this can only happen if we don't have any pollsocks, hence the "else"

         __FhgfsOpsCommKit_readfileV2bStartRetry(&statesHelper, states, numStates);
      }


   } while(statesHelper.numDone != numStates);

}


/**
 * Note: This method cannot fail (no return value), but the performed communication etc. can fail,
 * of course. Hence, read the results from the states.
 *
 * @param pollSocks array length must match numStates
 */
void FhgfsOpsCommKit_writefileV2bCommunicate(App* app, RemotingIOInfo* ioInfo,
   WritefileState* states, size_t numStates, PollSocketEx* pollSocks)
{
   Config* cfg = App_getConfig(app);

   RWfileStatesHelper statesHelper =
   {
      .app = app,
      .log = App_getLogger(app),
      .logContext = "writefile (communication)",

      .ioInfo = ioInfo,

      .numRetryWaiters = 0, // counter for states that encountered a comm error
      .numDone = 0, // counter for finished states
      .numAcquiredConns = 0, // counter for currently acquired conns (to wait only for first conn)

      .pollTimedOut = fhgfs_false,
      .pollTimeoutLogged = fhgfs_false,
      .connFailedLogged = fhgfs_false,

      .pollSocks = pollSocks,

      .currentRetryNum = 0,
      .maxNumRetries = Config_getConnNumCommRetries(cfg),
   };


   size_t i;

   // loop until all states increased the done-counter
   do
   {
      statesHelper.numUnconnectable = 0; // will be increased by states the didn't get a conn
      statesHelper.numPollSocks = 0; // will be increased by the states that need to poll

      // let each state do something (before we call poll)
      for(i = 0; i < numStates; i++)
      {
         WritefileState* currentState = &states[i];

         switch(currentState->stage)
         {
            case WritefileStage_PREPARE:
            {
               if(!__FhgfsOpsCommKit_writefileV2bStagePREPARE(&statesHelper, currentState) )
                  break;
            } /* fall through */

            case WritefileStage_SENDHEADER:
            {
               __FhgfsOpsCommKit_writefileV2bStageSENDHEADER(&statesHelper, currentState);
            } break;

            case WritefileStage_SENDDATA:
            {
               __FhgfsOpsCommKit_writefileV2bStageSENDDATA(&statesHelper, currentState);
            } break;

            case WritefileStage_RECV:
            {
               __FhgfsOpsCommKit_writefileV2bStageRECV(&statesHelper, currentState);
            } break;

            case WritefileStage_SOCKETEXCEPTION:
            {
               __FhgfsOpsCommKit_writefileV2bStageSOCKETEXCEPTION(&statesHelper, currentState);
            } /* fall through */

            case WritefileStage_SOCKETINVALIDATE:
            {
               __FhgfsOpsCommKit_writefileV2bStageSOCKETINVALIDATE(&statesHelper, currentState);
            } /* fall through */

            case WritefileStage_CLEANUP:
            {
               __FhgfsOpsCommKit_writefileV2bStageCLEANUP(&statesHelper, currentState);
            } break;

            case WritefileStage_RETRYWAIT:
            {
               // nothing to be done here
            } break;

            case WritefileStage_DONE:
            {
               // nothing to be done here
            } break;

            default:
            { // this should actually never be called
               // nothing to be done here
            } break;

         } // end of switch

      } // end of numStates loop


      if(statesHelper.numPollSocks)
         __FhgfsOpsCommKitCommon_pollStateSocks(&statesHelper, numStates);
      else
      if(unlikely(
         statesHelper.numRetryWaiters &&
         ( (statesHelper.numDone + statesHelper.numRetryWaiters) == numStates) ) )
      { // we have some retry waiters and the rest is done
         // note: this can only happen if we don't have any pollsocks, hence the "else"

         __FhgfsOpsCommKit_writefileV2bStartRetry(&statesHelper, states, numStates);
      }


   } while(statesHelper.numDone != numStates);

}


