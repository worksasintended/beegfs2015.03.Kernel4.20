#include <app/App.h>
#include <common/net/message/control/GenericResponseMsg.h>
#include <common/nodes/MirrorBuddyGroupMapper.h>
#include <common/nodes/Node.h>
#include <common/nodes/NodeConnPool.h>
#include <common/nodes/Node.h>
#include <common/nodes/TargetStateStore.h>
#include <nodes/NodeStoreEx.h>
#include <toolkit/NoAllocBufferStore.h>
#include <toolkit/SignalTk.h>
#include "MessagingTk.h"


#define MSGTK_KMALLOC_RECV_BUF_LEN    (4*1024) /* kmalloc-style recv is only ok for small replies */
#define MSGTK_STATE_SLEEP_MS          5000 /* how long to sleep if target state not good/offline */
#define MSGTK_INFINITE_RETRY_WAIT_MS  5000 // how long to wait if peer asks for retry

/**
 * Note: rrArgs->outRespBuf must be returned/freed by the caller (depending on respBufType)
 */
FhgfsOpsErr MessagingTk_requestResponseWithRRArgs(App* app,
   RequestResponseArgs* rrArgs)
{
   Logger* log = App_getLogger(app);
   const char* logContext = "Messaging (RPC)";

   fhgfs_bool infiniteRetries = rrArgs->numRetries ? fhgfs_false : fhgfs_true;

   unsigned currentRetryNum = 0;
   FhgfsOpsErr commRes;

   for( ; ; ) // retry loop
   {
      App_incNumRPCs(app);

      commRes = __MessagingTk_requestResponseWithRRArgsComm(app, rrArgs);
      if(likely(commRes == FhgfsOpsErr_SUCCESS) )
         return FhgfsOpsErr_SUCCESS;
      else
      if(Thread_isSignalPending() )
      { // no retry allowed in this situation
         return FhgfsOpsErr_INTERRUPTED;
      }
      else
      if(!Node_getIsActive(rrArgs->node) )
      { // no retry allowed in this situation
         return FhgfsOpsErr_UNKNOWNNODE;
      }
      else
      if(commRes == FhgfsOpsErr_WOULDBLOCK)
         return FhgfsOpsErr_COMMUNICATION; // no retries in this case
      else
      if( (commRes == FhgfsOpsErr_AGAIN) && App_getConnRetriesEnabled(app) )
      { // retry infinitely
         currentRetryNum = 0;

         Thread_sleep(MSGTK_INFINITE_RETRY_WAIT_MS); // sleep interruptible

         continue;
      }
      else
      if(commRes != FhgfsOpsErr_COMMUNICATION)
      { // no retry allowed in this situation
         return commRes;
      }

      if(App_getConnRetriesEnabled(app) &&
         (infiniteRetries || (currentRetryNum < rrArgs->numRetries) ) )
      { // we have a retry left
         MessagingTk_waitBeforeRetry(currentRetryNum);
         currentRetryNum++;

         if(currentRetryNum == 1 // log retry message only on first retry (to not spam the log)
            && !(rrArgs->logFlags & REQUESTRESPONSEARGS_LOGFLAG_RETRY) )
         {
            Logger_logFormatted(log, Log_NOTICE, logContext,
               "Retrying communication with node: %s", Node_getNodeIDWithTypeStr(rrArgs->node) );
            Logger_logFormatted(log, Log_DEBUG, logContext,
               "Message type: %hu", NetMessage_getMsgType(rrArgs->requestMsg) );
         }
      }
      else
      { // no more retries left
         return FhgfsOpsErr_COMMUNICATION;
      }

   }
}

/**
 * Note: You probably rather want to call the alternative method, which gets buffers from the
 * store (unless you are the logger and want to avoid a deadlock wrt depleted msg buffers from the
 * store).
 * Note: Allows only a single retry. (One retry allowed because we might have gotten an already
 * broken connection from the conn pool.)
 *
 * @param outRespBuf will be kmalloced and needs to be kfreed by the caller
 */
FhgfsOpsErr MessagingTk_requestResponseKMalloc(App* app, Node* node, NetMessage* requestMsg,
   unsigned respMsgType, char** outRespBuf, NetMessage** outRespMsg)
{
   RequestResponseArgs rrArgs;
   FhgfsOpsErr rrRes;

   RequestResponseArgs_prepare(&rrArgs, node, requestMsg, respMsgType);

   rrArgs.respBufType = MessagingTkBufType_kmalloc;

   rrRes = MessagingTk_requestResponseWithRRArgs(app, &rrArgs);

   *outRespBuf = rrArgs.outRespBuf;
   *outRespMsg = rrArgs.outRespMsg;

   return rrRes;
}


/**
 * Note: Allows only a single retry. (One retry allowed because we might have gotten an already
 * broken connection from the conn pool.)
 *
 * @param outRespBuf must be returned to the store - not freed!
 */
FhgfsOpsErr MessagingTk_requestResponse(App* app, Node* node, NetMessage* requestMsg,
   unsigned respMsgType, char** outRespBuf, NetMessage** outRespMsg)
{
   RequestResponseArgs rrArgs;
   FhgfsOpsErr rrRes;

   RequestResponseArgs_prepare(&rrArgs, node, requestMsg, respMsgType);

   rrRes = MessagingTk_requestResponseWithRRArgs(app, &rrArgs);

   *outRespBuf = rrArgs.outRespBuf;
   *outRespMsg = rrArgs.outRespMsg;

   return rrRes;
}

