#ifndef APP_H_
#define APP_H_

#include <app/config/MountConfig.h>
#include <common/toolkit/list/PointerListIter.h>
#include <common/toolkit/list/StrCpyListIter.h>
#include <common/toolkit/list/UInt16ListIter.h>
#include <common/threading/AtomicInt.h>
#include <common/threading/Mutex.h>
#include <common/threading/Thread.h>
#include <common/Common.h>
#include <toolkit/BitStore.h>


// program return codes
#define APPCODE_NO_ERROR               0
#define APPCODE_PROGRAM_ERROR          1
#define APPCODE_INVALID_CONFIG         2
#define APPCODE_INITIALIZATION_ERROR   3
#define APPCODE_RUNTIME_ERROR          4

// forward declarations
struct Config;
struct ExternalHelperd;
struct Logger;
struct Worker;
struct DatagramListener;
struct InternodeSyncer;
struct AckManager;
struct Flusher;
struct Node;
struct NodeStoreEx;
struct TargetMapper;
struct MirrorBuddyGroupMapper;
struct TargetStateStore;
struct WorkQueue;
struct NoAllocBufferStore;
struct AcknowledgmentStore;
struct NetFilter;
struct InodeRefStore;
struct StatFsCache;
struct SyncedCounterStore;



struct App;
typedef struct App App;


extern void App_init(App* this, MountConfig* mountConfig);
extern App* App_construct(MountConfig* mountConfig);
extern void App_uninit(App* this);
extern void App_destruct(App* this);

extern int App_run(App* this);
extern void App_stop(App* this);

extern fhgfs_bool __App_initDataObjects(App* this, MountConfig* mountConfig);
extern fhgfs_bool __App_initInodeOperations(App* this);
extern fhgfs_bool __App_initLocalNodeInfo(App* this);
extern fhgfs_bool __App_initStorage(App* this);
extern fhgfs_bool __App_initComponents(App* this);
extern void __App_startComponents(App* this);
extern void __App_stopComponents(App* this);
extern void __App_joinComponents(App* this);
extern void __App_waitForComponentTermination(App* this, Thread* component);

extern void __App_logInfos(App* this);

extern fhgfs_bool __App_workersInit(App* this);
extern void __App_workersStart(App* this);
extern void __App_workersStop(App* this);
extern void __App_workersDelete(App* this);
extern void __App_workersJoin(App* this);

extern fhgfs_bool __App_mountServerCheck(App* this);

extern unsigned __App_featuresGetHighestNum(const unsigned* featuresArray, unsigned numArrayElems);
extern void __App_featuresToBitStore(const unsigned* featuresArray, unsigned numArrayElems,
   BitStore* outBitStore);


// external getters & setters
extern const char* App_getVersionStr(void);


// inliners
static inline struct Logger* App_getLogger(App* this);
static inline struct ExternalHelperd* App_getHelperd(App* this);
static inline struct Config* App_getConfig(App* this);
static inline struct NetFilter* App_getNetFilter(App* this);
static inline struct NetFilter* App_getTcpOnlyFilter(App* this);
static inline UInt16List* App_getPreferredStorageTargets(App* this);
static inline UInt16List* App_getPreferredMetaNodes(App* this);
static inline struct Node* App_getLocalNode(App* this);
static inline struct NodeStoreEx* App_getMgmtNodes(App* this);
static inline struct NodeStoreEx* App_getMetaNodes(App* this);
static inline struct NodeStoreEx* App_getStorageNodes(App* this);
static inline struct TargetMapper* App_getTargetMapper(App* this);
static inline struct MirrorBuddyGroupMapper* App_getMirrorBuddyGroupMapper(App* this);
static inline struct TargetStateStore* App_getTargetStateStore(App* this);
static inline struct TargetStateStore* App_getMetaStateStore(App* this);
static inline struct WorkQueue* App_getWorkQueue(App* this);
static inline struct WorkQueue* App_getRetryWorkQueue(App* this);
static inline struct NoAllocBufferStore* App_getCacheBufStore(App* this);
static inline struct NoAllocBufferStore* App_getRWStatesStore(App* this);
static inline struct NoAllocBufferStore* App_getPathBufStore(App* this);
static inline struct NoAllocBufferStore* App_getMsgBufStore(App* this);
static inline struct AcknowledgmentStore* App_getAckStore(App* this);
static inline struct InodeRefStore* App_getInodeRefStore(App* this);
static inline struct StatFsCache* App_getStatFsCache(App* this);
static inline struct SyncedCounterStore* App_getWorkSyncedCounterStore(App* this);
static inline struct DatagramListener* App_getDatagramListener(App* this);
static inline struct InternodeSyncer* App_getInternodeSyncer(App* this);
static inline struct AckManager* App_getAckManager(App* this);
static inline struct Flusher* App_getFlusher(App* this);
static inline AtomicInt* App_getLockAckAtomicCounter(App* this);
static inline fhgfs_bool App_getConnRetriesEnabled(App* this);
static inline void App_setConnRetriesEnabled(App* this, fhgfs_bool connRetriesEnabled);
static inline fhgfs_bool App_getNetBenchModeEnabled(App* this);
static inline void App_setNetBenchModeEnabled(App* this, fhgfs_bool netBenchModeEnabled);

