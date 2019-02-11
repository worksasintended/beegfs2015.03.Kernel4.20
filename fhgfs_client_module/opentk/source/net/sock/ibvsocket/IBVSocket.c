#ifdef BEEGFS_OPENTK_IBVERBS

#ifdef KERNEL_HAS_SCSI_FC_COMPAT
#include <scsi/fc_compat.h> // some kernels (e.g. rhel 5.9) forgot this in their rdma headers
#endif // KERNEL_HAS_SCSI_FC_COMPAT


#include "IBVSocket.h"

#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/socket.h>

#define IBVSOCKET_CONN_TIMEOUT_MS         5000
#define IBVSOCKET_COMPLETION_TIMEOUT_MS   300000 /* this also includes send completion wait times */
#define IBVSOCKET_FLOWCONTROL_ONSEND_TIMEOUT_MS  180000
#define IBVSOCKET_FLOWCONTROL_MSG_LEN                 1
#define IBVSOCKET_FLOWCONTROL_ONRECV_TIMEOUT_MS  180000
#define IBVSOCKET_STALE_RETRIES_NUM                 128


void IBVSocket_init(IBVSocket* _this)
{
   fhgfs_bool createIDRes;

   memset(_this, 0, sizeof(*_this) );

   _this->sockValid = fhgfs_false;
   _this->connState = IBVSOCKETCONNSTATE_UNCONNECTED;

   _this->typeOfService = 0;

   init_waitqueue_head(&_this->eventWaitQ);

   createIDRes = __IBVSocket_createNewID(_this);
   if(!createIDRes)
      return;

   _this->sockValid = fhgfs_true;

   return;
}



IBVSocket* IBVSocket_construct()
{
   IBVSocket* _this = (IBVSocket*)kmalloc(sizeof(*_this), GFP_KERNEL);

   IBVSocket_init(_this);

   return _this;
}


void IBVSocket_uninit(IBVSocket* _this)
{
   __IBVSocket_close(_this);
}


void IBVSocket_destruct(IBVSocket* _this)
{
   IBVSocket_uninit(_this);

   kfree(_this);
}


fhgfs_bool IBVSocket_rdmaDevicesExist()
{
   // Note: We use this (currently) just to inform the higher levels
   //    about availability of RDMA functionality
   return fhgfs_true;
}



/**
 * Create a new cm_id.
 * This is not only intended for new sockets, but also for stale cm_ids, so this can cleanup/replace
 * existing cm_ids and resets error states.
 */
fhgfs_bool __IBVSocket_createNewID(IBVSocket* _this)
{
   struct rdma_cm_id* new_cm_id;

   //#if defined(OFED_HAS_NETNS)
      new_cm_id = rdma_create_id(&init_net, __IBVSocket_cmaHandler, _this, RDMA_PS_TCP, IB_QPT_RC);
   //#elif defined(OFED_HAS_RDMA_CREATE_QPTYPE)
   //   new_cm_id = rdma_create_id(__IBVSocket_cmaHandler, _this, RDMA_PS_TCP, IB_QPT_RC);
   //#else
   //   new_cm_id = rdma_create_id(__IBVSocket_cmaHandler, _this, RDMA_PS_TCP);
   //#endif

   if(IS_ERR(new_cm_id) )
   {
      printk_fhgfs(KERN_INFO, "%s:%d: rdma_create_id failed. ErrCode: %ld\n",
         __func__, __LINE__, PTR_ERR(new_cm_id) );

      return fhgfs_false;
   }

   if(_this->cm_id)
      rdma_destroy_id(_this->cm_id);

   _this->cm_id = new_cm_id;

   _this->connState = IBVSOCKETCONNSTATE_UNCONNECTED;
   _this->errState = 0;

   return fhgfs_true;
}

fhgfs_bool IBVSocket_connectByIP(IBVSocket* _this, struct in_addr* ipaddress, unsigned short port,
   IBVCommConfig* commCfg)
{
   struct sockaddr_in sin;
   fhgfs_bool stateChangeRes;

   /* note: rejected as stale means remote side still had an old open connection associated with
         our current cm_id. what most likely happened is that the client was reset (i.e. no clean
         disconnect) and our new cm_id after reboot now matches one of the old previous cm_ids.
         => only possible solution seems to be retrying with another cm_id. */
   int numStaleRetriesLeft = IBVSOCKET_STALE_RETRIES_NUM;


   for( ; ; ) // stale retry loop
   {
      // set type of service for this connection
      #ifdef OFED_HAS_SET_SERVICE_TYPE
         if (_this->typeOfService)
            rdma_set_service_type(_this->cm_id, _this->typeOfService);
      #endif // OFED_HAS_SET_SERVICE_TYPE

      /* note: the rest of the connect procedure is invoked through the cmaHandler when the
            corresponding asynchronous events arrive => we just have to wait for the connState
            to change here */

      __IBVSocket_setConnState(_this, IBVSOCKETCONNSTATE_CONNECTING);


      // resolve IP address ...
      // (async event handler also automatically resolves route on success)

      sin.sin_addr.s_addr = ipaddress->s_addr;
      sin.sin_family = AF_INET;
      sin.sin_port = htons(port);

      if(rdma_resolve_addr(_this->cm_id, NULL, (struct sockaddr*)&sin, IBVSOCKET_CONN_TIMEOUT_MS) )
      {
         printk_fhgfs_debug(KERN_INFO, "%d:%s: rdma_resolve_addr failed\n", __LINE__, __func__);
         goto err_invalidateSock;
      }

      // wait for async event
      stateChangeRes = __IBVSocket_waitForConnStateChange(_this, IBVSOCKETCONNSTATE_CONNECTING);
      if(!stateChangeRes ||
         (__IBVSocket_getConnState(_this) != IBVSOCKETCONNSTATE_ROUTERESOLVED) )
         goto err_invalidateSock;


      // establish connection...

      // (handler calls rdma_connect() )
      if(__IBVSocket_routeResolvedHandler(_this, _this->cm_id, commCfg, &_this->commContext) )
      {
         printk_fhgfs_debug(KERN_INFO, "%d:%s: route resolved handler failed\n",
            __LINE__, __func__);
         goto err_invalidateSock;
      }

      // wait for async event
      stateChangeRes = __IBVSocket_waitForConnStateChange(_this, IBVSOCKETCONNSTATE_ROUTERESOLVED);
      if(!stateChangeRes)
         goto err_invalidateSock;

      // check if cm_id was reported as stale by remote side
      if(__IBVSocket_getConnState(_this) == IBVSOCKETCONNSTATE_REJECTED_STALE)
      {
         fhgfs_bool createIDRes;

         if(!numStaleRetriesLeft)
         { // no more stale retries left
            if(IBVSOCKET_STALE_RETRIES_NUM) // did we have any retries at all
               printk_fhgfs(KERN_INFO, "Giving up after %d stale cm_id retries\n",
                  (int)IBVSOCKET_STALE_RETRIES_NUM);

            goto err_invalidateSock;
         }

         printk_fhgfs_connerr(KERN_INFO, "Stale cm_id detected. Retrying with a new one...\n");

         createIDRes = __IBVSocket_createNewID(_this);
         if(!createIDRes)
            goto err_invalidateSock;

         numStaleRetriesLeft--;
         continue;
      }

      if(__IBVSocket_getConnState(_this) != IBVSOCKETCONNSTATE_ESTABLISHED)
         goto err_invalidateSock;


      // connected

      if(numStaleRetriesLeft != IBVSOCKET_STALE_RETRIES_NUM)
      {
         printk_fhgfs_debug(KERN_INFO, "Succeeded after %d stale cm_id retries\n",
            (int)(IBVSOCKET_STALE_RETRIES_NUM - numStaleRetriesLeft) );
      }

      return fhgfs_true;
   }


err_invalidateSock:
   _this->errState = -1;

   return fhgfs_false;
}



/**
 * @return fhgfs_true on success
 */
fhgfs_bool IBVSocket_bind(IBVSocket* _this, unsigned short port)
{
   struct in_addr ipAddr;

   ipAddr.s_addr = INADDR_ANY;

   return IBVSocket_bindToAddr(_this, &ipAddr, port);
}


fhgfs_bool IBVSocket_bindToAddr(IBVSocket* _this, struct in_addr* ipAddr, unsigned short port)
{
   struct sockaddr_in bindAddr;

   bindAddr.sin_family = AF_INET;
   bindAddr.sin_addr = *ipAddr;
   bindAddr.sin_port = htons(port);

   if(rdma_bind_addr(_this->cm_id, (struct sockaddr*)&bindAddr) )
   {
      //printk_fhgfs_debug(KERN_INFO, "%s:%d rdma_bind_addr failed (port: %d)\n",
      //   __func__, __LINE__, (int)port); // debug in
      goto err_invalidateSock;
   }

   return fhgfs_true;


err_invalidateSock:
   _this->errState = -1;

   return fhgfs_false;
}


/**
 * @return fhgfs_true on success
 */
fhgfs_bool IBVSocket_listen(IBVSocket* _this)
{
   if(rdma_listen(_this->cm_id, 0) )
   {
      printk_fhgfs(KERN_INFO, "%d:%s: rdma_listen failed\n", __LINE__, __func__);
      goto err_invalidateSock;
   }

   return fhgfs_true;


err_invalidateSock:
   _this->errState = -1;

   return fhgfs_false;
}



fhgfs_bool IBVSocket_shutdown(IBVSocket* _this)
{
   IBVCommContext* commContext = _this->commContext;

   unsigned numWaitWrites = 0;
   unsigned numWaitReads = 0;
   int timeoutMS = 250;

   if(_this->errState)
      return fhgfs_true; // fhgfs_true, because the conn is down anyways

   if(!commContext)
      return fhgfs_true; // this socket has never been connected

   if(commContext->incompleteSend.numAvailable)
   { // wait for all incomplete sends
      int waitRes;

      waitRes = __IBVSocket_waitForTotalSendCompletion(_this,
         &commContext->incompleteSend.numAvailable, &numWaitWrites, &numWaitReads, timeoutMS);
      if(waitRes < 0)
      {
         printk_fhgfs_debug(KERN_INFO, "%s: Waiting for incomplete send requests failed\n",
            __func__);
         return fhgfs_false;
      }
   }

   //__IBVSocket_disconnect(_this, fhgfs_false); // note: read the method comments before using this

   return fhgfs_true;
}


/**
 * Continues an incomplete former recv() by returning immediately available data from the
 * corresponding buffer.
 */
