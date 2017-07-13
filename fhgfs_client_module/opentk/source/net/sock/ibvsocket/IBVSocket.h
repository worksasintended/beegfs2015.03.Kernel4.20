#ifndef IBVSOCKET_H_
#define IBVSOCKET_H_

#include <common/threading/Condition.h>
#include <common/threading/Mutex.h>
#include <common/toolkit/Serialization.h>
#include <common/Common.h>
#include <opentk/net/sock/ibvsocket/OpenTk_IBVSocket.h>

#include <linux/in.h>
#include <linux/inet.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <net/ip.h>
#include <net/sock.h>
#include <net/inet_common.h>
#include <asm/atomic.h>

#ifdef BEEGFS_OPENTK_IBVERBS
#include <rdma/ib_cm.h>
#include <rdma/ib_verbs.h>
#include <rdma/rdma_cm.h>
#endif

#define IBVSOCKET_RECV_WORK_ID_OFFSET  (1)
#define IBVSOCKET_SEND_WORK_ID_OFFSET  (1 + IBVSOCKET_RECV_WORK_ID_OFFSET)
#define IBVSOCKET_WRITE_WORK_ID        (1 + IBVSOCKET_SEND_WORK_ID_OFFSET)
#define IBVSOCKET_READ_WORK_ID         (1 + IBVSOCKET_WRITE_WORK_ID)

#define IBVSOCKET_PRIVATEDATA_STR            "fhgfs0 " // must be exactly(!!) 8 bytes long
#define IBVSOCKET_PRIVATEDATA_STR_LEN        8
#define IBVSOCKET_PRIVATEDATA_PROTOCOL_VER   1


struct IBVIncompleteRecv;
typedef struct IBVIncompleteRecv IBVIncompleteRecv;
struct IBVIncompleteSend;
typedef struct IBVIncompleteSend IBVIncompleteSend;

struct IBVCommContext;
typedef struct IBVCommContext IBVCommContext;

struct IBVCommDest;
typedef struct IBVCommDest IBVCommDest;


#ifdef BEEGFS_OPENTK_IBVERBS


enum IBVSocketConnState;
typedef enum IBVSocketConnState IBVSocketConnState_t;


extern char* __IBVSocket_allocAndRegisterBuf(IBVCommContext* commContext, size_t bufLen,
   u64* outBufDMA);
extern void __IBVSocket_freeAndUnregisterBuf(IBVCommContext* commContext, size_t bufLen, void* buf,
   u64 bufDMA);

extern fhgfs_bool __IBVSocket_createNewID(IBVSocket* _this);
extern fhgfs_bool __IBVSocket_createCommContext(IBVSocket* _this, struct rdma_cm_id* cm_id,
   IBVCommConfig* commCfg, IBVCommContext** outCommContext);
extern void __IBVSocket_cleanupCommContext(struct rdma_cm_id* cm_id, IBVCommContext* commContext);

extern void __IBVSocket_initCommDest(IBVCommContext* commContext, IBVCommDest* outDest);
extern fhgfs_bool __IBVSocket_parseCommDest(const void* buf, size_t bufLen, IBVCommDest** outDest);

extern int __IBVSocket_postRecv(IBVSocket* _this, IBVCommContext* commContext, size_t bufIndex);
extern int __IBVSocket_postWrite(IBVSocket* _this, IBVCommDest* remoteDest,
   u64 localBuf, int bufLen);
extern int __IBVSocket_postRead(IBVSocket* _this, IBVCommDest* remoteDest,
   u64 localBuf, int bufLen);
extern int __IBVSocket_postSend(IBVSocket* _this, size_t bufIndex, int bufLen);
extern int __IBVSocket_recvWC(IBVSocket* _this, int timeoutMS, struct ib_wc* outWC);

extern int __IBVSocket_flowControlOnRecv(IBVSocket* _this, int timeoutMS);
extern void __IBVSocket_flowControlOnSendUpdateCounters(IBVSocket* _this);
extern int __IBVSocket_flowControlOnSendWait(IBVSocket* _this, int timeoutMS);

extern int __IBVSocket_waitForRecvCompletionEvent(IBVSocket* _this, int timeoutMS,
   struct ib_wc* outWC);
extern int __IBVSocket_waitForSendCompletionEvent(IBVSocket* _this, int oldSendCount,
   int timeoutMS);
extern int __IBVSocket_waitForTotalSendCompletion(IBVSocket* _this,
   unsigned* numSendElements, unsigned* numWriteElements, unsigned* numReadElements, int timeoutMS);

extern ssize_t __IBVSocket_recvContinueIncomplete(IBVSocket* _this,
   char* buf, size_t bufLen);

extern void __IBVSocket_disconnect(IBVSocket* _this, fhgfs_bool waitForEvent);
extern void __IBVSocket_close(IBVSocket* _this);

extern int __IBVSocket_cmaHandler(struct rdma_cm_id* cm_id, struct rdma_cm_event* event);
extern void __IBVSocket_cqSendEventHandler(struct ib_event* event, void* data);
extern void __IBVSocket_sendCompletionHandler(struct ib_cq* cq, void* cq_context);
extern void __IBVSocket_cqRecvEventHandler(struct ib_event* event, void* data);
extern void __IBVSocket_recvCompletionHandler(struct ib_cq* cq, void* cq_context);
extern void __IBVSocket_qpEventHandler(struct ib_event* event, void* data);
extern int __IBVSocket_routeResolvedHandler(IBVSocket* _this, struct rdma_cm_id* cm_id,
   IBVCommConfig* commCfg, IBVCommContext** outCommContext);
