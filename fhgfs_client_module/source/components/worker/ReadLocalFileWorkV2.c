#include <app/log/Logger.h>
#include <app/App.h>
#include <common/net/message/session/rw/ReadLocalFileV2Msg.h>
#include <common/net/message/NetMessage.h>
#include <common/nodes/TargetMapper.h>
#include <common/threading/Thread.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/VersionTk.h>
#include <common/components/worker/WorkQueue.h>
#include <nodes/NodeStoreEx.h>
#include "ReadLocalFileWorkV2.h"


void ReadLocalFileWorkV2_process(Work* this, char* bufIn, unsigned bufInLen,
   char* bufOut, unsigned bufOutLen)
{
   ReadLocalFileWorkV2* thisCast = (ReadLocalFileWorkV2*)this;

   TargetMapper* targetMapper = App_getTargetMapper(thisCast->app);
   NodeStoreEx* nodes = App_getStorageNodes(thisCast->app);

   FhgfsOpsErr resolveErr;

   Node* node = NodeStoreEx_referenceNodeByTargetID(nodes, thisCast->targetID, targetMapper,
      &resolveErr);
   if(unlikely(!node) )
   { // unable to resolve targetID
      *thisCast->nodeResult = -resolveErr;
   }
   else
   { // got node reference => begin communication
      int64_t commRes = __ReadLocalFileWorkV2_communicate(
         thisCast, node, bufIn, bufInLen, bufOut, bufOutLen);

      NodeStoreEx_releaseNode(nodes, &node);

      // handle retry
      if(unlikely(commRes == -FhgfsOpsErr_COMMUNICATION) )
      { // comm error occurred => prepare retry (if allowed)
         if(__ReadLocalFileWorkV2_prepareRetry(thisCast) )
         {
            // retry prepared => don't inc the done-counter and don't touch any shared variables
            return;
         }
      }
   }

   SynchronizedCounter_incCount(thisCast->counter);
}