static inline struct inode_operations* App_getFileInodeOps(App* this);
static inline struct inode_operations* App_getSymlinkInodeOps(App* this);
static inline struct inode_operations* App_getDirInodeOps(App* this);
static inline struct inode_operations* App_getSpecialInodeOps(App* this);

#ifdef BEEGFS_DEBUG
static inline size_t App_getNumRPCs(App* this);
static inline void App_incNumRPCs(App* this);
static inline size_t App_getNumRemoteReads(App* this);
static inline void App_incNumRemoteReads(App* this);
static inline size_t App_getNumRemoteWrites(App* this);
static inline void App_incNumRemoteWrites(App* this);
#endif // BEEGFS_DEBUG



struct App
{
   int appResult;
   MountConfig* mountConfig;

   struct Config*  cfg;
   struct ExternalHelperd* helperd;
   struct Logger*  logger;

   struct NetFilter* netFilter; // empty filter means "all nets allowed"
   struct NetFilter* tcpOnlyFilter; // for IPs which allow only plain TCP (no RDMA etc)
   StrCpyList* allowedInterfaces; // empty list means "all interfaces accepted"
   UInt16List* preferredMetaNodes; // empty list means "no preferred nodes => use any"
   UInt16List* preferredStorageTargets; // empty list means "no preferred nodes => use any"

   struct Node* localNode;
   struct NodeStoreEx* mgmtNodes;
   struct NodeStoreEx* metaNodes;
   struct NodeStoreEx* storageNodes;

   struct TargetMapper* targetMapper;
   struct MirrorBuddyGroupMapper* mirrorBuddyGroupMapper;
   struct TargetStateStore* targetStateStore; // map storage targets IDs to a state
   struct TargetStateStore* metaStateStore; // map mds targets (i.e. nodeIDs) to a state

   struct WorkQueue* workQueue;
   struct WorkQueue* retryWorkQueue;

   struct NoAllocBufferStore* cacheBufStore; // for buffered cache mode
   struct NoAllocBufferStore* rwStatesStore; // for single-threaded read/write remoting
   struct NoAllocBufferStore* pathBufStore; // for dentry path lookups
   struct NoAllocBufferStore* msgBufStore; // for MessagingTk request/response
   struct AcknowledgmentStore* ackStore;
   struct InodeRefStore* inodeRefStore;
   struct StatFsCache* statfsCache;
   struct SyncedCounterStore* workSyncedCounterStore; /* pool of SynchronizedCounters for work
                                                         completion waiting */

   struct DatagramListener* dgramListener;
   PointerList* workerList;
   PointerList* retryWorkerList;
   struct InternodeSyncer* internodeSyncer;
   struct AckManager* ackManager;
   struct Flusher* flusher;

   AtomicInt* lockAckAtomicCounter; // used by remoting to generate unique lockAckIDs
   volatile fhgfs_bool connRetriesEnabled; // changed at umount and via procfs
   fhgfs_bool netBenchModeEnabled; // changed via procfs to disable server-side disk read/write

   // Inode operations. Since the members of the structs depend on runtime config opts, we need
   // one copy of each struct per App object.
   struct inode_operations* fileInodeOps;
   struct inode_operations* symlinkInodeOps;
   struct inode_operations* dirInodeOps;
   struct inode_operations* specialInodeOps;

#ifdef BEEGFS_DEBUG
   Mutex* debugCounterMutex; // this is the closed tree, so we don't have atomics here (but doesn't
      // matter since this is debug info and not performance critical)

   size_t numRPCs;
   size_t numRemoteReads;
   size_t numRemoteWrites;
#endif // BEEGFS_DEBUG

};


struct Logger* App_getLogger(App* this)
{
   return this->logger;
}

struct Config* App_getConfig(App* this)
{
   return this->cfg;
}

struct ExternalHelperd* App_getHelperd(App* this)
{
   return this->helperd;
}

struct NetFilter* App_getNetFilter(App* this)
{
   return this->netFilter;
}

struct NetFilter* App_getTcpOnlyFilter(App* this)
{
   return this->tcpOnlyFilter;
}

UInt16List* App_getPreferredMetaNodes(App* this)
{
   return this->preferredMetaNodes;
}