ssize_t __IBVSocket_recvContinueIncomplete(IBVSocket* _this, char* buf, size_t bufLen)
{
   IBVCommContext* commContext = _this->commContext;
   int completedOffset = commContext->incompleteRecv.completedOffset;
   size_t availableLen = commContext->incompleteRecv.wc.byte_len - completedOffset;
   size_t bufIndex = commContext->incompleteRecv.wc.wr_id - IBVSOCKET_RECV_WORK_ID_OFFSET;

   if(unlikely(_this->errState) )
      return -1;

   if(availableLen <= bufLen)
   { // old data fits completely into buf
      int postRes;

      IGNORE_RETURN_VALUE(
         __copy_to_user(buf, &(commContext->recvBufs)[bufIndex][completedOffset], availableLen) );

      commContext->incompleteRecv.isAvailable = 0;

      postRes = __IBVSocket_postRecv(_this, _this->commContext, bufIndex);
      if(unlikely(postRes) )
         goto err_invalidateSock;

      return availableLen;
   }
   else
   { // still too much data for the buf => copy partially
      IGNORE_RETURN_VALUE(
         __copy_to_user(buf, &(commContext->recvBufs)[bufIndex][completedOffset], bufLen) );

      commContext->incompleteRecv.completedOffset += bufLen;

      return bufLen;
   }


err_invalidateSock:
   printk_fhgfs_debug(KERN_INFO, "%s:%d: invalidating connection\n", __func__, __LINE__);

   _this->errState = -1;
   return -1;
}


ssize_t IBVSocket_recv(IBVSocket* _this, char* buf, size_t bufLen, int flags)
{
   const int timeoutMS = 1024*1024;
   ssize_t recvTRes;

   do
   {
      recvTRes = IBVSocket_recvT(_this, buf, bufLen, flags, timeoutMS);
   } while(recvTRes == -ETIMEDOUT);

   return recvTRes;
}



/**
 * @return number of received bytes on success, -ETIMEDOUT on timeout, -ECOMM on error
 */
ssize_t IBVSocket_recvT(IBVSocket* _this, char* buf, size_t bufLen, int flags, int timeoutMS)
{
   IBVCommContext* commContext = _this->commContext;
   struct ib_wc* wc = &commContext->incompleteRecv.wc;
   int flowControlRes;
   int recvWCRes;

   if(unlikely(_this->errState) )
   {
      printk_fhgfs_debug(KERN_INFO, "%s:%d: called for an erroneous connection. ErrCode: %d\n",
         __func__, __LINE__, _this->errState);
      return -1;
   }

   // check whether an old buffer has not been fully read yet
   if(!commContext->incompleteRecv.isAvailable)
   { // no partially read data available => recv new buffer

      // check whether we have a pending on-send flow control packet that needs to be received first
      flowControlRes = __IBVSocket_flowControlOnSendWait(_this, timeoutMS);
      if(flowControlRes <= 0)
      {
         if(likely(!flowControlRes) )
            return -ETIMEDOUT; // timeout

         goto err_invalidateSock;
      }

      // recv a new buffer (into the incompleteRecv structure)
      recvWCRes = __IBVSocket_recvWC(_this, timeoutMS, wc);
      if(unlikely(recvWCRes <= 0) )
      {
         if(!recvWCRes)
            return -ETIMEDOUT; // timeout

         goto err_invalidateSock; // error occurred
      }

      // recvWC was positive => we're guaranteed to have an incompleteRecv buf availabe

      commContext->incompleteRecv.completedOffset = 0;
      commContext->incompleteRecv.isAvailable = 1;
   }

   return __IBVSocket_recvContinueIncomplete(_this, buf, bufLen);


err_invalidateSock:
   printk_fhgfs_debug(KERN_INFO, "%s:%d: invalidating connection\n", __func__, __LINE__);

   _this->errState = -1;
   return -ECOMM;
}


/**
 * @flags supports MSG_DONTWAIT
 * @return number of bytes sent or negative error code (-EAGAIN in case of MSG_DONTWAIT if no data
 * could be sent without blocking)
 */
ssize_t IBVSocket_send(IBVSocket* _this, const char* buf, size_t bufLen, int flags)
{
   IBVCommContext* commContext = _this->commContext;
   int flowControlRes;
   size_t currentBufIndex;
   int postRes;
   size_t postedLen = 0;
   int currentPostLen;
   int waitRes;

   unsigned numWaitWrites = 0;
   unsigned numWaitReads = 0;
   int timeoutMS = IBVSOCKET_COMPLETION_TIMEOUT_MS;

   if(unlikely(_this->errState) )
      return -1;

   // handle flags
   if(flags & MSG_DONTWAIT)
   { // send only as much as we can without blocking

      // note: we adapt the bufLen variable as necessary here for simplicity

      int checkSendRes;
      size_t bufNumLeft;
      size_t bufLenLeft;

      checkSendRes = IBVSocket_nonblockingSendCheck(_this);
      if(!checkSendRes)
      { // we can't send non-blocking at the moment, caller shall try again later
         return -EAGAIN;
      }
      else
      if(unlikely(checkSendRes < 0) )
         goto err_invalidateSock;

      // buffers available => adapt bufLen (if necessary)

      bufNumLeft = MIN(commContext->commCfg.bufNum - commContext->incompleteSend.numAvailable,
         commContext->numSendBufsLeft);
      bufLenLeft = bufNumLeft * commContext->commCfg.bufSize;

      bufLen = MIN(bufLen, bufLenLeft);
   }

   // send data cut in buf-sized pieces...

   do
   {
      flowControlRes = __IBVSocket_flowControlOnSendWait(_this,
         IBVSOCKET_FLOWCONTROL_ONSEND_TIMEOUT_MS);
      if(unlikely(flowControlRes <= 0) )
         goto err_invalidateSock;

      // note: we only poll for completed sends if forced or after we used up all (!) available bufs

      if(commContext->incompleteSend.forceWaitForAll ||
         (commContext->incompleteSend.numAvailable == commContext->commCfg.bufNum) )
      { // wait for all (!) incomplete sends
         waitRes = __IBVSocket_waitForTotalSendCompletion(_this,
            &commContext->incompleteSend.numAvailable, &numWaitWrites, &numWaitReads, timeoutMS);
         if(waitRes <= 0)
            goto err_invalidateSock;

         commContext->incompleteSend.forceWaitForAll = fhgfs_false;
      }


      currentPostLen = MIN(bufLen-postedLen, commContext->commCfg.bufSize);
      currentBufIndex = commContext->incompleteSend.numAvailable;

      IGNORE_RETURN_VALUE(
         __copy_from_user( (commContext->sendBufs)[currentBufIndex],
            &buf[postedLen], currentPostLen) );

      commContext->incompleteSend.numAvailable++; /* inc'ed before postSend() for conn checks */

      postRes = __IBVSocket_postSend(_this, currentBufIndex, currentPostLen);
      if(unlikely(postRes) )
      {
         commContext->incompleteSend.numAvailable--;
         goto err_invalidateSock;
      }


      postedLen += currentPostLen;

   } while(postedLen < bufLen);

   return (ssize_t)bufLen;


err_invalidateSock:
   _this->errState = -1;

   return -ECOMM;
}


void IBVSocket_setTypeOfService(IBVSocket* _this, int typeOfService)
{
   _this->typeOfService = typeOfService;
}



/**
 * Note: Use freeAndUnregisterBuf() to release the buffer and dma handle
 * @return kmalloc'ed buffer or NULL if no mem left
 */
char* __IBVSocket_allocAndRegisterBuf(IBVCommContext* commContext, size_t bufLen, u64* outBufDMA)
{
   return ib_dma_alloc_coherent(commContext->pd->device, bufLen, outBufDMA, GFP_KERNEL);
}

/**
 * Note: Use this to release buffers and dma handles that you got from allocAndRegisterBuf().
 */
void __IBVSocket_freeAndUnregisterBuf(IBVCommContext* commContext, size_t bufLen, void* buf,
   u64 bufDMA)
{
   ib_dma_free_coherent(commContext->pd->device, bufLen, buf, bufDMA);
}