/**
 * Note: Uses the number of retries that has been defined in the app config.
 *
 * @param outRespBuf must be returned to the store - not freed!
 */
FhgfsOpsErr MessagingTk_requestResponseRetry(App* app, Node* node, NetMessage* requestMsg,
   unsigned respMsgType, char** outRespBuf, NetMessage** outRespMsg)
{
   Config* cfg = App_getConfig(app);

   RequestResponseArgs rrArgs;
   FhgfsOpsErr rrRes;

   RequestResponseArgs_prepare(&rrArgs, node, requestMsg, respMsgType);

   rrArgs.numRetries = Config_getConnNumCommRetries(cfg);

   rrRes = MessagingTk_requestResponseWithRRArgs(app, &rrArgs);

   *outRespBuf = rrArgs.outRespBuf;
   *outRespMsg = rrArgs.outRespMsg;

   return rrRes;
}



/**
 * This version will automatically blocks signals including interrupt and reenables interrupt if the
 * response doesn't arrive within a few seconds.
 *
 * This is intended to help avoid cases where we miss a reply due to the user pressing ctrl+c (under
 * the assumption that the user won't care to wait a fews seconds longer for interruption).
 *
 * Note: Uses the number of retries that has been defined in the app config.
 * Note: Blocked signals will be restored to original values before this method returns.
 *
 * @param outRespBuf must be returned to the store - not freed!
 */
FhgfsOpsErr MessagingTk_requestResponseRetryAutoIntr(App* app, Node* node, NetMessage* requestMsg,
   unsigned respMsgType, char** outRespBuf, NetMessage** outRespMsg)
{
   Config* cfg = App_getConfig(app);

   sigset_t oldSignalSet;

   RequestResponseArgs rrArgs;
   FhgfsOpsErr rrRes;

   // keep thread interruptible for connect (will later be made uninterruptible for send/recv)
   SignalTk_blockSignals(fhgfs_true, &oldSignalSet); // B L O C K _ S I G s

   RequestResponseArgs_prepare(&rrArgs, node, requestMsg, respMsgType);

   rrArgs.numRetries = Config_getConnNumCommRetries(cfg);
   rrArgs.rrFlags = REQUESTRESPONSEARGS_FLAG_USEDELAYEDINTERRUPT;

   rrRes = MessagingTk_requestResponseWithRRArgs(app, &rrArgs);

   *outRespBuf = rrArgs.outRespBuf;
   *outRespMsg = rrArgs.outRespMsg;

   SignalTk_restoreSignals(&oldSignalSet); // U N B L O C K _ S I G s

   return rrRes;
}

/**
 * Sends a message to a node and receives a response.
 * Can handle target states and mapped mirror IDs. Node does not need to be referenced by caller.
 *
 * If target states are provided, communication might be skipped for certain states.
 *
 * This version allows only a single retry. (One retry allowed because we might have gotten an
 * already broken connection from the conn pool.)
 *
 * note: uses the number of retries that has been defined in the app config.
 * note: blocked signals will be restored to original values before this method returns.
 *
 * @param rrArgs outRespBuf must be returned to the store - not freed; rrArgs->nodeID may optionally
 *    be provided when calling this.
 * @return received message and buffer are available through rrArgs in case of success.
 */
FhgfsOpsErr MessagingTk_requestResponseNode(App* app,
   RequestResponseNode* rrNode, RequestResponseArgs* rrArgs)
{
   FhgfsOpsErr rrRes;

   rrArgs->numRetries = 1;
   rrArgs->rrFlags = 0;
   rrArgs->respBufType = MessagingTkBufType_BufStore;

   rrRes = __MessagingTk_requestResponseNodeRetry(app, rrNode, rrArgs);

   return rrRes;
}

/**
 * Sends a message to a node and receives a response.
 * Can handle target states and mapped mirror IDs. Node does not need to be referenced by caller.
 *
 * If target states are provided, communication might be skipped for certain states.
 *
 * This version will automatically block signals including interrupt and reenables interrupt if the
 * response doesn't arrive within a few seconds. This is intended to help avoid cases where we miss
 * a reply due to the user pressing ctrl+c (under the assumption that the user won't care to wait a
 * fews seconds longer for interruption).
 *
 * note: uses the number of retries that has been defined in the app config.
 * note: blocked signals will be restored to original values before this method returns.
 *
 * @param rrArgs outRespBuf must be returned to the store - not freed; rrArgs->nodeID may optionally
 *    be provided when calling this.
 * @return received message and buffer are available through rrArgs in case of success.
 */
FhgfsOpsErr MessagingTk_requestResponseNodeRetryAutoIntr(App* app, RequestResponseNode* rrNode,
   RequestResponseArgs* rrArgs)
{
   Config* cfg = App_getConfig(app);

   sigset_t oldSignalSet;

   FhgfsOpsErr rrRes;

   rrArgs->numRetries = Config_getConnNumCommRetries(cfg);
   rrArgs->rrFlags = REQUESTRESPONSEARGS_FLAG_USEDELAYEDINTERRUPT |
      REQUESTRESPONSEARGS_FLAG_ALLOWSTATESLEEP;
   rrArgs->respBufType = MessagingTkBufType_BufStore;

   // keep thread interruptible for connect (will later be made uninterruptible for send/recv)
   SignalTk_blockSignals(fhgfs_true, &oldSignalSet); // B L O C K _ S I G s

   rrRes = __MessagingTk_requestResponseNodeRetry(app, rrNode, rrArgs);

   SignalTk_restoreSignals(&oldSignalSet); // U N B L O C K _ S I G s

   return rrRes;
}