UInt16List* App_getPreferredStorageTargets(App* this)
{
   return this->preferredStorageTargets;
}

struct Node* App_getLocalNode(App* this)
{
   return this->localNode;
}

struct NodeStoreEx* App_getMgmtNodes(App* this)
{
   return this->mgmtNodes;
}

struct NodeStoreEx* App_getMetaNodes(App* this)
{
   return this->metaNodes;
}

struct NodeStoreEx* App_getStorageNodes(App* this)
{
   return this->storageNodes;
}

struct TargetMapper* App_getTargetMapper(App* this)
{
   return this->targetMapper;
}

struct MirrorBuddyGroupMapper* App_getMirrorBuddyGroupMapper(App* this)
{
   return this->mirrorBuddyGroupMapper;
}

struct TargetStateStore* App_getTargetStateStore(App* this)
{
   return this->targetStateStore;
}

struct TargetStateStore* App_getMetaStateStore(App* this)
{
   return this->metaStateStore;
}

struct WorkQueue* App_getWorkQueue(App* this)
{
   return this->workQueue;
}

struct WorkQueue* App_getRetryWorkQueue(App* this)
{
   return this->retryWorkQueue;
}

struct NoAllocBufferStore* App_getCacheBufStore(App* this)
{
   return this->cacheBufStore;
}

struct NoAllocBufferStore* App_getRWStatesStore(App* this)
{
   return this->rwStatesStore;
}

struct NoAllocBufferStore* App_getPathBufStore(App* this)
{
   return this->pathBufStore;
}

struct NoAllocBufferStore* App_getMsgBufStore(App* this)
{
   return this->msgBufStore;
}

struct AcknowledgmentStore* App_getAckStore(App* this)
{
   return this->ackStore;
}

struct InodeRefStore* App_getInodeRefStore(App* this)
{
   return this->inodeRefStore;
}

struct StatFsCache* App_getStatFsCache(App* this)
{
   return this->statfsCache;
}

struct SyncedCounterStore* App_getWorkSyncedCounterStore(App* this)
{
   return this->workSyncedCounterStore;
}

struct DatagramListener* App_getDatagramListener(App* this)
{
   return this->dgramListener;
}

struct InternodeSyncer* App_getInternodeSyncer(App* this)
{
   return this->internodeSyncer;
}

struct AckManager* App_getAckManager(App* this)
{
   return this->ackManager;
}

struct Flusher* App_getFlusher(App* this)
{
   return this->flusher;
}

AtomicInt* App_getLockAckAtomicCounter(App* this)
{
   return this->lockAckAtomicCounter;
}

fhgfs_bool App_getConnRetriesEnabled(App* this)
{
   return this->connRetriesEnabled;
}

void App_setConnRetriesEnabled(App* this, fhgfs_bool connRetriesEnabled)
{
   this->connRetriesEnabled = connRetriesEnabled;
}

fhgfs_bool App_getNetBenchModeEnabled(App* this)
{
   return this->netBenchModeEnabled;
}

void App_setNetBenchModeEnabled(App* this, fhgfs_bool netBenchModeEnabled)
{
   this->netBenchModeEnabled = netBenchModeEnabled;
}

struct inode_operations* App_getFileInodeOps(App* this)
{
   return this->fileInodeOps;
}

struct inode_operations* App_getSymlinkInodeOps(App* this)
{
   return this->symlinkInodeOps;
}

struct inode_operations* App_getDirInodeOps(App* this)
{
   return this->dirInodeOps;
}

struct inode_operations* App_getSpecialInodeOps(App* this)
{
   return this->specialInodeOps;
}

#ifdef BEEGFS_DEBUG

   size_t App_getNumRPCs(App* this)
   {
      return this->numRPCs;
   }

   void App_incNumRPCs(App* this)
   {
      Mutex_lock(this->debugCounterMutex);
      this->numRPCs++;
      Mutex_unlock(this->debugCounterMutex);
   }

   size_t App_getNumRemoteReads(App* this)
   {
      return this->numRemoteReads;
   }

   void App_incNumRemoteReads(App* this)
   {
      Mutex_lock(this->debugCounterMutex);
      this->numRemoteReads++;
      Mutex_unlock(this->debugCounterMutex);
   }

   size_t App_getNumRemoteWrites(App* this)
   {
      return this->numRemoteWrites;
   }

   void App_incNumRemoteWrites(App* this)
   {
      Mutex_lock(this->debugCounterMutex);
      this->numRemoteWrites++;
      Mutex_unlock(this->debugCounterMutex);
   }

#else // BEEGFS_DEBUG

   #define App_incNumRPCs(this)
   #define App_incNumRemoteReads(this)
   #define App_incNumRemoteWrites(this)

#endif // BEEGFS_DEBUG


#endif /*APP_H_*/