fhgfs_bool __IBVSocket_createCommContext(IBVSocket* _this, struct rdma_cm_id* cm_id,
   IBVCommConfig* commCfg, IBVCommContext** outCommContext)
{
   IBVCommContext* commContext;
   struct ib_qp_init_attr qpInitAttr;
   int qpRes;
   unsigned i;

   commContext = (IBVCommContext*)kzalloc(sizeof(*commContext), GFP_KERNEL);
   if(!commContext)
      goto err_cleanup;

   // prepare recv and send event notification

   init_waitqueue_head(&commContext->recvCompWaitQ);
   init_waitqueue_head(&commContext->sendCompWaitQ);

   atomic_set(&commContext->recvCompEventCount, 0);
   atomic_set(&commContext->sendCompEventCount, 0);

   // protection domain...

#ifndef OFED_UNSAFE_GLOBAL_RKEY
   commContext->pd = ib_alloc_pd(cm_id->device);
#else
   commContext->pd = ib_alloc_pd(cm_id->device, IB_PD_UNSAFE_GLOBAL_RKEY);
#endif
   if(IS_ERR(commContext->pd) )
   {
      printk_fhgfs(KERN_INFO, "%s:%d: Couldn't allocate PD. ErrCode: %d\n", __func__, __LINE__,
         (int)PTR_ERR(commContext->pd) );

      commContext->pd = NULL;
      goto err_cleanup;
   }

   // DMA system mem region...

   // (Note: IB spec says:
   //    "The consumer is not allowed to assign remote-write (or remote-atomic) to
   //    a memory region that has not been assigned local-write.")

#ifndef OFED_UNSAFE_GLOBAL_RKEY
   commContext->dmaMR = ib_get_dma_mr(commContext->pd,
      IB_ACCESS_LOCAL_WRITE| IB_ACCESS_REMOTE_READ| IB_ACCESS_REMOTE_WRITE);
   if(IS_ERR(commContext->dmaMR) )
   {
      printk_fhgfs(KERN_INFO, "%s:%d: ib_get_dma_mr failed. ErrCode: %d\n", __func__, __LINE__,
         (int)PTR_ERR(commContext->dmaMR) );

      commContext->dmaMR = NULL;
      goto err_cleanup;
   }
#endif

   // alloc and register buffers...

   commContext->commCfg = *commCfg;

   commContext->recvBufs = (char**)kzalloc(commCfg->bufNum * sizeof(char*), GFP_KERNEL);
   commContext->recvBufsDMA = (u64*)kzalloc(commCfg->bufNum * sizeof(u64), GFP_KERNEL);

   for(i=0; i < commCfg->bufNum; i++)
   {
      commContext->recvBufs[i] = __IBVSocket_allocAndRegisterBuf(commContext, commCfg->bufSize,
         &commContext->recvBufsDMA[i] );
      if(!commContext->recvBufs[i])
      {
         printk_fhgfs(KERN_INFO, "%s:%d: Couldn't prepare recvBuf #%d\n", __func__, __LINE__, i+1);
         goto err_cleanup;
      }
   }

   commContext->sendBufs = (char**)kzalloc(commCfg->bufNum * sizeof(char*), GFP_KERNEL);
   commContext->sendBufsDMA = (u64*)kzalloc(commCfg->bufNum * sizeof(u64), GFP_KERNEL);

   for(i=0; i < commCfg->bufNum; i++)
   {
      commContext->sendBufs[i] = __IBVSocket_allocAndRegisterBuf(commContext, commCfg->bufSize,
         &commContext->sendBufsDMA[i] );
      if(!commContext->sendBufs[i])
      {
         printk_fhgfs(KERN_INFO, "%s:%d: Couldn't prepare sendBuf #%d\n", __func__, __LINE__, i+1);
         goto err_cleanup;
      }
   }

   commContext->numUsedSendBufsP = (uint64_t*)__IBVSocket_allocAndRegisterBuf(commContext,
      sizeof(*commContext->numUsedSendBufsP), &commContext->numUsedSendBufsDMA);
   if(!(commContext->numUsedSendBufsP) )
   {
      printk_fhgfs(KERN_INFO, "%s:%d: Couldn't alloc dma control memory region\n",
         __func__, __LINE__);
      goto err_cleanup;
   }

   commContext->numUsedSendBufsResetP = (uint64_t*)__IBVSocket_allocAndRegisterBuf(commContext,
      sizeof(*commContext->numUsedSendBufsResetP), &commContext->numUsedSendBufsResetDMA);
   if(!(commContext->numUsedSendBufsResetP) )
   {
      printk_fhgfs(KERN_INFO, "%s:%d: Couldn't alloc dma control memory reset region\n",
         __func__, __LINE__);
      goto err_cleanup;
   }

   // init flow control v2 (to avoid long receiver-not-ready timeouts)

   /* note: we use -1 because the last buf might not be read by the user (eg during
      nonblockingRecvCheck) and so it might not be immediately available again. */
   commContext->numReceivedBufsLeft = commCfg->bufNum - 1;
   commContext->numSendBufsLeft = commCfg->bufNum - 1;

   // create completion queues...

   commContext->recvCQ = __IBVSocket_createCompletionQueue(cm_id->device,
      __IBVSocket_recvCompletionHandler, __IBVSocket_cqRecvEventHandler,
      _this, commCfg->bufNum);
   if(IS_ERR(commContext->recvCQ) )
   {
      printk_fhgfs(KERN_INFO, "%s:%d: Couldn't create recv CQ. ErrCode: %d\n",
         __func__, __LINE__, (int)PTR_ERR(commContext->recvCQ) );

      commContext->recvCQ = NULL;
      goto err_cleanup;
   }

   // note: 1+commCfg->bufNum here for the checkConnection() RDMA read
   commContext->sendCQ = __IBVSocket_createCompletionQueue(cm_id->device,
      __IBVSocket_sendCompletionHandler, __IBVSocket_cqSendEventHandler,
      _this, 1+commCfg->bufNum);
   if(IS_ERR(commContext->sendCQ) )
   {
      printk_fhgfs(KERN_INFO, "%s:%d: Couldn't create send CQ. ErrCode: %d\n",
         __func__, __LINE__, (int)PTR_ERR(commContext->sendCQ) );

      commContext->sendCQ = NULL;
      goto err_cleanup;
   }

   // note: 1+commCfg->bufNum here for the checkConnection() RDMA read
   memset(&qpInitAttr, 0, sizeof(qpInitAttr) );

   qpInitAttr.event_handler = __IBVSocket_qpEventHandler;
   qpInitAttr.send_cq = commContext->sendCQ;
   qpInitAttr.recv_cq = commContext->recvCQ;
   qpInitAttr.qp_type = IB_QPT_RC;
   qpInitAttr.sq_sig_type = IB_SIGNAL_REQ_WR;
   qpInitAttr.cap.max_send_wr = 1+commCfg->bufNum;
   qpInitAttr.cap.max_recv_wr = commCfg->bufNum;
   qpInitAttr.cap.max_send_sge = 1;
   qpInitAttr.cap.max_recv_sge = 1;
   qpInitAttr.cap.max_inline_data = 0;

   qpRes = rdma_create_qp(cm_id, commContext->pd, &qpInitAttr);
   if(qpRes)
   {
      printk_fhgfs(KERN_INFO, "%s:%d: Couldn't create QP. ErrCode: %d\n",
         __func__, __LINE__, qpRes);
      goto err_cleanup;
   }

   commContext->qp = cm_id->qp;
   // atomic_set(&commContext->qp->usecnt, 0);

   // post initial recv buffers...

   for(i=0; i < commCfg->bufNum; i++)
   {
      if(__IBVSocket_postRecv(_this, commContext, i) )
      {
         printk_fhgfs(KERN_INFO, "%s:%d: Couldn't post recv buffer with index %d\n",
            __func__, __LINE__, i);
         goto err_cleanup;
      }
   }

   // prepare event notification...

   // initial event notification requests
   if(ib_req_notify_cq(commContext->recvCQ, IB_CQ_NEXT_COMP) )
   {
      printk_fhgfs(KERN_INFO, "%s:%d: Couldn't request CQ notification\n", __func__, __LINE__);
      goto err_cleanup;
   }

   if(ib_req_notify_cq(commContext->sendCQ, IB_CQ_NEXT_COMP) )
   {
      printk_fhgfs(KERN_INFO, "%s:%d: Couldn't request CQ notification\n", __func__, __LINE__);
      goto err_cleanup;
   }

   *outCommContext = commContext;
   return fhgfs_true;


   //  error handling

err_cleanup:
   __IBVSocket_cleanupCommContext(cm_id, commContext);

   *outCommContext = NULL;
   return fhgfs_false;
}

void __IBVSocket_cleanupCommContext(struct rdma_cm_id* cm_id, IBVCommContext* commContext)
{
   unsigned i;
   struct ib_device* dev;


   if(!commContext)
      return;

   dev = commContext->pd ? commContext->pd->device : NULL;

   if(!dev)
      goto cleanup_no_dev;

   if(cm_id && commContext->qp && cm_id->qp)
      rdma_destroy_qp(cm_id);


   if(commContext->sendCQ)
   {
      int destroyRes = ib_destroy_cq(commContext->sendCQ);
      if (unlikely(destroyRes) )
      {
         printk_fhgfs(KERN_WARNING, "%s: sendCQ destroy failed: %d\n", __func__, destroyRes);
         dump_stack();
      }
   }

   if(commContext->recvCQ)
   {
      int destroyRes = ib_destroy_cq(commContext->recvCQ);
      if (unlikely(destroyRes) )
      {
         printk_fhgfs(KERN_WARNING, "%s: recvCQ destroy failed: %d\n", __func__, destroyRes);
         dump_stack();
      }
   }

   if(commContext->numUsedSendBufsP)
   {
      __IBVSocket_freeAndUnregisterBuf(commContext, sizeof(*commContext->numUsedSendBufsP),
         commContext->numUsedSendBufsP, commContext->numUsedSendBufsDMA);
   }

   if(commContext->numUsedSendBufsResetP)
   {
      __IBVSocket_freeAndUnregisterBuf(commContext, sizeof(*commContext->numUsedSendBufsResetP),
         commContext->numUsedSendBufsResetP, commContext->numUsedSendBufsResetDMA);
   }

   for(i=0; i < commContext->commCfg.bufNum; i++)
   {
      if(commContext->recvBufs && commContext->recvBufs[i] )
      {
         __IBVSocket_freeAndUnregisterBuf(commContext, commContext->commCfg.bufSize,
            commContext->recvBufs[i], commContext->recvBufsDMA[i] );
      }

      if(commContext->sendBufs && commContext->sendBufs[i] )
      {
         __IBVSocket_freeAndUnregisterBuf(commContext, commContext->commCfg.bufSize,
            commContext->sendBufs[i], commContext->sendBufsDMA[i] );
      }
   }

   SAFE_KFREE(commContext->recvBufs);
   SAFE_KFREE(commContext->recvBufsDMA);
   SAFE_KFREE(commContext->sendBufs);
   SAFE_KFREE(commContext->sendBufsDMA);

#ifndef OFED_UNSAFE_GLOBAL_RKEY
   if(commContext->dmaMR)
      ib_dereg_mr(commContext->dmaMR);
#endif

   if(commContext->pd)
      ib_dealloc_pd(commContext->pd);


cleanup_no_dev:

   kfree(commContext);
}

/**
 * Initializes a (local) IBVCommDest.
 */
void __IBVSocket_initCommDest(IBVCommContext* commContext, IBVCommDest* outDest)
{
   memcpy(outDest->verificationStr, IBVSOCKET_PRIVATEDATA_STR, IBVSOCKET_PRIVATEDATA_STR_LEN);

   //outDest->protocolVersion = IBVSOCKET_PRIVATEDATA_PROTOCOL_VER;
   Serialization_serializeUInt64( (char*)&outDest->protocolVersion,
      IBVSOCKET_PRIVATEDATA_PROTOCOL_VER);

   //outDest->rkey = commContext->dmaMR->rkey;
#ifndef OFED_UNSAFE_GLOBAL_RKEY
   Serialization_serializeUInt( (char*)&outDest->rkey, commContext->dmaMR->rkey);
#else
   Serialization_serializeUInt( (char*)&outDest->rkey, commContext->pd->unsafe_global_rkey);
#endif

   //outDest->vaddr = commContext->numUsedSendBufsDMA;
   Serialization_serializeUInt64( (char*)&outDest->vaddr, commContext->numUsedSendBufsDMA);

   //outDest->recvBufNum = commContext->commCfg.bufNum;
   Serialization_serializeUInt( (char*)&outDest->recvBufNum, commContext->commCfg.bufNum);

   //outDest->recvBufSize = commContext->commCfg.bufSize;
   Serialization_serializeUInt( (char*)&outDest->recvBufSize, commContext->commCfg.bufSize);
}