/**
 * Sends a message to a node and receives a response.
 * Can handle target states and mapped mirror IDs. Node does not need to be referenced by caller.
 *
 * If target states are provided, communication might be skipped for certain states.
 *
 * @param rrArgs rrArgs->nodeID may optionally be provided when calling this.
 * @return received message and buffer are available through rrArgs in case of success.
 */
FhgfsOpsErr __MessagingTk_requestResponseNodeRetry(App* app, RequestResponseNode* rrNode,
   RequestResponseArgs* rrArgs)
{
   const char* logContext = "Messaging (RPC node)";

   unsigned currentRetryNum = 0; // used number of retries so far
   FhgfsOpsErr commRes;

   for( ; ; ) // retry loop
   {
      fhgfs_bool nodeNeedsRelease = fhgfs_false;

      // select the right targetID

      uint16_t nodeID = rrNode->nodeID; // don't modify caller's nodeID

      if(rrNode->mirrorBuddies)
      { // given targetID refers to a buddy mirror group

         nodeID = rrNode->useBuddyMirrorSecond ?
            MirrorBuddyGroupMapper_getSecondaryTargetID(rrNode->mirrorBuddies, nodeID) :
            MirrorBuddyGroupMapper_getPrimaryTargetID(rrNode->mirrorBuddies, nodeID);

         if(unlikely(!nodeID) )
         {
            Logger* log = App_getLogger(app);
            Logger_logErrFormatted(log, logContext, "Invalid node mirror buddy group ID: %hu",
               rrNode->nodeID);

            return FhgfsOpsErr_UNKNOWNNODE;
         }
      }

      // check target state

      if(rrNode->targetStates)
      {
         CombinedTargetState state;
         fhgfs_bool getStateRes = TargetStateStore_getState(rrNode->targetStates, nodeID,
            &state);

         rrNode->outTargetReachabilityState = state.reachabilityState;
         rrNode->outTargetConsistencyState = state.consistencyState;

         if(unlikely( !getStateRes ||
            (state.reachabilityState != TargetReachabilityState_ONLINE) ||
            (state.consistencyState != TargetConsistencyState_GOOD) ) )
         {
            if(state.reachabilityState == TargetReachabilityState_OFFLINE)
            { // no need to wait for offline servers
               LOG_DEBUG_FORMATTED(App_getLogger(app), Log_SPAM, logContext,
                  "Skipping communication with offline nodeID: %hu", nodeID);

               return FhgfsOpsErr_COMMUNICATION;
            }

            if(!(rrArgs->rrFlags & REQUESTRESPONSEARGS_FLAG_ALLOWSTATESLEEP) )
            { // caller did not allow sleeping if target state is not {good, offline}
               LOG_DEBUG_FORMATTED(App_getLogger(app), Log_SPAM, logContext,
                  "Skipping communication with nodeID: %hu; "
                  "target state: %s / %s",
                  nodeID, TargetStateStore_reachabilityStateToStr(state.reachabilityState),
                  TargetStateStore_consistencyStateToStr(state.consistencyState) );
               return FhgfsOpsErr_COMMUNICATION;
            }

            // sleep on states other than "good" and "offline" with mirroring
            if(rrNode->mirrorBuddies)
            {
               LOG_DEBUG_FORMATTED(App_getLogger(app), Log_DEBUG, logContext,
                  "Waiting before communication because of node state. "
                  "nodeID: %hu; node state: %s / %s",
                  nodeID, TargetStateStore_reachabilityStateToStr(state.reachabilityState),
                  TargetStateStore_consistencyStateToStr(state.consistencyState) );

               Thread_sleep(MSGTK_STATE_SLEEP_MS);
               if(Thread_isSignalPending() )
               { // make sure we don't loop endless if signal pending
                  LOG_DEBUG_FORMATTED(App_getLogger(app), Log_DEBUG, logContext,
                     "Waiting before communication was interrupted by signal. "
                     "nodeID: %hu; node state: %s / %s",
                     nodeID, TargetStateStore_reachabilityStateToStr(state.reachabilityState),
                     TargetStateStore_consistencyStateToStr(state.consistencyState) );
                  return FhgfsOpsErr_INTERRUPTED;
               }

               currentRetryNum = 0; // reset retries in case of unusable target state
               continue;
            }
         }
      }

      // reference node (if not provided by caller already)

      if(!rrArgs->node)
      {
         rrArgs->node = NodeStoreEx_referenceNode(rrNode->nodeStore, nodeID);

         if(!rrArgs->node)
         {
            Logger* log = App_getLogger(app);
            NodeType storeType = NodeStoreEx_getStoreType(rrNode->nodeStore);
            Logger_logFormatted(log, Log_WARNING, logContext, "Unknown nodeID: %hu; type: %s",
               nodeID, Node_nodeTypeToStr(storeType) );

            return FhgfsOpsErr_UNKNOWNNODE;
         }

         nodeNeedsRelease = fhgfs_true;
      }
      else
         BEEGFS_BUG_ON_DEBUG(Node_getNumID(rrArgs->node) != nodeID,
            "Mismatch between given rrArgs->node ID and nodeID");

      // communicate

      commRes = __MessagingTk_requestResponseWithRRArgsComm(app, rrArgs);

      if(likely(commRes == FhgfsOpsErr_SUCCESS) )
         goto release_node_and_break;
      else
      if(Thread_isSignalPending() )
      { // no retry allowed in this situation
         commRes = FhgfsOpsErr_INTERRUPTED;
         goto release_node_and_break;
      }
      else
      if(!Node_getIsActive(rrArgs->node) )
      { // no retry allowed in this situation
         commRes = FhgfsOpsErr_UNKNOWNNODE;
         goto release_node_and_break;
      }
      else
      if(commRes == FhgfsOpsErr_WOULDBLOCK)
      { // no retries in this case
         commRes = FhgfsOpsErr_COMMUNICATION;
         goto release_node_and_break;
      }
      else
      if( (commRes == FhgfsOpsErr_AGAIN) && App_getConnRetriesEnabled(app) )
      { // retry infinitely
         currentRetryNum = 0;

         Thread_sleep(MSGTK_INFINITE_RETRY_WAIT_MS); // sleep interruptible

         goto release_node_and_continue;
      }
      else
      if(commRes != FhgfsOpsErr_COMMUNICATION)
      { // no retry allowed in this situation
         goto release_node_and_break;
      }

      if(App_getConnRetriesEnabled(app) &&
         (!rrArgs->numRetries || (currentRetryNum < rrArgs->numRetries) ) )
      { // we have a retry left
         MessagingTk_waitBeforeRetry(currentRetryNum);
         currentRetryNum++;

         if(currentRetryNum == 1 // log retry message only on first retry (to not spam the log)
            && !(rrArgs->logFlags & REQUESTRESPONSEARGS_LOGFLAG_RETRY) )
         {
            Logger* log = App_getLogger(app);
            Logger_logFormatted(log, Log_NOTICE, logContext,
               "Retrying communication with node: %s", Node_getNodeIDWithTypeStr(rrArgs->node) );
            Logger_logFormatted(log, Log_DEBUG, logContext,
               "Message type: %hu", NetMessage_getMsgType(rrArgs->requestMsg) );
         }
      }
      else
      { // no more retries left
         commRes = FhgfsOpsErr_COMMUNICATION;
         goto release_node_and_break;
      }

   release_node_and_continue:
      if(nodeNeedsRelease)
         NodeStoreEx_releaseNode(rrNode->nodeStore, &rrArgs->node);

      continue;

      // cleanup before early loop exit
   release_node_and_break:
      if(nodeNeedsRelease)
         NodeStoreEx_releaseNode(rrNode->nodeStore, &rrArgs->node);

      break;
   }

   return commRes;
}

