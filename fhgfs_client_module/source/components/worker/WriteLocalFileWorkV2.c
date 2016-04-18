/*****
 *
 * OBSOLETE
 * OBSOLETE
 * OBSOLETE

*/
/*

#include <app/log/Logger.h>
#include <app/App.h>
#include <common/net/message/session/rw/WriteLocalFileMsg.h>
#include <common/net/message/session/rw/WriteLocalFileRespMsg.h>
#include <common/net/message/NetMessage.h>
#include <common/components/worker/WorkQueue.h>
#include <common/threading/Thread.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/VersionTk.h>
#include <nodes/NodeStoreEx.h>
#include "WriteLocalFileWorkV2.h"


void WriteLocalFileWorkV2_process(Work* this, char* bufIn, unsigned bufInLen,
   char* bufOut, unsigned bufOutLen)
{
   WriteLocalFileWorkV2* thisCast = (WriteLocalFileWorkV2*)this;

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
      int64_t commRes = __WriteLocalFileWorkV2_communicate(
         thisCast, node, bufIn, bufInLen, bufOut, bufOutLen);

      NodeStoreEx_releaseNode(nodes, &node);

      // handle retry
      if(unlikely(commRes == -FhgfsOpsErr_COMMUNICATION) )
      { // comm error occurred => prepare retry (if allowed)
         if(__WriteLocalFileWorkV2_prepareRetry(thisCast) )
         {
            // retry prepared => don't inc the done-counter and don't touch any shared variables
            return;
         }
      }
   }

   SynchronizedCounter_incCount(thisCast->counter);
}

int64_t __WriteLocalFileWorkV2_communicate(WriteLocalFileWorkV2* this, Node* node,
   char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen)
{
   const char* logContext = "WriteFileV2 Work (communication)";

   Logger* log = App_getLogger(this->app);
   Config* cfg = App_getConfig(this->app);

   Node* localNode = App_getLocalNode(this->app);
   char* localNodeID = Node_getID(localNode);

   NodeConnPool* connPool = Node_getConnPool(node);
   Socket* sock = NULL;
   fhgfs_bool connFailedLogged = fhgfs_false;

   WriteLocalFileMsg writeMsg;
   ssize_t sendRes;
   fhgfs_bool sendDataRes;
   WriteLocalFileRespMsg writeRespMsg;
   ssize_t respRes;
   fhgfs_bool deserRes;
   int64_t writeRespValue;

   *this->nodeResult = -FhgfsOpsErr_COMMUNICATION;

   // request init
   WriteLocalFileMsg_initFromSession(&writeMsg, localNodeID, this->fileHandleID, this->targetID,
      this->pathInfo, this->accessFlags, this->offset, this->size);

   if (Config_getQuotaEnabled(cfg) )
      WriteLocalFileMsg_setUserdataForQuota(&writeMsg, this->userID, this->groupID);

   if (this->firstWriteDoneForTarget)
      NetMessage_addMsgHeaderFeatureFlag( (NetMessage*)&writeMsg,
         WRITELOCALFILEMSG_FLAG_SESSION_CHECK);

   WriteLocalFileMsg_setMirrorToTargetID(&writeMsg, this->mirrorToTargetID);
   WriteLocalFileMsg_setDisableIO(&writeMsg, App_getNetBenchModeEnabled(this->app) );

   WriteLocalFileRespMsg_init(&writeRespMsg);

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
   NetMessage_serialize( (NetMessage*)&writeMsg, bufOut, bufOutLen);

   sendRes = sock->send(sock, bufOut, NetMessage_getMsgLength( (NetMessage*)&writeMsg), 0);
   if(unlikely(sendRes <= 0) )
   {
      Logger_logFormatted(log, 2, logContext,
         "Failed to send request to %s: %s", Node_getID(node), Socket_getPeername(sock) );
      goto loop_socketexception;
   }

   // send file data
   sendDataRes = WriteLocalFileMsg_sendData(&writeMsg, this->buf, sock);
   if(unlikely(!sendDataRes) )
   {
      Logger_logErrFormatted(log, logContext,
         "Communication error in SENDDATA stage. Node: %s", Node_getID(node));
      goto loop_socketexception;
   }

   // recv response
   respRes = MessagingTk_recvMsgBuf(this->app, sock, bufIn, bufInLen);
   if(unlikely(respRes <= 0) )
   { // error
      Logger_logFormatted(log, Log_WARNING, logContext,
         "Receive failed from: %s @ %s", Node_getID(node), Socket_getPeername(sock) );
      goto loop_socketexception;
   }

   // got response => deserialize it
   deserRes = NetMessageFactory_deserializeFromBuf(this->app, bufIn, respRes,
      (NetMessage*)&writeRespMsg, NETMSGTYPE_WriteLocalFileResp);
   if(unlikely(!deserRes) )
   { // response invalid
      Logger_logFormatted(log, Log_WARNING, logContext,
         "Received invalid response from %s. Expected type: %d. Disconnecting: %s",
         Node_getID(node), NETMSGTYPE_WriteLocalFileResp, Socket_getPeername(sock) );

      goto loop_socketinvalidate;
   }

   // correct response => store result value
   writeRespValue = WriteLocalFileRespMsg_getValue(&writeRespMsg);

   NodeConnPool_releaseStreamSocket(connPool, sock);

   *this->nodeResult = writeRespValue;

   goto loop_cleanup;


loop_socketexception:
   Logger_logErrFormatted(log, logContext,
      "Communication error. NodeID: %s", Node_getID(node));

   Logger_logFormatted(log, 4, logContext,
      "Request details: nodeID/fileHandleID/offset/size: %s/%s/%lld/%lld",
      Node_getID(node), this->fileHandleID, (long long)this->offset, (long long)this->size);

   *this->nodeResult = -FhgfsOpsErr_COMMUNICATION;


loop_socketinvalidate:
   NodeConnPool_invalidateStreamSocket(connPool, sock);


loop_cleanup:
   WriteLocalFileRespMsg_uninit( (NetMessage*)&writeRespMsg);
   WriteLocalFileMsg_uninit( (NetMessage*)&writeMsg);

   return *this->nodeResult;
}

*
 * @return fhgfs_true if a retry was prepared (e.g. caller shouldn't notify waiters), fhgfs_false
 * if no retry allowed and we're done

fhgfs_bool __WriteLocalFileWorkV2_prepareRetry(WriteLocalFileWorkV2* this)
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
      WriteLocalFileWorkV2* newWork;
      RetryableWork* newRetryWork;

      newWork = WriteLocalFileWorkV2_construct(this->app, this->buf,
         this->offset, this->size, this->fileHandleID, this->accessFlags, this->targetID,
         this->pathInfo, this->nodeResult, this->counter, this->firstWriteDoneForTarget);
      WriteLocalFileWorkV2_setMirrorToTargetID(newWork, this->mirrorToTargetID);
      WriteLocalFileWorkV2_setUserdataForQuota(newWork, this->userID, this->groupID);

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
*/