/**
 * Parses and checks a (remote) IBVCommDest.
 *
 * @param buf should usually be the private_data of the connection handshake
 * @param outDest will be kmalloced (if fhgfs_true is returned) and needs to be kfree'd by the
 * caller
 * @return fhgfs_true if data is okay, fhgfs_false otherwise
 */
fhgfs_bool __IBVSocket_parseCommDest(const void* buf, size_t bufLen, IBVCommDest** outDest)
{
   IBVCommDest* dest = NULL;
   uint64_t destProtoVer;
   unsigned destProtoLen;
   unsigned destRKey;
   unsigned destRKeyLen;
   uint64_t destVAddr;
   unsigned destVAddrLen;
   unsigned destRecvBufNum;
   unsigned destRecvBufNumLen;
   unsigned destRecvBufSize;
   unsigned destRecvBufSizeLen;

   *outDest = NULL;


   // Note: "bufLen < ..." (and not "!="), because there might be some extra padding
   if(!buf || (bufLen < sizeof(*dest) ) )
   {
      printk_fhgfs(KERN_INFO, "%d:%s: Bad private data size. length: %d\n",
         __LINE__, __func__, (int)bufLen);

      return fhgfs_false;
   }

   dest = (IBVCommDest*)kmalloc(sizeof(*dest), GFP_ATOMIC);
   if(!dest)
      return fhgfs_false;

   memcpy(dest, buf, sizeof(*dest) );

   if(memcmp(dest->verificationStr, IBVSOCKET_PRIVATEDATA_STR, IBVSOCKET_PRIVATEDATA_STR_LEN) )
      goto err_cleanup;


   Serialization_deserializeUInt64(
      (const char*)&dest->protocolVersion, 8, &destProtoVer, &destProtoLen);

   dest->protocolVersion = destProtoVer; // replace with native-endian version

   if(destProtoVer != IBVSOCKET_PRIVATEDATA_PROTOCOL_VER)
      goto err_cleanup;


   Serialization_deserializeUInt(
      (const char*)&dest->rkey, 8L, &destRKey, &destRKeyLen);

   dest->rkey = destRKey; // replace with native-endian version

   Serialization_deserializeUInt64(
      (const char*)&dest->vaddr, 8L, &destVAddr, &destVAddrLen);

   dest->vaddr = destVAddr; // replace with native-endian version

   Serialization_deserializeUInt(
      (const char*)&dest->recvBufNum, 8L, &destRecvBufNum, &destRecvBufNumLen);

   dest->recvBufNum = destRecvBufNum; // replace with native-endian version

   Serialization_deserializeUInt(
      (const char*)&dest->recvBufSize, 8L, &destRecvBufSize, &destRecvBufSizeLen);

   dest->recvBufSize = destRecvBufSize; // replace with native-endian version


   *outDest = dest;

   return fhgfs_true;


err_cleanup:
   SAFE_KFREE(dest);

   return fhgfs_false;
}


/**
 * Append buffer to receive queue.
 *
 * @param commContext passed seperately because it's not the _this->commContext during
 *    accept() of incoming connections
 * @return 0 on success, -1 on error
 */
int __IBVSocket_postRecv(IBVSocket* _this, IBVCommContext* commContext, size_t bufIndex)
{
   struct ib_sge list;
   struct ib_recv_wr wr;
   const struct ib_recv_wr* bad_wr;
   int postRes;

   list.addr = (u64)commContext->recvBufsDMA[bufIndex];
   list.length = commContext->commCfg.bufSize;
#ifndef OFED_UNSAFE_GLOBAL_RKEY
   list.lkey = commContext->dmaMR->lkey;
#else
   list.lkey = commContext->pd->local_dma_lkey;
#endif

   wr.next = NULL;
   wr.wr_id = bufIndex + IBVSOCKET_RECV_WORK_ID_OFFSET;
   wr.sg_list = &list;
   wr.num_sge = 1;

   postRes = ib_post_recv(commContext->qp, &wr, &bad_wr);
   if(unlikely(postRes) )
   {
      printk_fhgfs(KERN_INFO, "%d:%s: ib_post_recv failed. ErrCode: %d\n",
         __LINE__, __func__, postRes);

      return -1;
   }

   return 0;
}

/**
 * Synchronous RDMA write (waits for completion).
 *
 * @param localBuf DMA pointer
 * @return 0 on success, -1 on error
 */
int __IBVSocket_postWrite(IBVSocket* _this, IBVCommDest* remoteDest, u64 localBuf, int bufLen)
{
   IBVCommContext* commContext = _this->commContext;
   struct ib_sge list;

   #ifdef OFED_SPLIT_WR
      # define rdma_of(wr) (wr)
      # define wr_of(wr) (wr.wr)
         struct ib_rdma_wr wr;
   #else
      # define rdma_of(wr) (wr.wr.rdma)
      # define wr_of(wr) (wr)
         struct ib_send_wr wr;
   #endif

   const struct ib_send_wr *bad_wr;
   int postRes;
   int waitRes;

   int timeoutMS = IBVSOCKET_COMPLETION_TIMEOUT_MS;
   unsigned numWaitWrites = 1;
   unsigned numWaitReads = 0;

   list.addr = (u64)localBuf;
   list.length = bufLen;
#ifndef OFED_UNSAFE_GLOBAL_RKEY
   list.lkey = commContext->dmaMR->lkey;
#else
   list.lkey = commContext->pd->local_dma_lkey;
#endif

   rdma_of(wr).remote_addr = remoteDest->vaddr;
   rdma_of(wr).rkey = remoteDest->rkey;

   wr_of(wr).wr_id      = 0;
   wr_of(wr).sg_list    = &list;
   wr_of(wr).num_sge    = 1;
   wr_of(wr).opcode     = IB_WR_RDMA_READ;
   wr_of(wr).send_flags = IB_SEND_SIGNALED;
   wr_of(wr).next       = NULL;

   postRes = ib_post_send(commContext->qp, &wr_of(wr), &bad_wr);
   if(unlikely(postRes) )
   {
      printk_fhgfs(KERN_INFO, "%d:%s: ib_post_send() failed. ErrCode: %d\n",
         __LINE__, __func__, postRes);

      return -1;
   }

   waitRes = __IBVSocket_waitForTotalSendCompletion(_this,
      &commContext->incompleteSend.numAvailable, &numWaitWrites, &numWaitReads, timeoutMS);
   if(unlikely(waitRes <= 0) )
      return -1;

   commContext->incompleteSend.forceWaitForAll = fhgfs_false;

   return 0;

   #undef rdma_of
   #undef wr_of
}

/**
 * Synchronous RDMA read (waits for completion).
 *
 * @param localBuf DMA pointer
 * @return 0 on success, -1 on error
 */
int __IBVSocket_postRead(IBVSocket* _this, IBVCommDest* remoteDest,
   u64 localBuf, int bufLen)
{
   IBVCommContext* commContext = _this->commContext;
   struct ib_sge list;

   #ifdef OFED_SPLIT_WR
      # define rdma_of(wr) (wr)
      # define wr_of(wr) (wr.wr)
         struct ib_rdma_wr wr;
   #else
      # define rdma_of(wr) (wr.wr.rdma)
      # define wr_of(wr) (wr)
         struct ib_send_wr wr;
   #endif

   const struct ib_send_wr *bad_wr;
   int postRes;
   int waitRes;

   int timeoutMS = IBVSOCKET_COMPLETION_TIMEOUT_MS;
   unsigned numWaitWrites = 0;
   unsigned numWaitReads = 1;

   list.addr = (u64)localBuf;
   list.length = bufLen;
#ifndef OFED_UNSAFE_GLOBAL_RKEY
   list.lkey = commContext->dmaMR->lkey;
#else
   list.lkey = commContext->pd->local_dma_lkey;
#endif

   rdma_of(wr).remote_addr = remoteDest->vaddr;
   rdma_of(wr).rkey = remoteDest->rkey;

   wr_of(wr).wr_id      = IBVSOCKET_READ_WORK_ID;
   wr_of(wr).sg_list    = &list;
   wr_of(wr).num_sge    = 1;
   wr_of(wr).opcode     = IB_WR_RDMA_READ;
   wr_of(wr).send_flags = IB_SEND_SIGNALED;
   wr_of(wr).next       = NULL;

   postRes = ib_post_send(commContext->qp, &wr_of(wr), &bad_wr);
   if(unlikely(postRes) )
   {
      printk_fhgfs(KERN_INFO, "%d:%s: ib_post_send() failed. ErrCode: %d\n",
         __LINE__, __func__, postRes);

      return -1;
   }

   waitRes = __IBVSocket_waitForTotalSendCompletion(_this,
      &commContext->incompleteSend.numAvailable, &numWaitWrites, &numWaitReads, timeoutMS);
   if(unlikely(waitRes <= 0) )
      return -1;

   commContext->incompleteSend.forceWaitForAll = fhgfs_false;

   return 0;
}

/**
 * Note: contains flow control.
 *
 * @return 0 on success, -1 on error
 */
int __IBVSocket_postSend(IBVSocket* _this, size_t bufIndex, int bufLen)
{
   IBVCommContext* commContext = _this->commContext;
   struct ib_sge list;
   struct ib_send_wr wr;
   const struct ib_send_wr *bad_wr;
   int postRes;

   list.addr   = (u64)commContext->sendBufsDMA[bufIndex];
   list.length = bufLen;
#ifndef OFED_UNSAFE_GLOBAL_RKEY
   list.lkey = commContext->dmaMR->lkey;
#else
   list.lkey = commContext->pd->local_dma_lkey;
#endif

   wr.wr_id      = bufIndex + IBVSOCKET_SEND_WORK_ID_OFFSET;
   wr.sg_list    = &list;
   wr.num_sge    = 1;
   wr.opcode     = IB_WR_SEND;
   wr.send_flags = IB_SEND_SIGNALED;
   wr.next       = NULL;

   postRes = ib_post_send(commContext->qp, &wr, &bad_wr);
   if(unlikely(postRes) )
   {
      printk_fhgfs(KERN_INFO, "%d:%s: ib_post_send() failed. ErrCode: %d\n",
         __LINE__, __func__, postRes);

      return -1;
   }

   // flow control
   __IBVSocket_flowControlOnSendUpdateCounters(_this);

   return 0;
}


/**
 * Receive work completion.
 *
 * Note: contains flow control.
 *
 * @param timeoutMS 0 for non-blocking
 * @return 1 on success, 0 on timeout, <0 on error
 */