/**
 * Sends a message to the owner of a target and receives a response.
 * Can handle target states and mapped mirror IDs. Owner node of target will be resolved and
 * referenced internally.
 *
 * If target states are provided, communication might be skipped for certain states.
 *
 * This version will automatically block signals including interrupt and reenables interrupt if the
 * response doesn't arrive within a few seconds. This is intended to help avoid cases where we miss
 * a reply due to the user pressing ctrl+c (under the assumption that the user won't care to wait a
 * fews seconds longer for interruption).
 *
 * note: uses the number of retries that has been defined in the app config.
 * note: blocked signals will be restored to original values before this method returns.
 *
 * @param rrArgs rrArgs->outRespBuf must be returned to the store - not freed; rrArgs->node must be
 *    NULL when calling this.
 * @return received message and buffer are available through rrArgs in case of success.
 */
FhgfsOpsErr MessagingTk_requestResponseTargetRetryAutoIntr(App* app, RequestResponseTarget* rrTarget,
   RequestResponseArgs* rrArgs)
{
   Config* cfg = App_getConfig(app);

   sigset_t oldSignalSet;

   FhgfsOpsErr rrRes;

   rrArgs->numRetries = Config_getConnNumCommRetries(cfg);
   rrArgs->rrFlags = REQUESTRESPONSEARGS_FLAG_USEDELAYEDINTERRUPT |
      REQUESTRESPONSEARGS_FLAG_ALLOWSTATESLEEP;
   rrArgs->respBufType = MessagingTkBufType_BufStore;

   // keep thread interruptible for connect (will later be made uninterruptible for send/recv)
   SignalTk_blockSignals(fhgfs_true, &oldSignalSet); // B L O C K _ S I G s

   rrRes = __MessagingTk_requestResponseTargetRetry(app, rrTarget, rrArgs);

   SignalTk_restoreSignals(&oldSignalSet); // U N B L O C K _ S I G s

   return rrRes;
}

/**
 * Sends a message to the owner of a target and receives a response.
 * Can handle target states and mapped mirror IDs. Owner node of target will be resolved and
 * referenced internally.
 *
 * If target states are provided, communication might be skipped for certain states.
 *
 * note: msg header targetID is automatically set to the resolved targetID.
 *
 * @param rrArgs rrArgs->node must be NULL when calling this.
 * @return received message and buffer are available through rrArgs in case of success.
 */