extern int __IBVSocket_connectResponseHandler(IBVSocket* _this, struct rdma_cm_id* cm_id,
   struct rdma_cm_event* event);
extern int __IBVSocket_connectedHandler(IBVSocket* _this, struct rdma_cm_event *event);
extern int __IBVSocket_disconnectedHandler(IBVSocket* _this);

extern void __IBVSocket_setConnState(IBVSocket* _this, IBVSocketConnState_t connState);
extern void __IBVSocket_setConnStateNotify(IBVSocket* _this, IBVSocketConnState_t connState);
extern IBVSocketConnState_t __IBVSocket_getConnState(IBVSocket* _this);
extern fhgfs_bool __IBVSocket_waitForConnStateChange(IBVSocket* _this,
   IBVSocketConnState_t oldState);

extern long __IBVSocket_msToJiffiesSchedulable(unsigned ms);

struct ib_cq* __IBVSocket_createCompletionQueue(struct ib_device* device,
            ib_comp_handler comp_handler, void (*event_handler)(struct ib_event *, void *),
            void* cq_context, int cqe);

const char* __IBVSocket_wcStatusStr(int wcStatusCode);


enum IBVSocketConnState
{
   IBVSOCKETCONNSTATE_UNCONNECTED=0,
   IBVSOCKETCONNSTATE_CONNECTING=1,
   /*IBVSOCKETCONNSTATE_ADDRESSRESOLVED=2,*/
   IBVSOCKETCONNSTATE_ROUTERESOLVED=3,
   IBVSOCKETCONNSTATE_ESTABLISHED=4,
   IBVSOCKETCONNSTATE_FAILED=5,
   IBVSOCKETCONNSTATE_REJECTED_STALE=6
};


struct IBVIncompleteRecv
{
   int                  isAvailable;
   int                  completedOffset;
   struct               ib_wc wc;
};

struct IBVIncompleteSend
{
   unsigned             numAvailable;
   fhgfs_bool           forceWaitForAll; // fhgfs_true if we received only some completions and need
                                         //    to wait for the rest before we can send more data
};

struct IBVCommContext
{
   //struct ib_context*        context; // doesn't exist in kernel ib impl
   struct ib_pd*             pd; // protection domain
   struct ib_mr*             dmaMR; // dma mem region keys

   atomic_t                  recvCompEventCount; // incremented on incoming event notification
   wait_queue_head_t         recvCompWaitQ; // for recvCompEvents
   wait_queue_t              recvWait;
   fhgfs_bool                recvWaitInitialized; // true if init_wait was called for the thread
   atomic_t                  sendCompEventCount; // incremented on incoming event notification
   wait_queue_head_t         sendCompWaitQ; // for sendCompEvents
   wait_queue_t              sendWait;
   fhgfs_bool                sendWaitInitialized; // true if init_wait was called for the thread
   //Mutex                     recvCompEventMutex; // not allowed to sleep in callbacks
   //Condition                 recvCompEventCond;

   struct ib_cq*             recvCQ; // recv completion queue
   struct ib_cq*             sendCQ; // send completion queue
   struct ib_qp*             qp; // send+recv queue pair

   IBVCommConfig             commCfg;
   char**                    recvBufs;
   char**                    sendBufs;
   u64*                      recvBufsDMA; // array of pointers
   u64*                      sendBufsDMA; // array of pointers
   u64                       numUsedSendBufsDMA; // pointer
   u64                       numUsedSendBufsResetDMA; // pointer
   uint64_t* volatile        numUsedSendBufsP; // sender's flow/flood control counter (volatile!!)
   uint64_t* volatile        numUsedSendBufsResetP; // flow/flood control reset value (volatile!!)
   uint64_t                  numUsedRecvBufs; // receiver's flow/flood control (reset) counter
   unsigned                  numReceivedBufsLeft; // flow control v2 to avoid IB rnr timeout
   unsigned                  numSendBufsLeft; // flow control v2 to avoid IB rnr timeout

   IBVIncompleteRecv         incompleteRecv;
   IBVIncompleteSend         incompleteSend;
};

#pragma pack(push, 1)
// Note: Make sure this struct has the same size on all architectures (because we use
//    sizeof(IBVCommDest) for private_data during handshake)
struct IBVCommDest
{
   char                 verificationStr[IBVSOCKET_PRIVATEDATA_STR_LEN];
   uint64_t             protocolVersion;
   uint64_t             vaddr;
   unsigned             rkey;
   unsigned             recvBufNum;
   unsigned             recvBufSize;
};
#pragma pack(pop)

struct IBVSocket
{
   //Mutex                         mutex; // sleeping locks not allowed here!!
   wait_queue_head_t             eventWaitQ; // used to wait for connState change during connect


   struct rdma_cm_id*            cm_id;

   IBVCommDest                   localDest;
   IBVCommDest*                  remoteDest;

   IBVCommContext*               commContext;

   int                           errState; // 0 = <no error>; -1 = <unspecified error>

   volatile IBVSocketConnState_t connState;

   fhgfs_bool                    sockValid;

   int                           typeOfService;
};


#else // BEEGFS_OPENTK_IBVERBS not defined in this case


struct IBVSocket
{
   fhgfs_bool                          sockValid;
};


#endif // BEEGFS_OPENTK_IBVERBS




#endif /*IBVSOCKET_H_*/