int __IBVSocket_recvWC(IBVSocket* _this, int timeoutMS, struct ib_wc* outWC)
{
   IBVCommContext* commContext = _this->commContext;
   int waitRes;
   size_t bufIndex;

   waitRes = __IBVSocket_waitForRecvCompletionEvent(_this, timeoutMS, outWC);
   if(waitRes <= 0)
   { // (note: waitRes==0 can often happen, because we call this with timeoutMS==0)

      if(unlikely(waitRes < 0) )
      {
         if(waitRes != -ERESTARTSYS)
         { // only print message if user didn't press "<CTRL>-C"
            printk_fhgfs(KERN_INFO, "%s:%d: retrieval of completion event failed. result: %d\n",
               __func__, __LINE__, waitRes);
         }
      }
      else
      if(unlikely(timeoutMS) )
      {
         //printk_fhgfs_debug(KERN_INFO, "%s:%d: waiting for recv completion timed out (%d ms).\n",
         //   __func__, __LINE__, timeoutMS); // debug in
      }

      return waitRes;
   }

   // we got something...

   if(unlikely(outWC->status != IB_WC_SUCCESS) )
   {
      printk_fhgfs_connerr(KERN_INFO, "%s: Connection error (wc_status: %d; msg: %s)\n",
         "IBVSocket (recv work completion)", (int)outWC->status,
         __IBVSocket_wcStatusStr(outWC->status) );
      return -1;
   }

   bufIndex = outWC->wr_id - IBVSOCKET_RECV_WORK_ID_OFFSET;

   if(unlikely(bufIndex >= commContext->commCfg.bufNum) )
   {
      printk_fhgfs(KERN_INFO, "%s: Completion for unknown/invalid wr_id %d\n",
         __func__, (int)outWC->wr_id);
      return -1;
   }

   // receive completed

   //printk_fhgfs(KERN_INFO, "%s: Recveived %u bytes.\n", __func__, outWC->byte_len); // debug in

   // flow control

   if(unlikely(__IBVSocket_flowControlOnRecv(_this, IBVSOCKET_FLOWCONTROL_ONRECV_TIMEOUT_MS) ) )
   {
      printk_fhgfs_debug(KERN_INFO, "%s:%d: got an error from flowControlOnRecv().\n",
         __func__, __LINE__);
      return -1;
   }

   return 1;
}

/**
 * Intention: Avoid IB rnr by sending control msg when (almost) all our recv bufs are used up to
 * show that we got our new recv bufs ready.
 *
 * @timeoutMS don't set this to 0, we really need to wait here sometimes
 * @return 0 on success, -1 on error
 */
int __IBVSocket_flowControlOnRecv(IBVSocket* _this, int timeoutMS)
{
   IBVCommContext* commContext = _this->commContext;

   // we received a packet, so peer has received all of our currently pending data => reset counter
   commContext->numSendBufsLeft = commContext->commCfg.bufNum - 1; /* (see
      createCommContext() for "-1" reason) */

   // send control packet if recv counter expires...

   #ifdef BEEGFS_DEBUG
      if(!commContext->numReceivedBufsLeft)
         printk_fhgfs(KERN_INFO, "%s: BUG: numReceivedBufsLeft underflow!\n", __func__);
   #endif // BEEGFS_DEBUG

   commContext->numReceivedBufsLeft--;

   if(!commContext->numReceivedBufsLeft)
   {
      size_t currentBufIndex;
      int postRes;

      if(commContext->incompleteSend.forceWaitForAll ||
         (commContext->incompleteSend.numAvailable == commContext->commCfg.bufNum) )
      { // wait for all (!) incomplete sends

         /* note: it's ok that all send bufs are used up, because it's possible that we do a lot of
            recv without the user sending any data in between (so the bufs were actually used up by
            flow control). */
         unsigned numWaitWrites = 0;
         unsigned numWaitReads = 0;

         int waitRes = __IBVSocket_waitForTotalSendCompletion(_this,
            &commContext->incompleteSend.numAvailable, &numWaitWrites, &numWaitReads, timeoutMS);
         if(waitRes <= 0)
         {
            printk_fhgfs(KERN_INFO, "%s:%d: problem encountered during "
               "waitForTotalSendCompletion(). ErrCode: %d\n", __func__, __LINE__, waitRes);
            return -1;
         }

         commContext->incompleteSend.forceWaitForAll = fhgfs_false;
      }

      currentBufIndex = commContext->incompleteSend.numAvailable;

      commContext->incompleteSend.numAvailable++; /* inc'ed before postSend() for conn checks */

      postRes = __IBVSocket_postSend(_this, currentBufIndex, IBVSOCKET_FLOWCONTROL_MSG_LEN);
      if(unlikely(postRes) )
      {
         commContext->incompleteSend.numAvailable--;
         return -1;
      }


      // note: numReceivedBufsLeft is reset during postSend() flow control
   }

   return 0;
}

/**
 * Called after sending a packet to update flow control counters.
 *
 * Intention: Avoid IB rnr by waiting for control msg when (almost) all peer bufs are used up.
 *
 * Note: This is only one part of the on-send flow control. The other one is
 * _flowControlOnSendWait().
 */
void __IBVSocket_flowControlOnSendUpdateCounters(IBVSocket* _this)
{
   IBVCommContext* commContext = _this->commContext;

   // we sent a packet, so we received all currently pending data from the peer => reset counter
   commContext->numReceivedBufsLeft = commContext->commCfg.bufNum - 1; /* (see
      createCommContext() for "-1" reason) */

   #ifdef BEEGFS_DEBUG

   if(!commContext->numSendBufsLeft)
      printk_fhgfs(KERN_INFO, "%s: BUG: numSendBufsLeft underflow!\n", __func__);

   #endif

   commContext->numSendBufsLeft--;
}

/**
 * Intention: Avoid IB rnr by waiting for control msg when (almost) all peer bufs are used up.
 *
 * @timeoutMS may be 0 for non-blocking operation, otherwise typically
 * IBVSOCKET_FLOWCONTROL_ONSEND_TIMEOUT_MS
 * @return >0 on success, 0 on timeout (waiting for flow control packet from peer), <0 on error
 */
int __IBVSocket_flowControlOnSendWait(IBVSocket* _this, int timeoutMS)
{
   IBVCommContext* commContext = _this->commContext;

   struct ib_wc wc;
   int recvRes;
   size_t bufIndex;
   int postRecvRes;

   if(commContext->numSendBufsLeft)
      return 1; // flow control not triggered yet

   recvRes = __IBVSocket_recvWC(_this, timeoutMS, &wc);
   if(recvRes <= 0)
      return recvRes;

   bufIndex = wc.wr_id - IBVSOCKET_RECV_WORK_ID_OFFSET;

   if(unlikely(wc.byte_len != IBVSOCKET_FLOWCONTROL_MSG_LEN) )
   { // error (bad length)
      printk_fhgfs(KERN_INFO, "%s: received flow control packet length mismatch %d\n", __func__,
         (int)wc.byte_len);
      return -1;
   }

   postRecvRes = __IBVSocket_postRecv(_this, _this->commContext, bufIndex);
   if(postRecvRes)
      return -1;

   // note: numSendBufsLeft is reset during recvWC() (if it actually received a packet)

   return 1;
}


/**
 * @return 1 on available data, 0 on timeout, <0 on error
 */
int __IBVSocket_waitForRecvCompletionEvent(IBVSocket* _this, int timeoutMS, struct ib_wc* outWC)
{
   IBVCommContext* commContext = _this->commContext;
   long waitRes;
   int numEvents = 0;
   int checkRes;

   // special quick path: other than in the userspace version of this method, we only need the
   //    quick path when timeoutMS==0, because then we might have been called from a special
   //    context, in which we don't want to sleep
   if(!timeoutMS)
      return ib_poll_cq(commContext->recvCQ, 1, outWC);


   for( ; ; )
   {
      /* note: we use pollTimeoutMS to check the conn every few secs (otherwise we might
         wait for a very long time in case the other side disconnected silently) */

      int pollTimeoutMS = MIN(10000, timeoutMS);
      long pollTimeoutJiffies = __IBVSocket_msToJiffiesSchedulable(pollTimeoutMS);

      /* note: don't think about ib_peek_cq here, because it is not implemented in the drivers. */

      waitRes = wait_event_interruptible_timeout(commContext->recvCompWaitQ,
         (numEvents = ib_poll_cq(commContext->recvCQ, 1, outWC) ), pollTimeoutJiffies);

      if(unlikely(waitRes == -ERESTARTSYS) )
      { // signal pending
         printk_fhgfs_debug(KERN_INFO, "%s:%d: wait for recvCompEvent ended by pending signal\n",
            __func__, __LINE__);

         return waitRes;
      }

      if(likely(numEvents) )
      { // we got something
         return numEvents;
      }

      // timeout

      checkRes = IBVSocket_checkConnection(_this);
      if(checkRes < 0)
         return -ECONNRESET;

      timeoutMS -= pollTimeoutMS;
      if(!timeoutMS)
         return 0;

   } // end of for-loop

}


/**
 * @param oldSendCount old sendCompEventCount
 * @return 1 on available data, 0 on timeout, -1 on error
 */
int __IBVSocket_waitForSendCompletionEvent(IBVSocket* _this, int oldSendCount, int timeoutMS)
{
   IBVCommContext* commContext = _this->commContext;
   long waitRes;

   for( ; ; )
   {
      // Note: We use pollTimeoutMS to check the conn every few secs (otherwise we might
      //    wait for a very long time in case the other side disconnected silently)
      int pollTimeoutMS = MIN(10000, timeoutMS);
      long pollTimeoutJiffies = __IBVSocket_msToJiffiesSchedulable(pollTimeoutMS);

      waitRes = wait_event_interruptible_timeout(commContext->sendCompWaitQ,
         atomic_read(&commContext->sendCompEventCount) != oldSendCount, pollTimeoutJiffies);

      if(unlikely(waitRes == -ERESTARTSYS) )
      { // signal pending
         printk_fhgfs_debug(KERN_INFO, "%s:%d: wait for sendCompEvent ended by pending signal\n",
            __func__, __LINE__);

         return -1;
      }

      if(unlikely(atomic_read(&commContext->sendCompEventCount) == oldSendCount) )
      { // timeout
         //printk_fhgfs(KERN_INFO "%s: wait timed out\n", __func__); // debug in

         timeoutMS -= pollTimeoutMS;
         if(!timeoutMS)
            return 0;

         continue;
      }
      else
      {
         //printk_fhgfs(KERN_INFO "%s:%d: received a completion event\n",
         //    __func__, __LINE__); // debug in

         return 1;
      }

   } // end of for-loop
}


