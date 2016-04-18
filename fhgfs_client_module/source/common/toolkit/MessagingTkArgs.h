#ifndef MESSAGINGTKARGS_H_
#define MESSAGINGTKARGS_H_

#include <common/nodes/TargetStateInfo.h>
#include <common/Common.h>
#include <toolkit/NoAllocBufferStore.h>


#define REQUESTRESPONSEARGS_FLAG_USEDELAYEDINTERRUPT  1 // block interrupt sig for short recv wait
#define REQUESTRESPONSEARGS_FLAG_ALLOWSTATESLEEP      2 // sleep on non-{good,offline} state

#define REQUESTRESPONSEARGS_LOGFLAG_PEERTRYAGAIN                 1 // peer asked for retry
#define REQUESTRESPONSEARGS_LOGFLAG_PEERINDIRECTCOMMM            2 /* peer encountered indirect comm
                                                                      error, suggests retry */
#define REQUESTRESPONSEARGS_LOGFLAG_PEERINDIRECTCOMMM_NOTAGAIN   4 /* peer encountered indirect comm
                                                                      error, suggests no retry */
#define REQUESTRESPONSEARGS_LOGFLAG_CONNESTABLISHFAILED          8 // unable to establish connection
#define REQUESTRESPONSEARGS_LOGFLAG_COMMERR                     16 // communication error
#define REQUESTRESPONSEARGS_LOGFLAG_RETRY                       32 // communication retry


struct TargetMapper; // forward declaration
struct TargetStateStore; // forward declaration
struct NodeStoreEx; // forward declaration
struct MirrorBuddyGroupMapper; // forward declaration


struct RequestResponseTarget;
typedef struct RequestResponseTarget RequestResponseTarget;

struct RequestResponseNode;
typedef struct RequestResponseNode RequestResponseNode;

enum MessagingTkBufType;
typedef enum MessagingTkBufType MessagingTkBufType;

struct RequestResponseArgs;
typedef struct RequestResponseArgs RequestResponseArgs;


static inline void RequestResponseTarget_prepare(RequestResponseTarget* this, uint16_t targetID,
   struct TargetMapper* targetMapper, struct NodeStoreEx* nodeStore);
static inline void RequestResponseTarget_prepareEx(RequestResponseTarget* this, uint16_t targetID,
   struct TargetMapper* targetMapper, struct NodeStoreEx* nodeStore,
   struct TargetStateStore* targetStates, struct MirrorBuddyGroupMapper* mirrorBuddies,
   fhgfs_bool useBuddyMirrorSecond);
static inline void RequestResponseTarget_setTargetStates(RequestResponseTarget* this,
   struct TargetStateStore* targetStates);
static inline void RequestResponseTarget_setMirrorInfo(RequestResponseTarget* this,
   struct MirrorBuddyGroupMapper* mirrorBuddies, fhgfs_bool useBuddyMirrorSecond);


static inline void RequestResponseNode_prepare(RequestResponseNode* this, uint16_t nodeID,
   struct NodeStoreEx* nodeStore);
static inline void RequestResponseNode_prepareEx(RequestResponseNode* this, uint16_t nodeID,
   struct NodeStoreEx* nodeStore, struct TargetStateStore* targetStates,
   struct MirrorBuddyGroupMapper* mirrorBuddies, fhgfs_bool useBuddyMirrorSecond);
static inline void RequestResponseNode_setTargetStates(RequestResponseNode* this,
   struct TargetStateStore* targetStates);
static inline void RequestResponseNode_setMirrorInfo(RequestResponseNode* this,
   struct MirrorBuddyGroupMapper* mirrorBuddies, fhgfs_bool useBuddyMirrorSecond);


static inline void RequestResponseArgs_prepare(RequestResponseArgs* this, Node* node,
   NetMessage* requestMsg, unsigned respMsgType);
static inline void RequestResponseArgs_freeRespBuffers(RequestResponseArgs* this, App* app);


/**
 * This value container contains arguments for requestResponse() with a certain target.
 */
struct RequestResponseTarget
{
   uint16_t targetID; // receiver
   struct TargetMapper* targetMapper; // for targetID -> nodeID lookup
   struct NodeStoreEx* nodeStore; // for nodeID lookup

   struct TargetStateStore* targetStates; /* if !NULL, check for state "good" or fail immediately if
                                          offline (other states handling depend on mirrorBuddies) */
   TargetReachabilityState outTargetReachabilityState; /* set to state of target by
                                          requestResponseX() if targetStates were given and
                                          FhgfsOpsErr_COMMUNICATION is returned */
   TargetConsistencyState outTargetConsistencyState;

   struct MirrorBuddyGroupMapper* mirrorBuddies; // if !NULL, the given targetID is a mirror groupID
   fhgfs_bool useBuddyMirrorSecond; // true to use secondary instead of primary
};

/**
 * Initializer without targetStates and without mirror handling.
 */
void RequestResponseTarget_prepare(RequestResponseTarget* this, uint16_t targetID,
   struct TargetMapper* targetMapper, struct NodeStoreEx* nodeStore)
{
   this->targetID = targetID;
   this->targetMapper = targetMapper;
   this->nodeStore = nodeStore;

   this->targetStates = NULL;

   this->mirrorBuddies = NULL;
   this->useBuddyMirrorSecond = fhgfs_false;
}

/**
 * Extended initializer with targetStates and with mirror handling.
 */