FhgfsOpsErr __MessagingTk_requestResponseTargetRetry(App* app,
   RequestResponseTarget* rrTarget, RequestResponseArgs* rrArgs)
{
   const char* logContext = "Messaging (RPC target)";

   unsigned currentRetryNum = 0; // used number of retries so far
   FhgfsOpsErr commRes;

   BEEGFS_BUG_ON_DEBUG(rrArgs->node, "rrArgs->node was not NULL and will leak now");

   for( ; ; ) // retry loop
   {
      // select the right targetID

      uint16_t targetID = rrTarget->targetID; // don't modify caller's targetID

      if(rrTarget->mirrorBuddies)
      { // given targetID refers to a buddy mirror group

         targetID = rrTarget->useBuddyMirrorSecond ?
            MirrorBuddyGroupMapper_getSecondaryTargetID(rrTarget->mirrorBuddies, targetID) :
            MirrorBuddyGroupMapper_getPrimaryTargetID(rrTarget->mirrorBuddies, targetID);

         if(unlikely(!targetID) )
         {
            Logger* log = App_getLogger(app);
            Logger_logErrFormatted(log, logContext, "Invalid target mirror buddy group ID: %hu",
               rrTarget->targetID);

            return FhgfsOpsErr_UNKNOWNTARGET;
         }
      }

      NetMessage_setMsgHeaderTargetID(rrArgs->requestMsg, targetID);

      // check target state

      if(rrTarget->targetStates)
      {
         CombinedTargetState state;
         fhgfs_bool getStateRes = TargetStateStore_getState(rrTarget->targetStates, targetID,
            &state);

         rrTarget->outTargetReachabilityState = state.reachabilityState;
         rrTarget->outTargetConsistencyState = state.consistencyState;

         if(unlikely( !getStateRes ||
            (state.reachabilityState != TargetReachabilityState_ONLINE) ||
            (state.consistencyState != TargetConsistencyState_GOOD) ) )
         {
            if(rrTarget->outTargetReachabilityState == TargetReachabilityState_OFFLINE)
            {
               LOG_DEBUG_FORMATTED(App_getLogger(app), Log_SPAM, logContext,
                  "Skipping communication with offline targetID: %hu", targetID);
               return FhgfsOpsErr_COMMUNICATION; // no need to wait for offline servers
            }

            if(!(rrArgs->rrFlags & REQUESTRESPONSEARGS_FLAG_ALLOWSTATESLEEP) )
            { // caller did not allow sleeping if target state is not {good, offline}
               LOG_DEBUG_FORMATTED(App_getLogger(app), Log_SPAM, logContext,
                  "Skipping communication with targetID: %hu; "
                  "target state: %s / %s",
                  targetID,
                  TargetStateStore_reachabilityStateToStr(rrTarget->outTargetReachabilityState),
                  TargetStateStore_consistencyStateToStr(rrTarget->outTargetConsistencyState) );
               return FhgfsOpsErr_COMMUNICATION;
            }

            // sleep on states other than "good" and "offline" with mirroring
            if(rrTarget->mirrorBuddies)
            {
               LOG_DEBUG_FORMATTED(App_getLogger(app), Log_DEBUG, logContext,
                  "Waiting before communication because of target state. "
                  "targetID: %hu; target state: %s / %s",
                  targetID,
                  TargetStateStore_reachabilityStateToStr(rrTarget->outTargetReachabilityState),
                  TargetStateStore_consistencyStateToStr(rrTarget->outTargetConsistencyState) );

               Thread_sleep(MSGTK_STATE_SLEEP_MS);
               if(Thread_isSignalPending() )
               { // make sure we don't loop endless if signal pending
                  LOG_DEBUG_FORMATTED(App_getLogger(app), Log_DEBUG, logContext,
                     "Waiting before communication was interrupted by signal. "
                     "targetID: %hu; target state: %s / %s",
                     targetID,
                     TargetStateStore_reachabilityStateToStr(rrTarget->outTargetReachabilityState),
                     TargetStateStore_consistencyStateToStr(rrTarget->outTargetConsistencyState) );
                  return FhgfsOpsErr_INTERRUPTED;
               }

               currentRetryNum = 0; // reset retries in case of unusable target state
               continue;
            }
         }
      }

      // reference node

      rrArgs->node = NodeStoreEx_referenceNodeByTargetID(rrTarget->nodeStore,
         targetID, rrTarget->targetMapper, NULL);

      if(!rrArgs->node)
      {
         Logger* log = App_getLogger(app);
         Logger_logFormatted(log, Log_WARNING, logContext,
            "Unable to resolve storage server for this targetID: %hu", targetID);

         return FhgfsOpsErr_UNKNOWNNODE;
      }

      // communicate

      commRes = __MessagingTk_requestResponseWithRRArgsComm(app, rrArgs);

      if(likely(commRes == FhgfsOpsErr_SUCCESS) )
         goto release_node_and_break;
      else
      if(Thread_isSignalPending() )
      { // no retry allowed in this situation
         commRes = FhgfsOpsErr_INTERRUPTED;
         goto release_node_and_break;
      }
      else
      if(!Node_getIsActive(rrArgs->node) )
      { // no retry allowed in this situation
         commRes = FhgfsOpsErr_UNKNOWNNODE;
         goto release_node_and_break;
      }
      else
      if(commRes == FhgfsOpsErr_WOULDBLOCK)
      { // no retries in this case
         commRes = FhgfsOpsErr_COMMUNICATION;
         goto release_node_and_break;
      }
      else
      if( (commRes == FhgfsOpsErr_AGAIN) && App_getConnRetriesEnabled(app) )
      { // retry infinitely
         currentRetryNum = 0;

         Thread_sleep(MSGTK_INFINITE_RETRY_WAIT_MS); // sleep interruptible

         goto release_node_and_continue;
      }
      else
      if(commRes != FhgfsOpsErr_COMMUNICATION)
      { // no retry allowed in this situation
         goto release_node_and_break;
      }

      if(App_getConnRetriesEnabled(app) &&
         (!rrArgs->numRetries || (currentRetryNum < rrArgs->numRetries) ) )
      { // we have a retry left
         MessagingTk_waitBeforeRetry(currentRetryNum);
         currentRetryNum++;

         if(currentRetryNum == 1 // log retry message only on first retry (to not spam the log)
            && !(rrArgs->logFlags & REQUESTRESPONSEARGS_LOGFLAG_RETRY) )
         {
            Logger* log = App_getLogger(app);
            Logger_logFormatted(log, Log_NOTICE, logContext,
               "Retrying communication with this targetID: %hu", targetID);
            Logger_logFormatted(log, Log_DEBUG, logContext,
               "Message type: %hu", NetMessage_getMsgType(rrArgs->requestMsg) );
         }
      }
      else
      { // no more retries left
         commRes = FhgfsOpsErr_COMMUNICATION;
         goto release_node_and_break;
      }

   release_node_and_continue:
      NodeStoreEx_releaseNode(rrTarget->nodeStore, &rrArgs->node);

      continue;

      // cleanup before early loop exit
   release_node_and_break:
      NodeStoreEx_releaseNode(rrTarget->nodeStore, &rrArgs->node);

      break;
   }

   return commRes;
}