/**
 * @param numSendElements also used as out-param to return the remaining number
 * @param timeoutMS 0 for non-blocking; this is a soft timeout that is reset after each received
 * completion
 * @return 1 if all completions received, 0 if completions missing (in case you wanted non-blocking)
 * or -1 in case of an error.
 */
int __IBVSocket_waitForTotalSendCompletion(IBVSocket* _this,
   unsigned* numSendElements, unsigned* numWriteElements, unsigned* numReadElements, int timeoutMS)
{
   IBVCommContext* commContext = _this->commContext;
   int numElements;
   int waitRes;
   int oldSendCount;
   int i;
   size_t bufIndex;
   struct ib_wc wc[2];

   do
   {
      oldSendCount = atomic_read(&commContext->sendCompEventCount);

      numElements = ib_poll_cq(commContext->sendCQ, 2, wc);
      if(unlikely(numElements < 0) )
      {
         printk_fhgfs(KERN_INFO, "%s:%d: bad ib_poll_cq result: %d\n", __func__, __LINE__,
            numElements);

         return -1;
      }
      else
      if(!numElements)
      { // no completions available yet => wait
         if(!timeoutMS)
            return 0;

         waitRes = __IBVSocket_waitForSendCompletionEvent(_this, oldSendCount, timeoutMS);
         if(likely(waitRes > 0) )
            continue;

         return waitRes;
      }

      // we got something...

      // for each completion element
      for(i=0; i < numElements; i++)
      {
         if(unlikely(wc[i].status != IB_WC_SUCCESS) )
         {
            printk_fhgfs_connerr(KERN_INFO, "%s: Connection error (wc_status: %d; msg: %s)\n",
               "IBVSocket (wait for total send completion)", (int)(wc[i].status),
               __IBVSocket_wcStatusStr(wc[i].status) );

            return -1;
         }

         switch(wc[i].opcode)
         {
            case IB_WC_SEND:
            {
               bufIndex = wc[i].wr_id - IBVSOCKET_SEND_WORK_ID_OFFSET;

               if(unlikely(bufIndex >= commContext->commCfg.bufNum) )
               {
                  printk_fhgfs(KERN_INFO, "%d:%s: bad send completion wr_id 0x%x\n", __LINE__,
                     __func__, (int)wc[i].wr_id);

                  return -1;
               }

               if(likely(*numSendElements) )
                  (*numSendElements)--;
               else
               {
                  printk_fhgfs(KERN_INFO, "%s:%d: received bad/unexpected send completion\n",
                     __func__, __LINE__);

                  return -1;
               }

            } break;

            case IB_WC_RDMA_WRITE:
            {
               if(unlikely(wc[i].wr_id != IBVSOCKET_WRITE_WORK_ID) )
               {
                  printk_fhgfs(KERN_INFO, "%d:%s: bad write completion wr_id 0x%x\n",
                     __LINE__, __func__, (int)wc[i].wr_id);

                  return -1;
               }

               if(likely(*numWriteElements) )
                  (*numWriteElements)--;
               else
               {
                  printk_fhgfs(KERN_INFO, "%s:%d: received bad/unexpected RDMA write completion\n",
                     __func__, __LINE__);

                  return -1;
               }
            } break;

            case IB_WC_RDMA_READ:
            {
               if(unlikely(wc[i].wr_id != IBVSOCKET_READ_WORK_ID) )
               {
                  printk_fhgfs(KERN_INFO, "%d:%s: bad read completion wr_id 0x%x\n", __LINE__,
                     __func__, (int)wc[i].wr_id);

                  return -1;
               }

               if(likely(*numReadElements) )
                  (*numReadElements)--;
               else
               {
                  printk_fhgfs(KERN_INFO, "%s:%d: received bad/unexpected RDMA read completion\n",
                     __func__, __LINE__);

                  return -1;
               }
            } break;

            default:
            {
               printk_fhgfs(KERN_INFO, "%s:%d: received bad/unexpected completion opcode %d\n",
                  __func__, __LINE__, wc[i].opcode);

               return -1;
            } break;

         } // end of switch

      } // end of for-loop

   } while(*numSendElements || *numWriteElements || *numReadElements);

   return 1;
}

/**
 * @return 0 on success, -1 on error
 */
int IBVSocket_checkConnection(IBVSocket* _this)
{
   int postRes;
   IBVCommContext* commContext = _this->commContext;

   if(unlikely(_this->errState) )
      return -1;

   // note: we read a remote value into the numUsedSendBufsReset field, which is actually
   //    meant for something else, so we need to reset the value afterwards

   //printk_fhgfs_debug(KERN_INFO, "%d:%s: post rdma_read to check connection...\n",
   //   __LINE__, __func__); // debug in

   postRes = __IBVSocket_postRead(_this, _this->remoteDest,
      commContext->numUsedSendBufsResetDMA, sizeof(*commContext->numUsedSendBufsResetP) );
   if(unlikely(postRes) )
   {
      _this->errState = -1;
      return -1;
   }

   *commContext->numUsedSendBufsResetP = 0;

   //printk_fhgfs_debug(KERN_INFO, "%d:%s: rdma_read succeeded\n", __LINE__, __func__); // debug in

   return 0;
}


/**
 * @return <0 on error, 0 if recv would block, >0 if recv would not block
 */
int IBVSocket_nonblockingRecvCheck(IBVSocket* _this)
{
   IBVCommContext* commContext = _this->commContext;
   struct ib_wc* wc = &commContext->incompleteRecv.wc;
   int flowControlRes;
   int recvRes;

   if(unlikely(_this->errState) )
      return -1;

   if(commContext->incompleteRecv.isAvailable)
      return 1;

   // check whether we have a pending on-send flow control packet that needs to be received first
   flowControlRes = __IBVSocket_flowControlOnSendWait(_this, 0);
   if(unlikely(flowControlRes < 0) )
   {
      printk_fhgfs_debug(KERN_INFO, "%s:%d: got an error from flowControlOnSendWait(). "
         "ErrCode: %d\n", __func__, __LINE__, flowControlRes);
      goto err_invalidateSock;
   }

   if(!flowControlRes)
      return 0;

   // recv one packet (if available) and add it as incompleteRecv
   recvRes = __IBVSocket_recvWC(_this, 0, wc);
   if(unlikely(recvRes < 0) )
   {
      printk_fhgfs_debug(KERN_INFO, "%s:%d: got an error from __IBVSocket_recvWC(). "
         "ErrCode: %d\n", __func__, __LINE__, recvRes);
      goto err_invalidateSock;
   }

   if(!recvRes)
      return 0;

   // we got something => prepare to continue later

   commContext->incompleteRecv.completedOffset = 0;
   commContext->incompleteRecv.isAvailable = 1;

   return 1;


err_invalidateSock:
   printk_fhgfs_debug(KERN_INFO, "%s:%d: invalidating connection\n", __func__, __LINE__);

   _this->errState = -1;
   return -1;
}


/**
 * @return <0 on error, 0 if send would block, >0 if send would not block
 */
int IBVSocket_nonblockingSendCheck(IBVSocket* _this)
{
   IBVCommContext* commContext = _this->commContext;
   int flowControlRes;
   int waitRes;

   unsigned numWaitWrites = 0;
   unsigned numWaitReads = 0;
   int timeoutMS = 0;

   if(unlikely(_this->errState) )
      return -1;

   // check whether we have a pending on-send flow control packet that needs to be received first
   flowControlRes = __IBVSocket_flowControlOnSendWait(_this, 0);
   if(unlikely(flowControlRes < 0) )
      goto err_invalidateSock;

   if(!flowControlRes)
      return flowControlRes;

   if(!commContext->incompleteSend.forceWaitForAll &&
      (commContext->incompleteSend.numAvailable < commContext->commCfg.bufNum) )
      return 1;

   commContext->incompleteSend.forceWaitForAll = fhgfs_true; // always setting saves an "if" below

   // we have to wait for completions before we can send...

   waitRes = __IBVSocket_waitForTotalSendCompletion(_this,
      &commContext->incompleteSend.numAvailable, &numWaitWrites, &numWaitReads, timeoutMS);
   if(unlikely(waitRes < 0) )
      goto err_invalidateSock;

   if(waitRes > 0)
      commContext->incompleteSend.forceWaitForAll = fhgfs_false; // no more completions peding

   return waitRes;

err_invalidateSock:
   printk_fhgfs_debug(KERN_INFO, "%s:%d: invalidating connection\n", __func__, __LINE__);

   _this->errState = -1;
   return -1;
}


/**
 * Note: Call this only once with finishPoll==fhgfs_true (=> non-blocking) or multiple times with
 *    finishPoll==fhgfs_true in the last call from the current thread (for cleanup).
 * Note: It's safe to call this multiple times with finishPoll==fhgfs_true (in case that caller does
 *    not want to sleep anymore).
 *
 * @param events the event flags you are interested in (POLL...)
 * @param finishPoll true for cleanup if you don't call poll again from this thread; (it's also ok
 *    to set this to true if you call poll only once and want to avoid blocking)
 * @return mask all available revents (like poll(), POLL... flags), may not only be the events
 * that were requested (and can also be the error events)
 */