void RequestResponseTarget_prepareEx(RequestResponseTarget* this, uint16_t targetID,
   struct TargetMapper* targetMapper, struct NodeStoreEx* nodeStore,
   struct TargetStateStore* targetStates, struct MirrorBuddyGroupMapper* mirrorBuddies,
   fhgfs_bool useBuddyMirrorSecond)
{
   this->targetID = targetID;
   this->targetMapper = targetMapper;
   this->nodeStore = nodeStore;

   this->targetStates = targetStates;

   this->mirrorBuddies = mirrorBuddies;
   this->useBuddyMirrorSecond = useBuddyMirrorSecond;
}

void RequestResponseTarget_setTargetStates(RequestResponseTarget* this,
   struct TargetStateStore* targetStates)
{
   this->targetStates = targetStates;
}

void RequestResponseTarget_setMirrorInfo(RequestResponseTarget* this,
   struct MirrorBuddyGroupMapper* mirrorBuddies, fhgfs_bool useBuddyMirrorSecond)
{
   this->mirrorBuddies = mirrorBuddies;
   this->useBuddyMirrorSecond = useBuddyMirrorSecond;
}


/**
 * This value container contains arguments for requestResponse() with a certain node.
 */
struct RequestResponseNode
{
   uint16_t nodeID; // receiver
   struct NodeStoreEx* nodeStore; // for nodeID lookup

   struct TargetStateStore* targetStates; /* if !NULL, check for state "good" or fail immediately if
                                          offline (other states handling depend on mirrorBuddies) */
   TargetReachabilityState outTargetReachabilityState; /* set to state of node by requestResponseX()
                                          if targetStates were given and FhgfsOpsErr_COMMUNICATION
                                          is returned */
   TargetConsistencyState outTargetConsistencyState;

   struct MirrorBuddyGroupMapper* mirrorBuddies; // if !NULL, the given targetID is a mirror groupID
   fhgfs_bool useBuddyMirrorSecond; // true to use secondary instead of primary
};

/**
 * Initializer without targetStates and without mirror handling.
 */
void RequestResponseNode_prepare(RequestResponseNode* this, uint16_t nodeID,
   struct NodeStoreEx* nodeStore)
{
   this->nodeID = nodeID;
   this->nodeStore = nodeStore;

   this->targetStates = NULL;

   this->mirrorBuddies = NULL;
   this->useBuddyMirrorSecond = fhgfs_false;
}

/**
 * Extended initializer with targetStates and with mirror handling.
 */
void RequestResponseNode_prepareEx(RequestResponseNode* this, uint16_t nodeID,
   struct NodeStoreEx* nodeStore, struct TargetStateStore* targetStates,
   struct MirrorBuddyGroupMapper* mirrorBuddies, fhgfs_bool useBuddyMirrorSecond)
{
   this->nodeID = nodeID;
   this->nodeStore = nodeStore;

   this->targetStates = targetStates;

   this->mirrorBuddies = mirrorBuddies;
   this->useBuddyMirrorSecond = useBuddyMirrorSecond;
}

void RequestResponseNode_setTargetStates(RequestResponseNode* this,
   struct TargetStateStore* targetStates)
{
   this->targetStates = targetStates;
}

void RequestResponseNode_setMirrorInfo(RequestResponseNode* this,
   struct MirrorBuddyGroupMapper* mirrorBuddies, fhgfs_bool useBuddyMirrorSecond)
{
   this->mirrorBuddies = mirrorBuddies;
   this->useBuddyMirrorSecond = useBuddyMirrorSecond;
}

enum MessagingTkBufType
{
   MessagingTkBufType_BufStore = 0,
   MessagingTkBufType_kmalloc  = 1, // only for small response messages (<4KB)
};

/**
 * Arguments for request-response communication.
 */
struct RequestResponseArgs
{
   Node* node; // receiver

   NetMessage* requestMsg;
   unsigned respMsgType; // expected type of response message

   unsigned numRetries; // 0 means infinite retries
   unsigned rrFlags; // combination of REQUESTRESPONSEARGS_FLAG_... flags
   MessagingTkBufType respBufType; // defines how to get/alloc outRespBuf

   char* outRespBuf;
   NetMessage* outRespMsg;

   // internal (initialized by MessagingTk_requestResponseWithRRArgs() )
   unsigned char logFlags; // REQUESTRESPONSEARGS_LOGFLAG_... combination to avoid double-logging
};

/**
 * Default initializer.
 * Some of the default values will overridden by the corresponding MessagingTk methods.
 *
 * @param node may be NULL, depending on which requestResponse...() method is used.
 * @param respMsgType expected type of response message (NETMSGTYPE_...)
 */
void RequestResponseArgs_prepare(RequestResponseArgs* this, Node* node, NetMessage* requestMsg,
   unsigned respMsgType)
{
   this->node = node;

   this->requestMsg = requestMsg;
   this->respMsgType = respMsgType;

   this->numRetries = 1;
   this->rrFlags = 0;
   this->respBufType = MessagingTkBufType_BufStore;

   this->outRespBuf = NULL;
   this->outRespMsg = NULL;

   this->logFlags = 0;
}

/**
 * Free/release outRespBuf and desctruct outRespMsg if they are not NULL.
 */
void RequestResponseArgs_freeRespBuffers(RequestResponseArgs* this, App* app)
{
   SAFE_DESTRUCT(this->outRespMsg, NetMessage_virtualDestruct);

   if(this->outRespBuf)
   {
      if(this->respBufType == MessagingTkBufType_BufStore)
      {
         NoAllocBufferStore* bufStore = App_getMsgBufStore(app);
         NoAllocBufferStore_addBuf(bufStore, this->outRespBuf);
      }
      else
         SAFE_KFREE(this->outRespBuf);
   }
}

#endif /* MESSAGINGTKARGS_H_ */