/**
 * Send a request message to a node and receive the response.
 *
 * @param rrArgs:
 * .node receiver of msg;
 * .requestMsg the message that should be sent to the receiver;
 * .respMsgType expected response message type;
 * .outRespBuf response buffer if successful (must be returned to store by the caller);
 * .outRespMsg response message if successful (must be deleted by the caller);
 * @return FhgfsOpsErr_COMMUNICATION on comm error, FhgfsOpsErr_WOULDBLOCK if remote side
 *    encountered an indirect comm error and suggests not to try again, FhgfsOpsErr_AGAIN if other
 *    side is suggesting infinite retries.
 */
FhgfsOpsErr __MessagingTk_requestResponseWithRRArgsComm(App* app,
   RequestResponseArgs* rrArgs)
{
   /* note: keep in mind that there are multiple alternative response buf alloc types avilable,
      e.g. "kmalloc" or "get from store". */

   Logger* log = App_getLogger(app);
   const char* logContext = "Messaging (RPC)";

   NodeConnPool* connPool = Node_getConnPool(rrArgs->node);

   FhgfsOpsErr retVal = FhgfsOpsErr_COMMUNICATION;

   sigset_t oldSignalSet;
   fhgfs_bool useDelayedRecvInterrupt = fhgfs_false;

   unsigned bufLen; // length of shared send/recv buffer
   unsigned sendBufLen; // serialization length for sending
   ssize_t respRes = 0;
   ssize_t sendRes;

   // cleanup init
   Socket* sock = NULL;
   rrArgs->outRespBuf = NULL;
   rrArgs->outRespMsg = NULL;

   // connect
   // note: acquireStreamSocket() will fail immediately if a signal is pending

   sock = NodeConnPool_acquireStreamSocket(connPool);
   if(unlikely(!sock) )
   { // not connected
      if(!(rrArgs->logFlags & REQUESTRESPONSEARGS_LOGFLAG_CONNESTABLISHFAILED) &&
         !Thread_isSignalPending() )
      { // only log once and only if user didn't manually interrupt with signal (to avoid log spam)
         Logger_logFormatted(log, Log_WARNING, logContext,
            "Unable to connect to: %s", Node_getNodeIDWithTypeStr(rrArgs->node) );
         Logger_logFormatted(log, Log_DEBUG, logContext,
            "Message type: %hu", NetMessage_getMsgType(rrArgs->requestMsg) );

         rrArgs->logFlags |= REQUESTRESPONSEARGS_LOGFLAG_CONNESTABLISHFAILED;
      }

      return FhgfsOpsErr_COMMUNICATION;
   }

   // block interrupt signal

   if(rrArgs->rrFlags & REQUESTRESPONSEARGS_FLAG_USEDELAYEDINTERRUPT)
   {
      SignalTk_blockSignals(fhgfs_false, &oldSignalSet); // B L O C K _ S I G s (incl. SIGINT)

      // (note: oldSignalSet is a set of previously blocked signals, so we use "!contains")
      useDelayedRecvInterrupt = !SignalTk_containsSignal(&oldSignalSet, SIGINT);
   }

   // prepare send buffer

   sendBufLen = NetMessage_getMsgLength(rrArgs->requestMsg);

   if(rrArgs->respBufType == MessagingTkBufType_BufStore)
   { // pre-alloc'ed buffer from store

      NoAllocBufferStore* bufStore = App_getMsgBufStore(app);
      bufLen = NoAllocBufferStore_getBufSize(bufStore);

      if(unlikely(bufLen < sendBufLen) )
      { // should never happen: trying to send a msg that is larger than pre-alloc'ed buf size
         Logger_logFormatted(log, Log_CRITICAL, logContext,
            "BufferStore buf size (%u) too small for msg length (%u). Message type: %hu",
            bufLen, sendBufLen, NetMessage_getMsgType(rrArgs->requestMsg) );

         retVal = FhgfsOpsErr_INTERNAL;
         goto socket_invalidate;
      }

      rrArgs->outRespBuf = NoAllocBufferStore_waitForBuf(bufStore);
   }
   else
   { // alloc'ed buffer

      bufLen = MAX(MSGTK_KMALLOC_RECV_BUF_LEN, sendBufLen);

      rrArgs->outRespBuf = (char*)os_kmalloc(bufLen);
      if(unlikely(!rrArgs->outRespBuf) )
      {
         Logger_logFormatted(log, Log_CRITICAL, logContext,
            "Buffer allocation failed. Message type: %hu; Alloc size: %u",
            NetMessage_getMsgType(rrArgs->requestMsg), bufLen);

         retVal = FhgfsOpsErr_OUTOFMEM;
         goto socket_invalidate;
      }
   }

   NetMessage_serialize(rrArgs->requestMsg, rrArgs->outRespBuf, sendBufLen);

   // send request

   sendRes = sock->send(sock, rrArgs->outRespBuf, sendBufLen, 0);

   if(unlikely(sendRes != (ssize_t)sendBufLen) )
      goto socket_exception;

   // receive response
   // (note: recvMsgBufEx might re-enable SIGINT if useDelayedRecvInterrupt is set)

   respRes = MessagingTk_recvMsgBufEx(app, sock, rrArgs->outRespBuf, bufLen,
      useDelayedRecvInterrupt);

   if(unlikely(respRes <= 0) )
   { // error
      if(!(rrArgs->logFlags & REQUESTRESPONSEARGS_LOGFLAG_COMMERR) )
      {
         if(Thread_isSignalPending() )
            Logger_logFormatted(log, Log_NOTICE, logContext,
               "Receive interrupted by signal. Node: %s @ %s",
               Node_getNodeIDWithTypeStr(rrArgs->node), Socket_getPeername(sock) );
         else
         if(respRes == -ETIMEDOUT)
            Logger_logFormatted(log, Log_WARNING, logContext,
               "Receive timed out from %s @ %s",
               Node_getNodeIDWithTypeStr(rrArgs->node), Socket_getPeername(sock) );
         else
            Logger_logFormatted(log, Log_WARNING, logContext,
               "Receive failed from %s @ %s (recv result: %lld)",
               Node_getNodeIDWithTypeStr(rrArgs->node), Socket_getPeername(sock), respRes);

         Logger_logFormatted(log, Log_DEBUG, logContext,
            "Expected response type: %u", rrArgs->respMsgType);
      }

      goto socket_invalidate;
   }

   // got response => deserialize it
   rrArgs->outRespMsg = NetMessageFactory_createFromBuf(app, rrArgs->outRespBuf, respRes);

   if(unlikely(NetMessage_getMsgType(rrArgs->outRespMsg) == NETMSGTYPE_GenericResponse) )
   { // special control msg received
      retVal = __MessagingTk_handleGenericResponse(app, rrArgs);
      if(retVal != FhgfsOpsErr_INTERNAL)
      { // we can re-use the connection
         NodeConnPool_releaseStreamSocket(connPool, sock);
         goto cleanup_no_socket;
      }

      goto socket_invalidate;
   }

   if(unlikely(NetMessage_getMsgType(rrArgs->outRespMsg) != rrArgs->respMsgType) )
   { // response invalid (wrong msgType)
      Logger_logErrFormatted(log, logContext,
         "Received invalid response type: %hu; expected: %d. Disconnecting: %s (%s)",
         NetMessage_getMsgType(rrArgs->outRespMsg), rrArgs->respMsgType,
         Node_getNodeIDWithTypeStr(rrArgs->node), Socket_getPeername(sock) );

      retVal = FhgfsOpsErr_INTERNAL;
      goto socket_invalidate;
   }

   // correct response => return it (through rrArgs)

   NodeConnPool_releaseStreamSocket(connPool, sock);

   if(rrArgs->rrFlags & REQUESTRESPONSEARGS_FLAG_USEDELAYEDINTERRUPT)
      SignalTk_restoreSignals(&oldSignalSet); // U N B L O C K _ S I G s

   return FhgfsOpsErr_SUCCESS;


   // error handling (something went wrong)...

   socket_exception:
   {
      if(!(rrArgs->logFlags & REQUESTRESPONSEARGS_LOGFLAG_COMMERR) )
      {
         Logger_logErrFormatted(log, logContext,
            "Communication error: Node: %s (comm result: %lld; message type: %hu)",
            Node_getNodeIDWithTypeStr(rrArgs->node),
            (long long)( (sendRes <= 0) ? sendRes : respRes),
            NetMessage_getMsgType(rrArgs->requestMsg) );

         rrArgs->logFlags |= REQUESTRESPONSEARGS_LOGFLAG_COMMERR;
      }
   }

   socket_invalidate:
   {
      NodeConnPool_invalidateStreamSocket(connPool, sock);
   }

   // clean up
   cleanup_no_socket:

   if(rrArgs->outRespMsg)
   {
      NetMessage_virtualDestruct(rrArgs->outRespMsg);
      rrArgs->outRespMsg = NULL;
   }

   if(rrArgs->outRespBuf)
   {
      if(rrArgs->respBufType == MessagingTkBufType_BufStore)
      {
         NoAllocBufferStore* bufStore = App_getMsgBufStore(app);
         NoAllocBufferStore_addBuf(bufStore, rrArgs->outRespBuf);
      }
      else
         os_kfree(rrArgs->outRespBuf);

      rrArgs->outRespBuf = NULL;
   }

   if(rrArgs->rrFlags & REQUESTRESPONSEARGS_FLAG_USEDELAYEDINTERRUPT)
      SignalTk_restoreSignals(&oldSignalSet); // U N B L O C K _ S I G s

   return retVal;
}