unsigned long IBVSocket_poll(IBVSocket* _this, short events, fhgfs_bool finishPoll)
{
   /* note: there are two possible uses for finishPoll==fhgfs_true:
         1) this method is called multiple times and finishPoll is fhgfs_true in the last loop
            (for cleanup)
         2) this method is called only once non-blocking and finishPoll is fhgfs_true to avoid
            blocking */

   /* note: it's good to call prepare_to_wait more than once for the same thread (e.g. if
      caller woke up from schedule() and decides to sleep again) */

   /* note: condition needs to be re-checked after prepare_to_wait to avoid the race when it became
      true between the initial check and the call to prepare_to_wait */

   /* note: we assume that after we returned a positive result, the caller will not try to sleep
      (but will still call this again with finishPoll==fhgfs_true to cleanup) */


   IBVCommContext* commContext = _this->commContext;
   unsigned long revents = 0; // return value


   if(unlikely(_this->errState) )
   {
      printk_fhgfs_debug(KERN_INFO, "%s:%d: called for an erroneous connection. ErrCode: %d\n",
         __func__, __LINE__, _this->errState);

      revents |= POLLERR;
      return revents;
   }

   if(!commContext->numSendBufsLeft)
   { /* special case: on-send flow control triggered, so we actually need to wait for incoming data
        even though the user set POLLOUT */

      events |= POLLIN;

      /* note: the actual checks for POLLIN will be handled below like in the normal POLLIN case.
         (we just want need to wake up when there is incoming data.) */

      /* note: this only works efficiently, because the beegfs client just checks for any revent and
         then calls _send() again. checking for POLLOUT explicitly would result in another call to
         _poll() and then we would notice that we can send now => would work, but is less efficient,
         obviously. */
   }


   /* note: the "if(POLLIN || recvWaitInitialized)" is necessary because !numSendBufsLeft might have
      triggered unrequested POLLIN check and we need to clean that up later. */

   if( (events & POLLIN) || (commContext->recvWaitInitialized) )
   {
      // initial check and wait preparations for incoming data

      if(IBVSocket_nonblockingRecvCheck(_this) )
      { // immediate data available => no need to prepare wait
         revents |= POLLIN;
      }
      else
      if(!finishPoll && !commContext->recvWaitInitialized)
      { // no incoming data and caller is planning to schedule()
         #ifdef BEEGFS_DEBUG
            if(waitqueue_active(&commContext->recvCompWaitQ) )
               printk_fhgfs(KERN_INFO, "%s:%d: BUG: recvCompWaitQ was not empty\n",
                  __func__, __LINE__);
         #endif // BEEGFS_DEBUG

         commContext->recvWaitInitialized = fhgfs_true;

         init_waitqueue_entry(&commContext->recvWait, current);
         add_wait_queue(&commContext->recvCompWaitQ, &commContext->recvWait);

         if(IBVSocket_nonblockingRecvCheck(_this) )
            revents |= POLLIN;
      }

      // cleanup

      if(finishPoll && commContext->recvWaitInitialized)
      {
         commContext->recvWaitInitialized = fhgfs_false;

         remove_wait_queue(&commContext->recvCompWaitQ, &commContext->recvWait);
      }
   }

   /* note: POLLOUT check must come _after_ POLLIN check, because the pollin-part
      won't set the POLLOUT flag if we recv the on-send flow ctl packet during
      nonblockingRecvCheck(). */

   if(events & POLLOUT)
   {
      // initial check and wait preparations for outgoing data

      if(IBVSocket_nonblockingSendCheck(_this) )
      { // immediate data available => no need to prepare wait
         revents |= POLLOUT;
      }
      else
      if(!finishPoll && !commContext->sendWaitInitialized)
      { // no incoming data and caller is planning to schedule()
         #ifdef BEEGFS_DEBUG
            if(waitqueue_active(&commContext->sendCompWaitQ) )
               printk_fhgfs(KERN_INFO, "%s:%d: BUG: sendCompWaitQ was not empty\n",
                  __func__, __LINE__);
         #endif // BEEGFS_DEBUG

         commContext->sendWaitInitialized = fhgfs_true;

         init_waitqueue_entry(&commContext->sendWait, current);
         add_wait_queue(&commContext->sendCompWaitQ, &commContext->sendWait);

         if(IBVSocket_nonblockingSendCheck(_this) )
            revents |= POLLOUT;
      }

      // cleanup

      if(finishPoll && commContext->sendWaitInitialized)
      {
         commContext->sendWaitInitialized = fhgfs_false;

         remove_wait_queue(&commContext->sendCompWaitQ, &commContext->sendWait);
      }
   }

   // check errState again in case it was modified during the checks above
   if(unlikely(_this->errState) )
   {
      printk_fhgfs_debug(KERN_INFO, "%s:%d: got erroneous connection state. ErrCode: %d\n",
         __func__, __LINE__, _this->errState);

      revents |= POLLERR;
   }

   return revents;
}


fhgfs_bool IBVSocket_getSockValid(IBVSocket* _this)
{
   return _this->sockValid;
}


/**
 * Note: this is provided simply because it exists, but somehow the connection tear-down process
 *    often works better (without delay) if this explicit disconnect is avoided. In fact, it seems
 *    a receiver might sometimes even hang infinitely if this is used.
 */
void __IBVSocket_disconnect(IBVSocket* _this, fhgfs_bool waitForEvent)
{
   int disconnectRes;

   disconnectRes = rdma_disconnect(_this->cm_id);
   if(disconnectRes)
   {
      printk_fhgfs(KERN_INFO, "%s:%d: rdma disconnect error\n", __func__, __LINE__);

      return;
   }

}

void __IBVSocket_close(IBVSocket* _this)
{
   SAFE_KFREE(_this->remoteDest);

   if(_this->commContext)
      __IBVSocket_cleanupCommContext(_this->cm_id, _this->commContext);

   if(_this->cm_id)
      rdma_destroy_id(_this->cm_id);
}


/**
 * Handle connection manager event callbacks from interrupt handler.
 *
 * Note: Sleeping (e.g. mutex locking) is not allowed in callbacks like this one!
 *
 * @return negative Linux error code on error, 0 otherwise; in case of return!=0, rdma_cm will
 * automatically call rdma_destroy_id(), so there are some errors for which we return 0, because we
 * want to cleanup cm_id ourselves later.
 */
int __IBVSocket_cmaHandler(struct rdma_cm_id* cm_id, struct rdma_cm_event* event)
{
   IBVSocket* _this;

   int retVal = 0;
   fhgfs_bool setConnStateNotifyFailed = fhgfs_false;
   IBVSocketConnState_t newNotifyConnState = IBVSOCKETCONNSTATE_FAILED; /* applied at the end if
      setConnStateNotifyFailed == fhgfs_true */

   _this = (IBVSocket*)cm_id->context;
   if(unlikely(!_this) )
   {
      printk_fhgfs_ir_debug(KERN_INFO, "cm_id is being torn down. Event: %d\n", event->event);
      return (event->event == RDMA_CM_EVENT_CONNECT_REQUEST) ? -EINVAL : 0;
   }

   //printk_fhgfs_ir_debug(KERN_INFO, "%s: event %d cm_id %p\n",
   //   __func__, event->event, cm_id); // debug in

   switch(event->event)
   {
      case RDMA_CM_EVENT_ADDR_RESOLVED:
      {
         printk_fhgfs_ir_debug(KERN_INFO, "RDMA_CM_EVENT_ADDR_RESOLVED\n");

         /* new connState, but don't notify connecting thread, because we go on automatically with
            resolve route */

         retVal = rdma_resolve_route(_this->cm_id, IBVSOCKET_CONN_TIMEOUT_MS);
         if(retVal)
         {
            printk_fhgfs_ir_debug(KERN_INFO, "%d:%s: rdma_resolve_route failed. ErrCode: %d\n",
               __LINE__, __func__, retVal);

            setConnStateNotifyFailed = fhgfs_true;
         }

      } break;

      case RDMA_CM_EVENT_ADDR_ERROR:
      {
         printk_fhgfs_ir_debug(KERN_INFO, "RDMA_CM_EVENT_ADDR_ERROR\n");

         setConnStateNotifyFailed = fhgfs_true;

         // don't set retVal != 0 here; we will clean up later and don't want cm_id to be freed now
         //retVal = -ENETUNREACH;
      } break;

      case RDMA_CM_EVENT_ROUTE_RESOLVED:
      {
         printk_fhgfs_ir_debug(KERN_INFO, "RDMA_CM_EVENT_ROUTE_RESOLVED\n");

         __IBVSocket_setConnStateNotify(_this, IBVSOCKETCONNSTATE_ROUTERESOLVED);
      } break;

      case RDMA_CM_EVENT_ROUTE_ERROR:
      {
         printk_fhgfs_ir_debug(KERN_INFO, "RDMA_CM_EVENT_ROUTE_ERROR : %p\n", cm_id);

         setConnStateNotifyFailed = fhgfs_true;

         // don't set retVal != 0 here; we will clean up later and don't want cm_id to be freed now
         //retVal = -ETIMEDOUT;
      } break;

      case RDMA_CM_EVENT_CONNECT_REQUEST:
      {
         printk_fhgfs_ir_debug(KERN_INFO, "RDMA_CM_EVENT_CONNECT_REQUEST\n");

         // incoming connections not supported => reject all
         rdma_reject(cm_id, NULL, 0);
      } break;

      case RDMA_CM_EVENT_CONNECT_RESPONSE:
      {
         printk_fhgfs_ir_debug(KERN_INFO, "RDMA_CM_EVENT_CONNECT_RESPONSE\n");

         retVal = __IBVSocket_connectResponseHandler(_this, cm_id, event);
         if(retVal)
            rdma_reject(cm_id, NULL, 0);
         else
            retVal = rdma_accept(cm_id, NULL);

         if(retVal)
            setConnStateNotifyFailed = fhgfs_true;

         break;
      }

      case RDMA_CM_EVENT_CONNECT_ERROR:
      {
         printk_fhgfs_ir_debug(KERN_INFO, "RDMA_CM_EVENT_CONNECT_ERROR\n");

         setConnStateNotifyFailed = fhgfs_true;

         // don't set retVal != 0 here; we will clean up later and don't want cm_id to be freed now
         //retVal = -ETIMEDOUT;
      } break;

      case RDMA_CM_EVENT_UNREACHABLE:
      {
         printk_fhgfs_ir_debug(KERN_INFO, "RDMA_CM_EVENT_UNREACHABLE\n");

         setConnStateNotifyFailed = fhgfs_true;

         // don't set retVal != 0 here; we will clean up later and don't want cm_id to be freed now
         //retVal = -ENETUNREACH;
      } break;

      case RDMA_CM_EVENT_REJECTED:
      {
         printk_fhgfs_ir_debug(KERN_INFO, "RDMA_CM_EVENT_REJECTED. ib_cm_rej_reason: %d\n",
            (int)event->status);

         if(event->status == IB_CM_REJ_STALE_CONN)
            newNotifyConnState = IBVSOCKETCONNSTATE_REJECTED_STALE;

         setConnStateNotifyFailed = fhgfs_true;

         // don't set retVal != 0 here; we will clean up later and don't want cm_id to be freed now
         //retVal = -ECONNREFUSED;
      } break;

      case RDMA_CM_EVENT_ESTABLISHED:
      {
         printk_fhgfs_ir_debug(KERN_INFO, "RDMA_CM_EVENT_ESTABLISHED\n");

         retVal = __IBVSocket_connectedHandler(_this, event);
         if(retVal)
            setConnStateNotifyFailed = fhgfs_true;
         else
            __IBVSocket_setConnStateNotify(_this, IBVSOCKETCONNSTATE_ESTABLISHED);

         // don't set retVal != 0 here; we will clean up later and don't want cm_id to be freed now
         retVal = 0;
      } break;

      case RDMA_CM_EVENT_DISCONNECTED:
      {
         printk_fhgfs_ir_debug(KERN_INFO, "RDMA_CM_EVENT_DISCONNECTED\n");

         rdma_disconnect(cm_id);
         retVal = __IBVSocket_disconnectedHandler(_this);
      } break;

      case RDMA_CM_EVENT_DEVICE_REMOVAL:
      { // note: all associated ressources have to be released here immediately
         printk_fhgfs_ir_debug(KERN_INFO, "RDMA_CM_EVENT_DEVICE_REMOVAL\n");

         _this->errState = -1;
         __IBVSocket_cleanupCommContext(cm_id, _this->commContext);
         _this->commContext = NULL;

         retVal = -ENETRESET;
      } break;

      default:
      {
         printk_fhgfs_ir_debug(KERN_INFO, "Ignoring RDMA_CMA event: %d\n", event->event);
         //retVal = -ECONNABORTED; // debug in
      } break;
   }


   /* note: rdma_cm automatically calls rdma_destroy_id() if retVal indicates an error
      (so we must make sure that the cm_id won't be destroyed again in IBVSocket_close() ) */

   if(unlikely(retVal) )
   {
      //_this->commContext->qp = NULL; // (doesn't seem to be needed here)

      _this->cm_id = NULL; // cm_id will automatically be destroyed when retVal!=0

      if(!_this->errState)
         _this->errState = -1;
   }

   /* note: wakeup of waiters via setConnStateNotify() must come after _this->cm_id=NULL to avoid
      race conditions */

   if(unlikely(setConnStateNotifyFailed) )
      __IBVSocket_setConnStateNotify(_this, newNotifyConnState);


   //printk_fhgfs_ir_debug(KERN_INFO, "%s: event %d done. status %d\n",
   //   __func__, event->event, retVal); // debug in


   return retVal;
}