int64_t __ReadLocalFileWorkV2_communicate(ReadLocalFileWorkV2* this, Node* node,
   char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen)
{
   const char* logContext = "ReadFileV2 Work (communication)";

   Logger* log = App_getLogger(this->app);

   Node* localNode = App_getLocalNode(this->app);
   char* localNodeID = Node_getID(localNode);

   NodeConnPool* connPool = Node_getConnPool(node);
   Socket* sock = NULL;
   fhgfs_bool connFailedLogged = fhgfs_false;

   ReadLocalFileV2Msg readMsg;
   ssize_t sendRes;
   int64_t numReceivedFileBytes = 0;

   *this->nodeResult = -FhgfsOpsErr_COMMUNICATION;

   // request init
   ReadLocalFileV2Msg_initFromSession(&readMsg, localNodeID,
      this->fileHandleID, this->targetID, this->pathInfo, this->accessFlags, this->offset,
      this->size);

   if (this->firstWriteDoneForTarget)
      NetMessage_addMsgHeaderFeatureFlag( (NetMessage*)&readMsg,
         READLOCALFILEMSG_FLAG_SESSION_CHECK);

   if(App_getNetBenchModeEnabled(this->app) )
      NetMessage_addMsgHeaderFeatureFlag( (NetMessage*)&readMsg, READLOCALFILEMSG_FLAG_DISABLE_IO);

   // connect
   sock = NodeConnPool_acquireStreamSocket(connPool);
   if(unlikely(!sock) )
   { // not connected
      if(!connFailedLogged)
         Logger_logFormatted(log, 2, logContext, "Unable to connect to storage server: %s",
            Node_getID(node) );

      connFailedLogged = fhgfs_true;
      goto loop_cleanup;
   }

   // prepare and send message
   NetMessage_serialize( (NetMessage*)&readMsg, bufOut, bufOutLen);

   sendRes = sock->send(sock, bufOut, NetMessage_getMsgLength( (NetMessage*)&readMsg), 0);
   if(unlikely(sendRes <= 0) )
   {
      Logger_logFormatted(log, 2, logContext,
         "Failed to send request to %s: %s", Node_getID(node), Socket_getPeername(sock) );
      goto loop_socketexception;
   }

   // recv length info and file data loop
   // note: we jump directly to cleanup from within this loop if the received header indicates
   //    end of transmission
   for( ; ; )
   {
      ssize_t recvRes;
      char dataLenBuf[sizeof(int64_t)]; // length info in fhgfs network byte order
      int64_t lengthInfo; // length info in fhgfs host byte order
      unsigned lengthInfoBufLen; // deserialization buf length for lengthInfo (ignored)

      recvRes = Socket_recvExactT(sock, &dataLenBuf, sizeof(int64_t), 0, CONN_LONG_TIMEOUT);
      if(unlikely(recvRes <= 0) )
      { // error
         Logger_logFormatted(log, Log_WARNING, logContext,
            "Failed to receive length info from %s: %s",
            Node_getID(node), Socket_getPeername(sock) );
         goto loop_socketexception;
      }

      // got the length info response
      Serialization_deserializeInt64(
         dataLenBuf, sizeof(int64_t), &lengthInfo, &lengthInfoBufLen);

      //LOG_DEBUG_FORMATTED(log, 5, logContext,
      //   "Received lengthInfo from %s: %lld", this->nodeID, (long long)lengthInfo);

      if(lengthInfo <= 0)
      { // end of file data transmission
         NodeConnPool_releaseStreamSocket(connPool, sock);

         if(unlikely(lengthInfo < 0) )
         { // error occurred
            *this->nodeResult = lengthInfo;
         }
         else
         { // normal end of file data transmission
            *this->nodeResult = numReceivedFileBytes;
         }

         goto loop_cleanup;
      }

      // buffer overflow check
      if(unlikely( (uint64_t)(numReceivedFileBytes + lengthInfo) > this->size) )
      {
         Logger_logErrFormatted(log, logContext,
            "Received a lengthInfo that would lead to a buffer overflow from %s: %lld",
            Node_getID(node), (long long)lengthInfo);

         *this->nodeResult = -FhgfsOpsErr_INTERNAL;

         goto loop_socketinvalidate;
      }

      // positive result => node is going to send some file data

      // receive announced dataPart
      recvRes = Socket_recvExactT(
         sock, &(this->buf)[numReceivedFileBytes], lengthInfo, 0, CONN_LONG_TIMEOUT);
      if(unlikely(recvRes <= 0) )
      { // something went wrong
         if(recvRes == -ETIMEDOUT)
         { // timeout
            Logger_logErrFormatted(log, logContext,
               "Communication timeout in RECVDATA stage. Node: %s", Node_getID(node) );
         }
         else
         { // error
            Logger_logErrFormatted(log, logContext,
               "Communication error in RECVDATA stage. Node: %s (recv result: %lld)",
               Node_getID(node), (long long)recvRes);
         }

         Logger_logFormatted(log, Log_DEBUG, logContext,
            "Request details: received/requested size from %s: %lld/%lld", Node_getID(node),
            (long long)numReceivedFileBytes, (long long)this->size);

         goto loop_socketexception;
      }

      numReceivedFileBytes += lengthInfo;

   } // end of recv file data + header loop


loop_socketexception:
   Logger_logErrFormatted(log, logContext,
      "Communication error. NodeID: %s", Node_getID(node));

   Logger_logFormatted(log, 4, logContext,
      "Sent request: nodeID/fileHandleID/offset/size: %s/%s/%lld/%lld",
      Node_getID(node), this->fileHandleID, (long long)this->offset, (long long)this->size);

   *this->nodeResult = -FhgfsOpsErr_COMMUNICATION;


loop_socketinvalidate:
   NodeConnPool_invalidateStreamSocket(connPool, sock);


loop_cleanup:
   ReadLocalFileV2Msg_uninit( (NetMessage*)&readMsg);

   return *this->nodeResult;

}

/**
 * @return fhgfs_true if a retry was prepared (e.g. caller shouldn't notify waiters), fhgfs_false
 * if no retry allowed and we're done
 */
fhgfs_bool __ReadLocalFileWorkV2_prepareRetry(ReadLocalFileWorkV2* this)
{
   RetryableWork* thisRetryWork = (RetryableWork*)this;
   Config* cfg = App_getConfig(this->app);
   unsigned maxNumRetries = Config_getConnNumCommRetries(cfg);

   if(RetryableWork_getIsInterrupted(thisRetryWork) )
   { // interrupted by initiator => no retry
      *this->nodeResult = -FhgfsOpsErr_INTERRUPTED;
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

      ReadLocalFileWorkV2* newWork = ReadLocalFileWorkV2_construct(this->app, this->buf,
         this->offset, this->size, this->fileHandleID, this->accessFlags, this->targetID,
         this->pathInfo, this->nodeResult, this->counter, this->firstWriteDoneForTarget);

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