/**
 * Print log message and determine appropriate return code for requestResponseComm.
 *
 * If FhgfsOpsErr_INTERNAL is returned, the connection should be invalidated.
 *
 * @return FhgfsOpsErr_COMMUNICATION on indirect comm error (retry suggested),
 *    FhgfsOpsErr_WOULDBLOCK if remote side encountered an indirect comm error and suggests not to
 *    try again, FhgfsOpsErr_AGAIN if other side is suggesting infinite retries.
 */
FhgfsOpsErr __MessagingTk_handleGenericResponse(App* app, RequestResponseArgs* rrArgs)
{
   Logger* log = App_getLogger(app);
   const char* logContext = "Messaging (RPC)";

   FhgfsOpsErr retVal;

   GenericResponseMsg* genericResp = (GenericResponseMsg*)rrArgs->outRespMsg;

   switch(GenericResponseMsg_getControlCode(genericResp) )
   {
      case GenericRespMsgCode_TRYAGAIN:
      {
         if(!(rrArgs->logFlags & REQUESTRESPONSEARGS_LOGFLAG_PEERTRYAGAIN) )
         {
            rrArgs->logFlags |= REQUESTRESPONSEARGS_LOGFLAG_PEERTRYAGAIN;

            Logger_logFormatted(log, Log_NOTICE, logContext,
               "Peer is asking for a retry: %s; Reason: %s",
               Node_getNodeIDWithTypeStr(rrArgs->node),
               GenericResponseMsg_getLogStr(genericResp) );
            Logger_logFormatted(log, Log_DEBUG, logContext,
               "Message type: %hu", NetMessage_getMsgType(rrArgs->requestMsg) );
         }

         retVal = FhgfsOpsErr_AGAIN;
      } break;

      case GenericRespMsgCode_INDIRECTCOMMERR:
      {
         if(!(rrArgs->logFlags & REQUESTRESPONSEARGS_LOGFLAG_PEERINDIRECTCOMMM) )
         {
            rrArgs->logFlags |= REQUESTRESPONSEARGS_LOGFLAG_PEERINDIRECTCOMMM;

            Logger_logFormatted(log, Log_NOTICE, logContext,
               "Peer reported indirect communication error: %s; Reason: %s",
               Node_getNodeIDWithTypeStr(rrArgs->node),
               GenericResponseMsg_getLogStr(genericResp) );
            Logger_logFormatted(log, Log_DEBUG, logContext,
               "Message type: %hu", NetMessage_getMsgType(rrArgs->requestMsg) );
         }

         retVal = FhgfsOpsErr_COMMUNICATION;
      } break;

      case GenericRespMsgCode_INDIRECTCOMMERR_NOTAGAIN:
      {
         if(!(rrArgs->logFlags & REQUESTRESPONSEARGS_LOGFLAG_PEERINDIRECTCOMMM_NOTAGAIN) )
         {
            rrArgs->logFlags |= REQUESTRESPONSEARGS_LOGFLAG_PEERINDIRECTCOMMM_NOTAGAIN;

            Logger_logFormatted(log, Log_NOTICE, logContext,
               "Peer reported indirect communication error (no retry): %s; Reason: %s",
               Node_getNodeIDWithTypeStr(rrArgs->node),
               GenericResponseMsg_getLogStr(genericResp) );
            Logger_logFormatted(log, Log_DEBUG, logContext,
               "Message type: %hu", NetMessage_getMsgType(rrArgs->requestMsg) );
         }

         retVal = FhgfsOpsErr_WOULDBLOCK;
      } break;

      default:
      {
         Logger_logFormatted(log, Log_NOTICE, logContext,
            "Peer replied with unknown control code: %s; Code: %u; Reason: %s",
            Node_getNodeIDWithTypeStr(rrArgs->node),
            (unsigned)GenericResponseMsg_getControlCode(genericResp),
            GenericResponseMsg_getLogStr(genericResp) );
         Logger_logFormatted(log, Log_DEBUG, logContext,
            "Message type: %hu", NetMessage_getMsgType(rrArgs->requestMsg) );

         retVal = FhgfsOpsErr_INTERNAL;
      } break;
   }


   return retVal;
}