/**
 * Invoked when an asynchronous event not associated with a completion occurs on the CQ.
 */
void __IBVSocket_cqSendEventHandler(struct ib_event *event, void *data)
{
   // nothing to be done here
   printk_fhgfs_ir_debug(KERN_INFO, "%s:%d: called. event type: %d (not handled)\n",
      __func__, __LINE__, (int)event->event);
}

/**
 * Invoked when a completion event occurs on the CQ.
 *
 * @param cq_context IBVSocket* _this
 */
void __IBVSocket_sendCompletionHandler(struct ib_cq *cq, void *cq_context)
{
   IBVSocket* _this = (IBVSocket*)cq_context;
   IBVCommContext* commContext = _this->commContext;
   int reqNotifySendRes;

   atomic_inc(&commContext->sendCompEventCount);
   //printk_fhgfs_ir(KERN_INFO, "INC( SEND ); read: %2d -- %p\n",
   //   atomic_read(&commContext->sendCompEventCount), _this); // debug in

   reqNotifySendRes = ib_req_notify_cq(commContext->sendCQ, IB_CQ_NEXT_COMP);
   if(unlikely(reqNotifySendRes) )
      printk_fhgfs_ir(KERN_INFO, "%s: Couldn't request CQ notification\n", __func__);

   wake_up_interruptible(&commContext->sendCompWaitQ);
}

/**
 * Invoked when an asynchronous event not associated with a completion occurs on the CQ.
 */
void __IBVSocket_cqRecvEventHandler(struct ib_event *event, void *data)
{
   // nothing to be done here
   printk_fhgfs_ir_debug(KERN_INFO, "%s:%d: called. event type: %d (not handled)\n",
      __func__, __LINE__, (int)event->event);
}

/**
 * Invoked when a completion event occurs on the CQ.
 *
 * @param cq_context IBVSocket* _this
 */
void __IBVSocket_recvCompletionHandler(struct ib_cq *cq, void *cq_context)
{
   IBVSocket* _this = (IBVSocket*)cq_context;
   IBVCommContext* commContext = _this->commContext;
   int reqNotifyRecvRes;

   atomic_inc(&commContext->recvCompEventCount);
   //printk_fhgfs_ir(KERN_INFO, "INC( RECV ); read: %2d -- %p\n",
   //   atomic_read(&commContext->recvCompEventCount), _this); // debug in

   reqNotifyRecvRes = ib_req_notify_cq(commContext->recvCQ, IB_CQ_NEXT_COMP);
   if(unlikely(reqNotifyRecvRes) )
      printk_fhgfs_ir(KERN_INFO, "%s: Couldn't request CQ notification\n", __func__);

   wake_up_interruptible(&commContext->recvCompWaitQ);
}

void __IBVSocket_qpEventHandler(struct ib_event *event, void *data)
{
   // nothing to be done here
   printk_fhgfs_ir_debug(KERN_INFO, "%s:%d: called. event type: %d (not handled)\n",
      __func__, __LINE__, (int)event->event);
}

int __IBVSocket_routeResolvedHandler(IBVSocket* _this, struct rdma_cm_id* cm_id,
   IBVCommConfig* commCfg, IBVCommContext** outCommContext)
{
   fhgfs_bool createContextRes;
   struct rdma_conn_param conn_param;

   createContextRes = __IBVSocket_createCommContext(_this, _this->cm_id, commCfg,
      &_this->commContext);
   if(!createContextRes)
   {
      printk_fhgfs_ir(KERN_INFO, "%d:%s: creation of CommContext failed\n", __LINE__, __func__);
      _this->errState = -1;

      return -EPERM;
   }

   // establish connection...

   __IBVSocket_initCommDest(_this->commContext, &_this->localDest);

   memset(&conn_param, 0, sizeof(conn_param) );
   conn_param.responder_resources = 1;
   conn_param.initiator_depth = 1;
   conn_param.flow_control = 0;
   conn_param.retry_count = 7; // (3 bits)
   conn_param.rnr_retry_count = 7; // rnr = receiver not ready (3 bits, 7 means infinity)
   conn_param.private_data = &_this->localDest;
   conn_param.private_data_len = sizeof(_this->localDest);


   return rdma_connect(_this->cm_id, &conn_param);
}


int __IBVSocket_connectResponseHandler(IBVSocket* _this, struct rdma_cm_id* cm_id,
   struct rdma_cm_event* event)
{
   return 0;
}

int __IBVSocket_connectedHandler(IBVSocket* _this, struct rdma_cm_event* event)
{
   int retVal = 0;
   fhgfs_bool parseCommDestRes;

   const void* private_data;
   u8 private_data_len;

#if defined BEEGFS_OFED_1_2_API && (BEEGFS_OFED_1_2_API == 1)
   private_data = event->private_data;
   private_data_len = event->private_data_len;
#else // OFED 1.2.5 or higher API
   private_data = event->param.conn.private_data;
   private_data_len = event->param.conn.private_data_len;
#endif

   parseCommDestRes = __IBVSocket_parseCommDest(
      private_data, private_data_len, &_this->remoteDest);
   if(!parseCommDestRes)
   {
      printk_fhgfs_ir(KERN_INFO, "%d:%s: bad private data received. len: %d\n",
         __LINE__, __func__, private_data_len);

      retVal = -EOPNOTSUPP;
      goto err_invalidateSock;
   }


   return retVal;


err_invalidateSock:
   _this->errState = -1;

   return retVal;
}

int __IBVSocket_disconnectedHandler(IBVSocket* _this)
{
   return 0;
}


/**
 * Note: Doesn't notify (wake) waiting threads.
 */
void __IBVSocket_setConnState(IBVSocket* _this, IBVSocketConnState_t connState)
{
   _this->connState = connState;
}

/**
 * Note: Notifies (wakes) waiting threads.
 */
void __IBVSocket_setConnStateNotify(IBVSocket* _this, IBVSocketConnState_t connState)
{
   _this->connState = connState;

   wake_up(&_this->eventWaitQ);
}

IBVSocketConnState_t __IBVSocket_getConnState(IBVSocket* _this)
{
   IBVSocketConnState_t retVal;

   retVal = _this->connState;

   return retVal;
}

/**
 * @return fhgfs_true if value changed, fhgfs_false on timeout
 */
fhgfs_bool __IBVSocket_waitForConnStateChange(IBVSocket* _this, IBVSocketConnState_t oldState)
{
   /* note: don't use wait_event_interruptible here, because that would lead to races, e.g. if a
      signal is pending at connectByIP:rdma_resolve_addr(). then cmaHandler() would invalidate the
      cm_id, but IBVSocket_close would also try to cleanup the cm_id (=> race condition).
      By waiting uninterruptible, we can assure to wait for the cmaHandler to complete before
      running IBVSocket_close. */

   wait_event(_this->eventWaitQ, _this->connState != oldState);

   return fhgfs_true;
}

/**
 * Convert millisecs to jiffies and mind the max schedule timeout.
 */
long __IBVSocket_msToJiffiesSchedulable(unsigned ms)
{
   unsigned long resultJiffies = msecs_to_jiffies(ms);

   return (resultJiffies >= MAX_SCHEDULE_TIMEOUT) ? (MAX_SCHEDULE_TIMEOUT-1) : resultJiffies;
}

struct ib_cq* __IBVSocket_createCompletionQueue(struct ib_device* device,
   ib_comp_handler comp_handler, void (*event_handler)(struct ib_event *, void *),
   void* cq_context, int cqe)
{
  // #if defined (BEEGFS_OFED_1_2_API) && BEEGFS_OFED_1_2_API >= 1
   //   return ib_create_cq(device, comp_handler, event_handler, cq_context, cqe);
   //#elif defined OFED_HAS_IB_CREATE_CQATTR 
      struct ib_cq_init_attr attrs = {
         .cqe = cqe,
         .comp_vector = 0,
      };

      return ib_create_cq(device, comp_handler, event_handler, cq_context, &attrs);
   //#else // OFED 1.2.5 or higher API
   //   return ib_create_cq(device, comp_handler, event_handler, cq_context, cqe, 0);
   //#endif
}

/**
 * @return pointer to static buffer with human readable string for a wc status code
 */
const char* __IBVSocket_wcStatusStr(int wcStatusCode)
{
   switch(wcStatusCode)
   {
      case IB_WC_WR_FLUSH_ERR:
         return "work request flush error";
      case IB_WC_RETRY_EXC_ERR:
         return "retries exceeded error";
      case IB_WC_RESP_TIMEOUT_ERR:
         return "response timeout error";

      default:
         return "<undefined>";
   }
}

#endif // BEEGFS_OPENTK_IBVERBS

