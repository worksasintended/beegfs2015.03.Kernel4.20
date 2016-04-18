#include <common/app/log/LogContext.h>
#include <common/app/AbstractApp.h>
#include <common/net/message/control/GenericResponseMsg.h>
#include <common/net/message/AbstractNetMessageFactory.h>
#include <common/net/message/NetMessageLogHelper.h>
#include <common/threading/PThread.h>
#include "MessagingTk.h"


#define MSGBUF_DEFAULT_SIZE     (64*1024) // at least big enough to store a datagram
#define MSGBUF_MAX_SIZE         (4*1024*1024) // max accepted size


bool MessagingTk::requestResponse(RequestResponseArgs* rrArgs)
{
   const char* logContext = "Messaging (RPC)";

   size_t numRetries = 0; // used number of retries so far
   FhgfsOpsErr commRes;

   do
   {
      commRes = requestResponseComm(rrArgs);

      if(likely(commRes == FhgfsOpsErr_SUCCESS) )
         break;
      else
      if(commRes == FhgfsOpsErr_WOULDBLOCK)
      {
         commRes = FhgfsOpsErr_COMMUNICATION;
         break; // no retries in this case
      }
      else
      if(commRes == FhgfsOpsErr_AGAIN)
      { // retry infinitely
         numRetries = 0;

         bool shallTerminate = PThread::getCurrentThreadApp()->waitForSelfTerminateOrder(
            MESSAGINGTK_INFINITE_RETRY_WAIT_MS);
         if(shallTerminate)
         {
            commRes = FhgfsOpsErr_INTERRUPTED;
            break;
         }

         continue;
      }
      else
      if(commRes != FhgfsOpsErr_COMMUNICATION)
         break; // don't retry if this wasn't a comm error

      if(!numRetries) // log only on first retry
      {
         if ( !(rrArgs->logFlags & REQUESTRESPONSEARGS_LOGFLAG_RETRY) )
         {
            LogContext(logContext).log(Log_WARNING,
               "Retrying communication with node: " + rrArgs->node->getNodeIDWithTypeStr() + " "
               "(Message type: " + rrArgs->requestMsg->getMsgTypeStr() + ")");
         }
      }

   } while(numRetries++ < MESSAGINGTK_NUM_COMM_RETRIES);

   return (commRes == FhgfsOpsErr_SUCCESS);
}

/**
 * Sends a message and receives the response.
 *
 * @param outRespBuf the underlying buffer for outRespMsg needs to be freed by the caller
 * if true is returned.
 * @param outRespMsg response message; needs to be freed by the caller if true is returned.
 * @return true if communication succeeded and expected response was received
 */
bool MessagingTk::requestResponse(Node* node, NetMessage* requestMsg, unsigned respMsgType,
   char** outRespBuf, NetMessage** outRespMsg)
{
   RequestResponseArgs rrArgs(node, requestMsg, respMsgType);

   bool rrRes = requestResponse(&rrArgs);

   *outRespBuf = rrArgs.outRespBuf;
   *outRespMsg = rrArgs.outRespMsg;

   rrArgs.outRespBuf = NULL; // to avoid deletion in RequestResponseArgs destructor
   rrArgs.outRespMsg = NULL; // to avoid deletion in RequestResponseArgs destructor

   return rrRes;
}

/**
 * Sends a message to a node and receives a response.
 * Can handle target states and mapped mirror IDs. Node does not need to be referenced by caller.
 *
 * If target states are provided, communication might be skipped for certain states.
 *
 * note: rrArgs->nodeID may optionally be provided when calling this.
 * note: received message and buffer are available through rrArgs in case of success.
 */
FhgfsOpsErr MessagingTk::requestResponseNode(RequestResponseNode* rrNode,
   RequestResponseArgs* rrArgs)
{
   const char* logContext = "Messaging (RPC node)";

   size_t numRetries = 0; // used number of retries so far
   FhgfsOpsErr commRes;

   do
   {
      // select the right targetID

      uint16_t nodeID = rrNode->nodeID; // don't modify caller's nodeID

      if(rrNode->mirrorBuddies)
      { // given targetID refers to a buddy mirror group

         nodeID = rrNode->useBuddyMirrorSecond ?
            rrNode->mirrorBuddies->getSecondaryTargetID(nodeID) :
            rrNode->mirrorBuddies->getPrimaryTargetID(nodeID);

         if(unlikely(!nodeID) )
         {
            LogContext(logContext).logErr(
               "Invalid node mirror buddy group ID: " + StringTk::uintToStr(rrNode->nodeID) + "; "
               "type: " + (rrArgs->node ?
                  rrArgs->node->getNodeTypeStr() : rrNode->nodeStore->getStoreTypeStr() ) );

            return FhgfsOpsErr_UNKNOWNNODE;
         }
      }

      // check target state

      if(rrNode->targetStates)
      {
         CombinedTargetState targetState;
         if (!rrNode->targetStates->getState(nodeID, targetState) )
         {
            LOG_DEBUG(logContext, Log_DEBUG,
               "Node has unknown state: " + StringTk::uintToStr(nodeID) );
            return FhgfsOpsErr_COMMUNICATION;
         }

         rrNode->outTargetReachabilityState = targetState.reachabilityState;
         rrNode->outTargetConsistencyState = targetState.consistencyState;

         if(unlikely(
            (rrNode->outTargetReachabilityState != TargetReachabilityState_ONLINE) ||
            (rrNode->outTargetConsistencyState != TargetConsistencyState_GOOD) ) )
         {
            if(rrNode->outTargetReachabilityState == TargetReachabilityState_OFFLINE)
            {
               LOG_DEBUG(logContext, Log_SPAM,
                  "Skipping communication with offline nodeID: " + StringTk::uintToStr(nodeID) +"; "
                  "type: " + (rrArgs->node ?
                     rrArgs->node->getNodeTypeStr() : rrNode->nodeStore->getStoreTypeStr() ) );
               return FhgfsOpsErr_COMMUNICATION; // no need to wait for offline servers
            }

            // states other than "good" and "needs resync" are not allowed with mirroring
            if(rrNode->mirrorBuddies &&
               (rrNode->outTargetConsistencyState != TargetConsistencyState_NEEDS_RESYNC) )
            {
               LOG_DEBUG(logContext, Log_DEBUG,
                  "Skipping communication with mirror nodeID: " + StringTk::uintToStr(nodeID) + "; "
                  "node state: " +
                     TargetStateStore::stateToStr(rrNode->outTargetConsistencyState) + "; "
                  "type: " + (rrArgs->node ?
                     rrArgs->node->getNodeTypeStr() : rrNode->nodeStore->getStoreTypeStr() ) );
               return FhgfsOpsErr_COMMUNICATION;
            }
         }
      }

      // reference node (if not provided by caller already)

      bool nodeNeedsRelease = false;

      if(!rrArgs->node)
      {
         rrArgs->node = rrNode->nodeStore->referenceNode(nodeID);

         if(!rrArgs->node)
         {
            LogContext(logContext).log(Log_WARNING,
               "Unknown nodeID: " + StringTk::uintToStr(nodeID) + "; "
               "type: " + rrNode->nodeStore->getStoreTypeStr() );

            return FhgfsOpsErr_UNKNOWNNODE;
         }

         nodeNeedsRelease = true;
      }
      else
         BEEGFS_BUG_ON_DEBUG(rrArgs->node->getNumID() != nodeID,
            "Mismatch between given rrArgs->node ID and nodeID");

      // communicate

      commRes = requestResponseComm(rrArgs);

      if(nodeNeedsRelease)
         rrNode->nodeStore->releaseNode(&rrArgs->node);

      if(likely(commRes == FhgfsOpsErr_SUCCESS) )
         break;
      else
      if(commRes == FhgfsOpsErr_WOULDBLOCK)
      {
         commRes = FhgfsOpsErr_COMMUNICATION;
         break; // no retries in this case
      }
      else
      if(commRes == FhgfsOpsErr_AGAIN)
      { // retry infinitely
         numRetries = 0;

         bool shallTerminate = PThread::getCurrentThreadApp()->waitForSelfTerminateOrder(
            MESSAGINGTK_INFINITE_RETRY_WAIT_MS);
         if(shallTerminate)
         {
            commRes = FhgfsOpsErr_INTERRUPTED;
            break;
         }

         continue;
      }
      else
      if(commRes != FhgfsOpsErr_COMMUNICATION)
         break; // don't retry if this wasn't a comm error

      if(!numRetries) // log only on first retry
      {
         LogContext(logContext).log(Log_WARNING,
            "Retrying communication with node: " +
            rrNode->nodeStore->getNodeIDWithTypeStr(nodeID) + "; "
            "Message type: " + rrArgs->requestMsg->getMsgTypeStr() );
      }

   } while(numRetries++ < MESSAGINGTK_NUM_COMM_RETRIES);

   return commRes;
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
FhgfsOpsErr MessagingTk::requestResponseTarget(RequestResponseTarget* rrTarget,
   RequestResponseArgs* rrArgs)
{
   const char* logContext = "Messaging (RPC target)";

   size_t numRetries = 0; // used number of retries so far
   FhgfsOpsErr commRes;

   BEEGFS_BUG_ON_DEBUG(rrArgs->node, "rrArgs->node was not NULL and will leak now");

   do
   {
      // select the right targetID

      uint16_t targetID = rrTarget->targetID; // don't modify caller's targetID

      if(rrTarget->mirrorBuddies)
      { // given targetID refers to a buddy mirror group

         targetID = rrTarget->useBuddyMirrorSecond ?
            rrTarget->mirrorBuddies->getSecondaryTargetID(targetID) :
            rrTarget->mirrorBuddies->getPrimaryTargetID(targetID);

         if(unlikely(!targetID) )
         {
            LogContext(logContext).logErr("Invalid target mirror buddy group ID: " +
               StringTk::uintToStr(rrTarget->targetID) );

            return FhgfsOpsErr_UNKNOWNTARGET;
         }
      }

      rrArgs->requestMsg->setMsgHeaderTargetID(targetID);

      // check target state

      if(rrTarget->targetStates)
      {
         CombinedTargetState targetState;
         if (!rrTarget->targetStates->getState(targetID, targetState) )
         {
            // maybe the target was removed, check the mapper as well to be sure
            if(!rrTarget->targetMapper->targetExists(targetID) )
               return FhgfsOpsErr_UNKNOWNTARGET;

            LOG_DEBUG(logContext, Log_DEBUG,
               "Target has unknown state: " + StringTk::uintToStr(targetID) );
            return FhgfsOpsErr_COMMUNICATION;
         }

         rrTarget->outTargetReachabilityState = targetState.reachabilityState;
         rrTarget->outTargetConsistencyState = targetState.consistencyState;

         if(unlikely(
            (rrTarget->outTargetReachabilityState != TargetReachabilityState_ONLINE) ||
            (rrTarget->outTargetConsistencyState != TargetConsistencyState_GOOD) ) )
         {
            if(rrTarget->outTargetReachabilityState == TargetReachabilityState_OFFLINE)
            {
               LOG_DEBUG(logContext, Log_SPAM,
                  "Skipping communication with offline targetID: " +
                  StringTk::uintToStr(targetID) );
               return FhgfsOpsErr_COMMUNICATION; // no need to wait for offline servers
            }

            // states other than "good" and "resyncing" are not allowed with mirroring
            if(rrTarget->mirrorBuddies &&
               (rrTarget->outTargetConsistencyState != TargetConsistencyState_NEEDS_RESYNC) )
            {
               LOG_DEBUG(logContext, Log_DEBUG,
                  "Skipping communication with mirror targetID: " +
                  StringTk::uintToStr(targetID) + "; "
                  "target state: " +
                  TargetStateStore::stateToStr(rrTarget->outTargetConsistencyState) );
               return FhgfsOpsErr_COMMUNICATION;
            }
         }
      }

      // reference node

      rrArgs->node = rrTarget->nodeStore->referenceNodeByTargetID(
         targetID, rrTarget->targetMapper);

      if(!rrArgs->node)
      {
         LogContext(logContext).log(Log_WARNING,
            "Unable to resolve storage server for this targetID: " +
            StringTk::uintToStr(targetID) );

         return FhgfsOpsErr_UNKNOWNNODE;
      }

      // communicate

      commRes = requestResponseComm(rrArgs);

      rrTarget->nodeStore->releaseNode(&rrArgs->node);

      if(likely(commRes == FhgfsOpsErr_SUCCESS) )
         break;
      else
      if(commRes == FhgfsOpsErr_WOULDBLOCK)
      {
         commRes = FhgfsOpsErr_COMMUNICATION;
         break; // no retries in this case
      }
      else
      if(commRes == FhgfsOpsErr_AGAIN)
      { // retry infinitely
         numRetries = 0;

         bool shallTerminate = PThread::getCurrentThreadApp()->waitForSelfTerminateOrder(
            MESSAGINGTK_INFINITE_RETRY_WAIT_MS);
         if(shallTerminate)
         {
            commRes = FhgfsOpsErr_INTERRUPTED;
            break;
         }

         continue;
      }
      else
      if(commRes != FhgfsOpsErr_COMMUNICATION)
         break; // don't retry if this wasn't a comm error

      if(!numRetries) // log only on first retry
      {
         LogContext(logContext).log(Log_WARNING,
            "Retrying communication with this targetID: " +
            StringTk::uintToStr(targetID) + "; "
            "Message type: " + rrArgs->requestMsg->getMsgTypeStr() );
      }

   } while(numRetries++ < MESSAGINGTK_NUM_COMM_RETRIES);

   return commRes;
}


/**
 * Note: This version will allocate the message buffer.
 * 
 * @param outBuf contains the received message; will be allocated and has to be
 * freed by the caller (only if the return value is greater than 0)
 * @return 0 on error (e.g. message to big), message length otherwise
 * @throw SocketException
 */ 
unsigned MessagingTk::recvMsgBuf(Socket* sock, char** outBuf)
{
   const char* logContext = "MessagingTk (recv msg out-buf)";

   const int recvTimeoutMS = CONN_LONG_TIMEOUT;
   
   unsigned numReceived = 0;

   *outBuf = (char*)malloc(MSGBUF_DEFAULT_SIZE);
   if(unlikely(!*outBuf) )
   {
      LogContext(logContext).log(Log_WARNING,
         "Memory allocation for incoming message buffer failed: " + sock->getPeername() );
      return 0;
   }
   
   try
   {
      // receive at least the message header
      
      numReceived += sock->recvExactT(*outBuf, NETMSG_MIN_LENGTH, 0, recvTimeoutMS);
      
      unsigned msgLength = NetMessage::extractMsgLengthFromBuf(*outBuf);
      
      if(msgLength > MSGBUF_MAX_SIZE)
      { // message too big to be accepted
         LogContext(logContext).log(Log_NOTICE,
            "Received a message with invalid length from: " + sock->getPeername() );
         
         SAFE_FREE(*outBuf);
         
         return 0;
      }
      else
      if(msgLength > MSGBUF_DEFAULT_SIZE)
      { // message larger than the default buffer
         char* oldBuf = *outBuf;

         *outBuf = (char*)realloc(*outBuf, msgLength);
         if(unlikely(!*outBuf) )
         {
            LogContext(logContext).log(Log_WARNING,
               "Memory reallocation for incoming message buffer failed: " + sock->getPeername() );
            free(oldBuf);
            return 0;
         }
      }
      
      // receive the rest of the message

      if(msgLength > numReceived)
         sock->recvExactT(&(*outBuf)[numReceived], msgLength-numReceived, 0, recvTimeoutMS);
      
      return msgLength;
   }
   catch(SocketException& e)
   {
      SAFE_FREE(*outBuf);
      
      throw;
   }
}


/**
 * Sends a request message to a node and receives the response.
 * 
 * @param rrArgs:
 *    .node receiver
 *    .requestMsg the message that should be sent to the receiver
 *    .respMsgType expected response message type
 *    .outRespBuf response buffer if successful (must be freed by the caller)
 *    .outRespMsg response message if successful (must be deleted by the caller)
 * @return FhgfsOpsErr_COMMUNICATION on comm error, FhgfsOpsErr_WOULDBLOCK if remote side
 *    encountered an indirect comm error and suggests not to try again, FhgfsOpsErr_AGAIN if other
 *    side is suggesting infinite retries.
 */
FhgfsOpsErr MessagingTk::requestResponseComm(RequestResponseArgs* rrArgs)
{
   const char* logContext = "Messaging (RPC)";
   
   Node* node = rrArgs->node;
   NodeConnPool* connPool = node->getConnPool();
   AbstractNetMessageFactory* netMessageFactory =
      PThread::getCurrentThreadApp()->getNetMessageFactory();

   FhgfsOpsErr retVal = FhgfsOpsErr_INTERNAL;

   // cleanup init
   Socket* sock = NULL;
   char* sendBuf = NULL;
   rrArgs->outRespBuf = NULL;
   rrArgs->outRespMsg = NULL;
   
   try
   {
      // connect
      sock = connPool->acquireStreamSocket();

      // prepare sendBuf
      size_t sendBufLen = rrArgs->requestMsg->getMsgLength();
      sendBuf = (char*)malloc(sendBufLen);

      if(unlikely(!sendBuf) )
      { // malloc failed
         LogContext(logContext).logErr(
            "Memory allocation for send buffer failed. "
            "Alloc size: " + StringTk::uintToStr(sendBufLen) );

         retVal = FhgfsOpsErr_OUTOFMEM;
         goto err_cleanup;
      }

      rrArgs->requestMsg->serialize(sendBuf, sendBufLen);
   
      // send request
      sock->send(sendBuf, sendBufLen, 0);
      SAFE_FREE(sendBuf);

      // receive response
      unsigned respLength = MessagingTk::recvMsgBuf(sock, &rrArgs->outRespBuf);
      if(unlikely(!respLength) )
      { // error (e.g. message too big)
         LogContext(logContext).log(Log_WARNING,
            "Failed to receive response from: " + node->getNodeIDWithTypeStr() + "; " +
            sock->getPeername() + ". " +
            "(Message type: " + rrArgs->requestMsg->getMsgTypeStr() + ")");

         goto err_cleanup;
      }

      // got response => deserialize it
      rrArgs->outRespMsg = netMessageFactory->createFromBuf(rrArgs->outRespBuf, respLength);

      if(unlikely(rrArgs->outRespMsg->getMsgType() == NETMSGTYPE_GenericResponse) )
      { // special control msg received
         retVal = handleGenericResponse(rrArgs);
         if(retVal != FhgfsOpsErr_INTERNAL)
         { // we can re-use the connection
            connPool->releaseStreamSocket(sock);
            sock = NULL;
         }

         goto err_cleanup;
      }

      if(unlikely(rrArgs->outRespMsg->getMsgType() != rrArgs->respMsgType) )
      { // response invalid (wrong msgType)
         LogContext(logContext).logErr(
            "Received invalid response type: " + rrArgs->outRespMsg->getMsgTypeStr() + "; "
            "expected: " + NetMsgStrMapping().defineToStr(rrArgs->respMsgType) + ". "
            "Disconnecting: " + node->getNodeIDWithTypeStr() + " @ " +
            sock->getPeername() );
         
         goto err_cleanup;
      }

      // got correct response

      connPool->releaseStreamSocket(sock);

      return FhgfsOpsErr_SUCCESS;
   }
   catch(SocketConnectException& e)
   {
      if ( !(rrArgs->logFlags & REQUESTRESPONSEARGS_LOGFLAG_CONNESTABLISHFAILED) )
      {
         LogContext(logContext).log(Log_WARNING,
            "Unable to connect to: " + node->getNodeIDWithTypeStr() + ". " +
            "(Message type: " + rrArgs->requestMsg->getMsgTypeStr() + ")");
      }

      retVal = FhgfsOpsErr_COMMUNICATION;
   }
   catch(SocketException& e)
   {
      LogContext(logContext).logErr("Communication error: " + std::string(e.what() ) + "; " +
         "Peer: " + node->getNodeIDWithTypeStr() + ". "
         "(Message type: " + rrArgs->requestMsg->getMsgTypeStr() + ")");

      retVal = FhgfsOpsErr_COMMUNICATION;
   }

   
err_cleanup:

   // clean up...

   if(sock)
      connPool->invalidateStreamSocket(sock);

   SAFE_DELETE(rrArgs->outRespMsg);
   SAFE_FREE(rrArgs->outRespBuf);
   SAFE_FREE(sendBuf);

   return retVal;
}

/**
 * Creates a message buffer of the required size and serializes the message to it.
 * 
 * @return buffer must be freed by the caller
 */
char* MessagingTk::createMsgBuf(NetMessage* msg)
{
   size_t bufLen = msg->getMsgLength();
   char* buf = (char*)malloc(bufLen);
   
   msg->serialize(buf, bufLen);
   
   return buf;
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
FhgfsOpsErr MessagingTk::handleGenericResponse(RequestResponseArgs* rrArgs)
{
   const char* logContext = "Messaging (RPC)";

   FhgfsOpsErr retVal;

   GenericResponseMsg* genericResp = (GenericResponseMsg*)rrArgs->outRespMsg;

   switch(genericResp->getControlCode() )
   {
      case GenericRespMsgCode_TRYAGAIN:
      {
         if(!(rrArgs->logFlags & REQUESTRESPONSEARGS_LOGFLAG_PEERTRYAGAIN) )
         {
            rrArgs->logFlags |= REQUESTRESPONSEARGS_LOGFLAG_PEERTRYAGAIN;

            LogContext(logContext).log(Log_NOTICE,
               "Peer is asking for a retry: " +
                  rrArgs->node->getNodeIDWithTypeStr() + "; "
               "Reason: " + genericResp->getLogStr() + ". "
               "(Message type: " + rrArgs->requestMsg->getMsgTypeStr() + ")");
         }

         retVal = FhgfsOpsErr_AGAIN;
      } break;

      case GenericRespMsgCode_INDIRECTCOMMERR:
      {
         if(!(rrArgs->logFlags & REQUESTRESPONSEARGS_LOGFLAG_PEERINDIRECTCOMMM) )
         {
            rrArgs->logFlags |= REQUESTRESPONSEARGS_LOGFLAG_PEERINDIRECTCOMMM;

            LogContext(logContext).log(Log_NOTICE,
               "Peer reported indirect communication error: " +
                  rrArgs->node->getNodeIDWithTypeStr() + "; "
               "Details: " + genericResp->getLogStr() + ". "
               "(Message type: " + rrArgs->requestMsg->getMsgTypeStr() + ")");
         }

         retVal = FhgfsOpsErr_COMMUNICATION;
      } break;

      case GenericRespMsgCode_INDIRECTCOMMERR_NOTAGAIN:
      {
         if(!(rrArgs->logFlags & REQUESTRESPONSEARGS_LOGFLAG_PEERINDIRECTCOMMM_NOTAGAIN) )
         {
            rrArgs->logFlags |= REQUESTRESPONSEARGS_LOGFLAG_PEERINDIRECTCOMMM_NOTAGAIN;

            LogContext(logContext).log(Log_NOTICE,
               "Peer reported indirect communication error (no retry): " +
                  rrArgs->node->getNodeIDWithTypeStr() + "; "
               "Details: " + genericResp->getLogStr() + ". "
               "(Message type: " + rrArgs->requestMsg->getMsgTypeStr() + ")");
         }

         retVal = FhgfsOpsErr_WOULDBLOCK;
      } break;

      default:
      {
         LogContext(logContext).log(Log_NOTICE,
            "Peer replied with unknown control code: " +
               rrArgs->node->getNodeIDWithTypeStr() + "; "
            "Code: " + StringTk::uintToStr(genericResp->getControlCode() ) + "; "
            "Details: " + genericResp->getLogStr() + ". "
            "(Message type: " + rrArgs->requestMsg->getMsgTypeStr() + ")");
         retVal = FhgfsOpsErr_INTERNAL;
      } break;
   }


   return retVal;
}



