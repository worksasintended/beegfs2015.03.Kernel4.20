#include <app/config/Config.h>
#include <app/log/Logger.h>
#include <app/App.h>
#include <common/components/worker/DummyRetryableWork.h>
#include <common/components/worker/DummyWork.h>
#include <common/components/worker/RetryWorker.h>
#include <common/components/worker/Worker.h>
#include <common/components/worker/WorkQueue.h>
#include <common/nodes/NodeFeatureFlags.h>
#include <common/nodes/MirrorBuddyGroupMapper.h>
#include <common/nodes/TargetMapper.h>
#include <common/nodes/TargetStateStore.h>
#include <common/system/System.h>
#include <common/toolkit/ackstore/AcknowledgmentStore.h>
#include <common/toolkit/NetFilter.h>
#include <common/toolkit/TimeAbs.h>
#include <components/AckManager.h>
#include <components/DatagramListener.h>
#include <components/Flusher.h>
#include <components/InternodeSyncer.h>
#include <filesystem/FhgfsOpsPages.h>
#include <filesystem/FhgfsOpsInode.h>
#include <net/filesystem/FhgfsOpsCommKit.h>
#include <net/filesystem/FhgfsOpsRemoting.h>
#include <nodes/NodeStoreEx.h>
#include <toolkit/InodeRefStore.h>
#include <toolkit/NoAllocBufferStore.h>
#include <toolkit/StatFsCache.h>
#include <toolkit/SyncedCounterStore.h>

#include <linux/xattr.h>

#define APP_FEATURES_ARRAY_LEN      (sizeof(APP_FEATURES)/sizeof(unsigned) )


#ifndef BEEGFS_VERSION
   #error BEEGFS_VERSION undefined
#endif

#if !defined(BEEGFS_VERSION_CODE) || (BEEGFS_VERSION_CODE == 0)
   #error BEEGFS_VERSION_CODE undefined
#endif


/**
 * Array of the feature bit numbers that are supported by this node/service.
 */
unsigned const APP_FEATURES[] =
{
   CLIENT_FEATURE_DUMMY,
};


/**
 * @param mountConfig belongs to the app afterwards (and will automatically be destructed
 * by the app)
 */
void App_init(App* this, MountConfig* mountConfig)
{
   this->mountConfig = mountConfig;

   this->appResult = APPCODE_NO_ERROR;

   this->lockAckAtomicCounter = NULL;
   this->connRetriesEnabled = fhgfs_true;
   this->netBenchModeEnabled = fhgfs_false;

   this->cfg = NULL;
   this->netFilter = NULL;
   this->tcpOnlyFilter = NULL;
   this->logger = NULL;
   this->helperd = NULL;
   this->allowedInterfaces = NULL;
   this->preferredMetaNodes = NULL;
   this->preferredStorageTargets = NULL;
   this->cacheBufStore = NULL;
   this->rwStatesStore = NULL;
   this->pathBufStore = NULL;
   this->msgBufStore = NULL;
   this->ackStore = NULL;
   this->inodeRefStore = NULL;
   this->statfsCache = NULL;
   this->workSyncedCounterStore = NULL;
   this->localNode = NULL;
   this->mgmtNodes = NULL;
   this->metaNodes = NULL;
   this->storageNodes = NULL;
   this->targetMapper = NULL;
   this->mirrorBuddyGroupMapper = NULL;
   this->targetStateStore = NULL;
   this->metaStateStore = NULL;
   this->workQueue = NULL;
   this->retryWorkQueue = NULL;

   this->dgramListener = NULL;
   this->workerList = NULL;
   this->retryWorkerList = NULL;
   this->internodeSyncer = NULL;
   this->ackManager = NULL;
   this->flusher = NULL;

   this->fileInodeOps = NULL;
   this->symlinkInodeOps = NULL;
   this->dirInodeOps = NULL;
   this->specialInodeOps = NULL;

#ifdef BEEGFS_DEBUG
   this->debugCounterMutex = NULL;

   this->numRPCs = 0;
   this->numRemoteReads = 0;
   this->numRemoteWrites = 0;
#endif // BEEGFS_DEBUG

}

/**
 * @param mountConfig belongs to the app afterwards (and will automatically be destructed
 * by the app)
 */
App* App_construct(MountConfig* mountConfig)
{
   App* this = (App*)os_kmalloc(sizeof(*this) );

   App_init(this, mountConfig);

   return this;
}

void App_uninit(App* this)
{
   __App_workersDelete(this);

   SAFE_DESTRUCT(this->flusher, Flusher_destruct);
   SAFE_DESTRUCT(this->ackManager, AckManager_destruct);
   SAFE_DESTRUCT(this->internodeSyncer, InternodeSyncer_destruct);
   SAFE_DESTRUCT(this->retryWorkerList, PointerList_destruct);
   SAFE_DESTRUCT(this->workerList, PointerList_destruct);
   SAFE_DESTRUCT(this->dgramListener, DatagramListener_destruct);

   SAFE_DESTRUCT(this->retryWorkQueue, WorkQueue_destruct);
   SAFE_DESTRUCT(this->workQueue, WorkQueue_destruct);
   SAFE_DESTRUCT(this->storageNodes, NodeStoreEx_destruct);
   SAFE_DESTRUCT(this->metaNodes, NodeStoreEx_destruct);
   SAFE_DESTRUCT(this->mgmtNodes, NodeStoreEx_destruct);

   if(this->logger)
      Logger_setAllLogLevels(this->logger, -1); // disable logging

   SAFE_DESTRUCT(this->localNode, Node_destruct);
   SAFE_DESTRUCT(this->targetMapper, TargetMapper_destruct);
   SAFE_DESTRUCT(this->mirrorBuddyGroupMapper, MirrorBuddyGroupMapper_destruct);
   SAFE_DESTRUCT(this->metaStateStore, TargetStateStore_destruct);
   SAFE_DESTRUCT(this->targetStateStore, TargetStateStore_destruct);
   SAFE_DESTRUCT(this->workSyncedCounterStore, SyncedCounterStore_destruct);
   SAFE_DESTRUCT(this->statfsCache, StatFsCache_destruct);
   SAFE_DESTRUCT(this->inodeRefStore, InodeRefStore_destruct);
   SAFE_DESTRUCT(this->ackStore, AcknowledgmentStore_destruct);
   SAFE_DESTRUCT(this->msgBufStore, NoAllocBufferStore_destruct);
   SAFE_DESTRUCT(this->pathBufStore, NoAllocBufferStore_destruct);
   SAFE_DESTRUCT(this->rwStatesStore, NoAllocBufferStore_destruct);
   SAFE_DESTRUCT(this->cacheBufStore, NoAllocBufferStore_destruct);
   SAFE_DESTRUCT(this->preferredStorageTargets, UInt16List_destruct);
   SAFE_DESTRUCT(this->preferredMetaNodes, UInt16List_destruct);
   SAFE_DESTRUCT(this->allowedInterfaces, StrCpyList_destruct);
   SAFE_DESTRUCT(this->helperd, ExternalHelperd_destruct);
   SAFE_DESTRUCT(this->logger, Logger_destruct);
   SAFE_DESTRUCT(this->tcpOnlyFilter, NetFilter_destruct);
   SAFE_DESTRUCT(this->netFilter, NetFilter_destruct);
   SAFE_DESTRUCT(this->cfg, Config_destruct);

   SAFE_DESTRUCT(this->lockAckAtomicCounter, AtomicInt_destruct);

   SAFE_DESTRUCT(this->mountConfig, MountConfig_destruct);

#ifdef BEEGFS_DEBUG
   SAFE_DESTRUCT(this->debugCounterMutex, Mutex_destruct);
#endif // BEEGFS_DEBUG
}

void App_destruct(App* this)
{
   App_uninit(this);

   os_kfree(this);
}


int App_run(App* this)
{
   // init data objects & storage

   if(!__App_initDataObjects(this, this->mountConfig) )
   {
      printk_fhgfs(KERN_WARNING,
         "Configuration error: Initialization of common objects failed. "
         "(Log file may provide additional information.)\n");
      this->appResult = APPCODE_INVALID_CONFIG;
      return this->appResult;
   }

   if(!__App_initInodeOperations(this) )
   {
      printk_fhgfs(KERN_WARNING, "Initialization of inode operations failed.");
      this->appResult = APPCODE_INITIALIZATION_ERROR;
      return this->appResult;
   }


   if(!__App_initStorage(this) )
   {
      printk_fhgfs(KERN_WARNING, "Configuration error: Initialization of storage failed\n");
      this->appResult = APPCODE_INVALID_CONFIG;
      return this->appResult;
   }

   if(Config_getDebugRunStartupTests(this->cfg) )
   { // startup tests
      // not impl'ed
   }


   // init components

   if(!__App_initComponents(this) )
   {
      printk_fhgfs(KERN_WARNING, "Component initialization error. "
         "(Log file may provide additional information.)\n");
      this->appResult = APPCODE_INITIALIZATION_ERROR;
      return this->appResult;
   }

   if(Config_getDebugRunComponentTests(this->cfg) )
   { // component tests
      // not impl'ed
   }


   __App_logInfos(this);


   // start components

   if(Config_getDebugRunComponentThreads(this->cfg) )
   {
      __App_startComponents(this);

      // Note: We wait some ms for the node downloads here because the kernel would like to
      //    check the properties of the root directory directly after mount.

      InternodeSyncer_waitForMgmtInit(this->internodeSyncer, 1000);

      if(!__App_mountServerCheck(this) )
      { // mount check failed => cancel mount
         printk_fhgfs(KERN_WARNING, "Mount sanity check failed. Canceling mount. "
            "(Log file may provide additional information. Check can be disabled with "
            "sysMountSanityCheckMS=0 in the config file.)\n");
         this->appResult = APPCODE_INITIALIZATION_ERROR;

         return this->appResult;
      }

      if(Config_getDebugRunIntegrationTests(this->cfg) )
      { // integration tests
         // not impl'ed
      }

      if(Config_getDebugRunThroughputTests(this->cfg) )
      { // throughput tests
         // not impl'ed
      }


      // mark: mount succeeded if we got here!
   }


   return this->appResult;
}

void App_stop(App* this)
{
   const char* logContext = "App (stop)";

   // note: be careful, because we don't know what has been succesfully initialized!

   // note: this can also be called from App_run() to cancel a mount!

   if(this->cfg && Config_getDebugRunComponentThreads(this->cfg) )
   {
      __App_stopComponents(this);

      if(!Config_getConnUnmountRetries(this->cfg) )
         this->connRetriesEnabled = fhgfs_false;

      // wait for termination
      __App_joinComponents(this);

      if(this->logger)
         Logger_log(this->logger, 1, logContext, "All components stopped.");
   }
}

fhgfs_bool __App_initDataObjects(App* this, MountConfig* mountConfig)
{
   const char* logContext = "App (init data objects)";

   char* interfacesFilename;
   char* preferredMetaFile;
   char* preferredStorageFile;
   size_t numCacheBufs;
   size_t cacheBufSize;
   size_t numRWStateBufs;
   size_t numRWStateBufNodes;
   size_t rwStateBufsSize;
   size_t numPathBufs;
   size_t pathBufsSize;
   size_t numMsgBufs;
   size_t msgBufsSize;
   size_t numSyncedCounters;


#ifdef BEEGFS_DEBUG
   this->debugCounterMutex = Mutex_construct(); /* required for RPCs (=> logging) etc */
#endif // BEEGFS_DEBUG

   this->lockAckAtomicCounter = AtomicInt_construct(0);

   this->cfg = Config_construct(mountConfig);

   if(!Config_getValid(this->cfg) )
      return fhgfs_false;


   if(!StringTk_hasLength(Config_getSysMgmtdHost(this->cfg) ) )
   {
      printk_fhgfs(KERN_WARNING, "Management host undefined\n");
      return fhgfs_false;
   }


   { // load net filter (required before any connection can be made, incl. local conns)
      const char* netFilterFile = Config_getConnNetFilterFile(this->cfg);

      this->netFilter = NetFilter_construct(netFilterFile);
      if(!NetFilter_getValid(this->netFilter) )
         return fhgfs_false;
   }

   { // load tcp-only filter (required before any connection can be made, incl. local conns)
      const char* tcpOnlyFilterFile = Config_getConnTcpOnlyFilterFile(this->cfg);

      this->tcpOnlyFilter = NetFilter_construct(tcpOnlyFilterFile);
      if(!NetFilter_getValid(this->tcpOnlyFilter) )
         return fhgfs_false;
   }

   // logger (+ hostname as initial clientID, real ID will be set below)
   this->logger = Logger_construct(this, this->cfg);
   if(Config_getLogClientID(this->cfg) )
   {
      const char* hostnameCopy = System_getHostnameCopy();
      Logger_setClientID(this->logger, hostnameCopy);
      os_kfree(hostnameCopy);
   }

   this->helperd = ExternalHelperd_construct(this, this->cfg);

   // load allowed interface file
   this->allowedInterfaces = StrCpyList_construct();
   interfacesFilename = Config_getConnInterfacesFile(this->cfg);
   if(os_strlen(interfacesFilename) &&
      !Config_loadStringListFile(interfacesFilename, this->allowedInterfaces) )
   {
      Logger_logErrFormatted(this->logger, logContext,
         "Unable to load configured interfaces file: %s", interfacesFilename);
      return fhgfs_false;
   }

   // load preferred nodes files

   this->preferredMetaNodes = UInt16List_construct();
   preferredMetaFile = Config_getTunePreferredMetaFile(this->cfg);
   if(os_strlen(preferredMetaFile) &&
      !Config_loadUInt16ListFile(this->cfg, preferredMetaFile, this->preferredMetaNodes) )
   {
      Logger_logErrFormatted(this->logger, logContext,
         "Unable to load configured preferred meta nodes file: %s", preferredMetaFile);
      return fhgfs_false;
   }

   this->preferredStorageTargets = UInt16List_construct();
   preferredStorageFile = Config_getTunePreferredStorageFile(this->cfg);
   if(os_strlen(preferredStorageFile) &&
      !Config_loadUInt16ListFile(this->cfg, preferredStorageFile, this->preferredStorageTargets) )
   {
      Logger_logErrFormatted(this->logger, logContext,
         "Unable to load configured preferred storage nodes file: %s", preferredStorageFile);
      return fhgfs_false;
   }

   // init stores, queues etc.

   this->targetMapper = TargetMapper_construct();

   this->mirrorBuddyGroupMapper = MirrorBuddyGroupMapper_construct();
   if(!MirrorBuddyGroupMapper_getValid(this->mirrorBuddyGroupMapper) )
      return fhgfs_false;

   this->targetStateStore = TargetStateStore_construct();
   if(!TargetStateStore_getValid(this->targetStateStore) )
      return fhgfs_false;

   this->metaStateStore = TargetStateStore_construct();
   if(!TargetStateStore_getValid(this->metaStateStore) )
      return fhgfs_false;

   this->mgmtNodes = NodeStoreEx_construct(this, NODETYPE_Mgmt);
   if(!NodeStoreEx_getValid(this->mgmtNodes) )
      return fhgfs_false;

   this->metaNodes = NodeStoreEx_construct(this, NODETYPE_Meta);
   if(!NodeStoreEx_getValid(this->metaNodes) )
      return fhgfs_false;

   this->storageNodes = NodeStoreEx_construct(this, NODETYPE_Storage);
   if(!NodeStoreEx_getValid(this->storageNodes) )
      return fhgfs_false;

   this->workQueue = WorkQueue_construct();
   this->retryWorkQueue = WorkQueue_construct();

   if(!__App_initLocalNodeInfo(this) )
      return fhgfs_false;

   if(Config_getLogClientID(this->cfg) )
   { // set real clientID
      const char* clientID = Node_getID(this->localNode);
      Logger_setClientID(this->logger, clientID);
   }

   // prealloc buffers

   switch(Config_getTuneFileCacheTypeNum(this->cfg) )
   {
      case FILECACHETYPE_Buffered:
      case FILECACHETYPE_Hybrid:
         numCacheBufs = Config_getTuneFileCacheBufNum(this->cfg);
         break;

      default:
         numCacheBufs = 0;
         break;
   }

   cacheBufSize = Config_getTuneFileCacheBufSize(this->cfg);

   this->cacheBufStore = NoAllocBufferStore_construct(numCacheBufs, cacheBufSize);
   if(!NoAllocBufferStore_getValid(this->cacheBufStore) )
      return fhgfs_false;

   numRWStateBufs = Config_getTuneMaxReadWriteNum(this->cfg);
   numRWStateBufNodes = Config_getTuneMaxReadWriteNodesNum(this->cfg);
   rwStateBufsSize = numRWStateBufNodes * (MAX(sizeof(WritefileState), sizeof(ReadfileState) ) +
      sizeof(PollSocketEx) + BEEGFS_COMMKIT_MSGBUF_SIZE);

   this->rwStatesStore = NoAllocBufferStore_construct(numRWStateBufs, rwStateBufsSize);

   numPathBufs = Config_getTunePathBufNum(this->cfg);
   pathBufsSize = Config_getTunePathBufSize(this->cfg);

   this->pathBufStore = NoAllocBufferStore_construct(numPathBufs, pathBufsSize);

   numMsgBufs = Config_getTuneMsgBufNum(this->cfg);
   msgBufsSize = Config_getTuneMsgBufSize(this->cfg);

   this->msgBufStore = NoAllocBufferStore_construct(numMsgBufs, msgBufsSize);

   this->ackStore = AcknowledgmentStore_construct();

   this->inodeRefStore = InodeRefStore_construct();

   this->statfsCache = StatFsCache_construct(this);

   numSyncedCounters = Config_getTuneNumWorkers(this->cfg) + 1; /* we could have a separate cfg
      variable, but based on number of workers seems reasonable for the counter store use-case */
   this->workSyncedCounterStore = SyncedCounterStore_construct(numSyncedCounters);
   if(!SyncedCounterStore_getStoreValid(this->workSyncedCounterStore) )
      return fhgfs_false;


   return fhgfs_true;
}

/**
 * Initialized the inode_operations structs depending on what features have been enabled in
 * the config.
 */
fhgfs_bool __App_initInodeOperations(App* this)
{
   Config* cfg = App_getConfig(this);

   this->fileInodeOps = os_kzalloc(sizeof(struct inode_operations) );
   this->symlinkInodeOps = os_kzalloc(sizeof(struct inode_operations) );
   this->dirInodeOps = os_kzalloc(sizeof(struct inode_operations) );
   this->specialInodeOps = os_kzalloc(sizeof(struct inode_operations) );

   if (!this->fileInodeOps || !this->symlinkInodeOps ||
       !this->dirInodeOps || !this->specialInodeOps)
   {
      SAFE_KFREE(this->fileInodeOps);
      SAFE_KFREE(this->symlinkInodeOps);
      SAFE_KFREE(this->dirInodeOps);
      SAFE_KFREE(this->specialInodeOps);

      return fhgfs_false;
   }

   this->fileInodeOps->getattr     = FhgfsOps_getattr;
   this->fileInodeOps->permission  = FhgfsOps_permission;
   this->fileInodeOps->setattr     = FhgfsOps_setattr;

   this->symlinkInodeOps->readlink    = generic_readlink; // default is fine for us currently
   this->symlinkInodeOps->follow_link = FhgfsOps_follow_link;
   this->symlinkInodeOps->put_link    = FhgfsOps_put_link;
   this->symlinkInodeOps->getattr     = FhgfsOps_getattr;
   this->symlinkInodeOps->permission  = FhgfsOps_permission;
   this->symlinkInodeOps->setattr     = FhgfsOps_setattr;

#ifdef KERNEL_HAS_ATOMIC_OPEN
   #ifdef BEEGFS_ENABLE_ATOMIC_OPEN
   this->dirInodeOps->atomic_open = FhgfsOps_atomicOpen;
   #endif // BEEGFS_ENABLE_ATOMIC_OPEN
#endif
   this->dirInodeOps->lookup      = FhgfsOps_lookupIntent;
   this->dirInodeOps->create      = FhgfsOps_createIntent;
   this->dirInodeOps->link        = FhgfsOps_link;
   this->dirInodeOps->unlink      = FhgfsOps_unlink;
   this->dirInodeOps->mknod       = FhgfsOps_mknod;
   this->dirInodeOps->symlink     = FhgfsOps_symlink;
   this->dirInodeOps->mkdir       = FhgfsOps_mkdir;
   this->dirInodeOps->rmdir       = FhgfsOps_rmdir;
   this->dirInodeOps->rename      = FhgfsOps_rename;
   this->dirInodeOps->getattr     = FhgfsOps_getattr;
   this->dirInodeOps->permission  = FhgfsOps_permission;
   this->dirInodeOps->setattr     = FhgfsOps_setattr;

   this->specialInodeOps->setattr = FhgfsOps_setattr;

   if (Config_getSysXAttrsEnabled(cfg) )
   {
      this->fileInodeOps->listxattr   = FhgfsOps_listxattr;
      this->fileInodeOps->getxattr    = generic_getxattr;
      this->fileInodeOps->removexattr = FhgfsOps_removexattr;
      this->fileInodeOps->setxattr    = generic_setxattr;

      this->dirInodeOps->listxattr   = FhgfsOps_listxattr;
      this->dirInodeOps->getxattr    = generic_getxattr;
      this->dirInodeOps->removexattr = FhgfsOps_removexattr;
      this->dirInodeOps->setxattr    = generic_setxattr;

      if (Config_getSysACLsEnabled(cfg) )
      {
#ifdef KERNEL_HAS_POSIX_GET_ACL
         this->fileInodeOps->get_acl = FhgfsOps_get_acl;
         this->dirInodeOps->get_acl  = FhgfsOps_get_acl;
         // Note: symlinks don't have ACLs
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0)
         this->fileInodeOps->set_acl = FhgfsOps_set_acl;
         this->dirInodeOps->set_acl  = FhgfsOps_set_acl;
#endif // LINUX_VERSION_CODE
#else
         Logger_logErr(this->logger, "Init inode operations",
            "ACLs activated in config, but not supported on this kernel version.");
         return fhgfs_false;
#endif // KERNEL_HAS_POSIX_GET_ACL
      }
   }

   return fhgfs_true;
}

fhgfs_bool __App_initLocalNodeInfo(App* this)
{
   const char* logContext = "App (init local node info)";

   NicAddressList nicList;
   NicAddressList* sortedNicList;
   char* hostname;
   TimeAbs nowT;
   pid_t currentPID;
   char* nodeID;
   fhgfs_bool useSDP = Config_getConnUseSDP(this->cfg);
   fhgfs_bool useRDMA = Config_getConnUseRDMA(this->cfg);

   NicAddressList_init(&nicList);

   NIC_findAll(this->allowedInterfaces, useSDP, useRDMA, &nicList);

   if(!NicAddressList_length(&nicList) )
   {
      Logger_logErr(this->logger, logContext, "Couldn't find any usable NIC");
      NicAddressList_uninit(&nicList);

      return fhgfs_false;
   }

   // created sorted nicList clone
   sortedNicList = ListTk_cloneSortNicAddressList(&nicList);


   // prepare clientID and create localNode
   TimeAbs_init(&nowT);
   hostname = System_getHostnameCopy();
   currentPID = os_getCurrentPID();

   nodeID = StringTk_kasprintf("%llX-%llX-%s",
      (uint64_t)currentPID, (uint64_t)TimeAbs_getTimeval(&nowT)->tv_sec, hostname);


   this->localNode = Node_construct(this, nodeID, 0, Config_getConnClientPortUDP(this->cfg), 0,
      sortedNicList);

   Node_setFhgfsVersion(this->localNode, BEEGFS_VERSION_CODE);

   { // nodeFeatureFlags
      BitStore nodeFeatureFlags;

      BitStore_init(&nodeFeatureFlags, fhgfs_true);

      __App_featuresToBitStore(APP_FEATURES, APP_FEATURES_ARRAY_LEN, &nodeFeatureFlags);
      Node_setFeatureFlags(this->localNode, &nodeFeatureFlags);

      BitStore_uninit(&nodeFeatureFlags);
   }


   // clean up
   os_kfree(nodeID);
   os_kfree(hostname);
   TimeAbs_uninit(&nowT);

   // delete nicList elems
   ListTk_kfreeNicAddressListElems(sortedNicList);
   ListTk_kfreeNicAddressListElems(&nicList);

   NicAddressList_destruct(sortedNicList);
   NicAddressList_uninit(&nicList);

   return fhgfs_true;
}

fhgfs_bool __App_initStorage(App* this)
{
   // nothing to be done here (client does not use any harddisk storage)

   return fhgfs_true;
}

fhgfs_bool __App_initComponents(App* this)
{
   const char* logContext = "App (init components)";

   Logger_log(this->logger, Log_DEBUG, logContext, "Initializing components...");

   this->dgramListener =
      DatagramListener_construct(this, this->localNode, Config_getConnClientPortUDP(this->cfg) );
   if(!DatagramListener_getValid(this->dgramListener) )
   {
      Logger_logErr(this->logger, logContext,
         "Initialization of DatagramListener component failed");
      return fhgfs_false;
   }

   __App_workersInit(this);

   this->internodeSyncer = InternodeSyncer_construct(this); // (requires dgramlis)
   if(!InternodeSyncer_getValid(this->internodeSyncer) )
   {
      Logger_logErr(this->logger, logContext,
         "Initialization of InternodeSyncer component failed");
      return fhgfs_false;
   }

   this->ackManager = AckManager_construct(this);
   if(!AckManager_getValid(this->ackManager) )
   {
      Logger_logErr(this->logger, logContext,
         "Initialization of AckManager component failed");
      return fhgfs_false;
   }

   this->flusher = Flusher_construct(this);
   if(!Flusher_getValid(this->flusher) )
   {
      Logger_logErr(this->logger, logContext,
         "Initialization of Flusher component failed");
      return fhgfs_false;
   }

   Logger_log(this->logger, Log_DEBUG, logContext, "Components initialized.");

   return fhgfs_true;
}

void __App_startComponents(App* this)
{
   const char* logContext = "App (start components)";

   Logger_log(this->logger, Log_DEBUG, logContext, "Starting up components...");

   Thread_start( (Thread*)this->dgramListener);

   __App_workersStart(this);

   Thread_start( (Thread*)this->internodeSyncer);
   Thread_start( (Thread*)this->ackManager);

   if( (Config_getTuneFileCacheTypeNum(this->cfg) == FILECACHETYPE_Buffered) ||
       (Config_getTuneFileCacheTypeNum(this->cfg) == FILECACHETYPE_Hybrid) )
      Thread_start( (Thread*)this->flusher);

   Logger_log(this->logger, Log_DEBUG, logContext, "Components running.");
}

void __App_stopComponents(App* this)
{
   const char* logContext = "App (stop components)";

   if(this->logger)
      Logger_log(this->logger, Log_WARNING, logContext, "Stopping components...");

   if(this->flusher &&
      ( (Config_getTuneFileCacheTypeNum(this->cfg) == FILECACHETYPE_Buffered) ||
        (Config_getTuneFileCacheTypeNum(this->cfg) == FILECACHETYPE_Hybrid) ) )
      Thread_selfTerminate( (Thread*)this->flusher);
   if(this->ackManager)
      Thread_selfTerminate( (Thread*)this->ackManager);
   if(this->internodeSyncer)
      Thread_selfTerminate( (Thread*)this->internodeSyncer);

   __App_workersStop(this);

   if(this->dgramListener)
      Thread_selfTerminate( (Thread*)this->dgramListener);
}

void __App_joinComponents(App* this)
{
   const char* logContext = "App (join components)";

   if(this->logger)
      Logger_log(this->logger, 4, logContext, "Waiting for components to self-terminate...");


   if( (Config_getTuneFileCacheTypeNum(this->cfg) == FILECACHETYPE_Buffered) ||
       (Config_getTuneFileCacheTypeNum(this->cfg) == FILECACHETYPE_Hybrid) )
      __App_waitForComponentTermination(this, (Thread*)this->flusher);

   __App_waitForComponentTermination(this, (Thread*)this->ackManager);
   __App_waitForComponentTermination(this, (Thread*)this->internodeSyncer);

   __App_workersJoin(this);

   __App_waitForComponentTermination(this, (Thread*)this->dgramListener);
}

/**
 * @param component the thread that we're waiting for via join(); may be NULL (in which case this
 * method returns immediately)
 */
void __App_waitForComponentTermination(App* this, Thread* component)
{
   const char* logContext = "App (wait for component termination)";

   const int timeoutMS = 2000;

   fhgfs_bool isTerminated;

   if(!component)
      return;

   isTerminated = Thread_timedjoin(component, timeoutMS);
   if(!isTerminated)
   { // print message to show user which thread is blocking
      if(this->logger)
         Logger_logFormatted(this->logger, 2, logContext,
            "Still waiting for this component to stop: %s", Thread_getName(component) );

      Thread_join(component);

      if(this->logger)
         Logger_logFormatted(this->logger, 2, logContext,
            "Component stopped: %s", Thread_getName(component) );
   }
}

void __App_logInfos(App* this)
{
   const char* logContext = "App_logInfos";

   char* localNodeID = Node_getID(this->localNode);

   size_t nicListStrLen = 1024;
   char* nicListStr = os_kmalloc(nicListStrLen);
   char* extendedNicListStr = os_kmalloc(nicListStrLen);
   NicAddressList* nicList = Node_getNicList(this->localNode);
   NicAddressListIter nicIter;


   // print software version (BEEGFS_VERSION)
   Logger_logFormatted(this->logger, 1, logContext, "BeeGFS Client Version: %s", BEEGFS_VERSION);

   // print debug version info
   LOG_DEBUG(this->logger, 1, logContext, "--DEBUG VERSION--");

   // print local clientID
   Logger_logFormatted(this->logger, 2, logContext, "ClientID: %s", localNodeID);

   // list usable network interfaces
   NicAddressListIter_init(&nicIter, nicList);
   nicListStr[0] = 0;
   extendedNicListStr[0] = 0;

   for( ; !NicAddressListIter_end(&nicIter); NicAddressListIter_next(&nicIter) )
   {
      const char* nicTypeStr;
      char* nicAddrStr;
      NicAddress* nicAddr = NicAddressListIter_value(&nicIter);
      char* tmpStr = os_kmalloc(nicListStrLen);

      if(nicAddr->nicType == NICADDRTYPE_RDMA)
         nicTypeStr = "RDMA";
      else
      if(nicAddr->nicType == NICADDRTYPE_SDP)
         nicTypeStr = "SDP";
      else
      if(nicAddr->nicType == NICADDRTYPE_STANDARD)
         nicTypeStr = "TCP";
      else
         nicTypeStr = "Unknown";

      // short NIC info
      os_snprintf(tmpStr, nicListStrLen, "%s%s(%s) ", nicListStr, nicAddr->name, nicTypeStr);
      os_strcpy(nicListStr, tmpStr); // tmpStr to avoid writing to a buffer that we're reading
         // from at the same time

      // extended NIC info
      nicAddrStr = NIC_nicAddrToString(nicAddr);
      os_snprintf(tmpStr, nicListStrLen, "%s\n+ %s", extendedNicListStr, nicAddrStr);
      os_strcpy(extendedNicListStr, tmpStr); // tmpStr to avoid writing to a buffer that we're
         // reading from at the same time
      os_kfree(nicAddrStr);

      SAFE_KFREE(tmpStr);
   }

   NicAddressListIter_uninit(&nicIter);

   Logger_logFormatted(this->logger, 2, logContext, "Usable NICs: %s", nicListStr);
   Logger_logFormatted(this->logger, 4, logContext, "Extended list of usable NICs: %s",
      extendedNicListStr);

   // print net filters
   if(NetFilter_getNumFilterEntries(this->netFilter) )
   {
      Logger_logFormatted(this->logger, Log_WARNING, logContext, "Net filters: %d",
         (int)NetFilter_getNumFilterEntries(this->netFilter) );
   }

   if(NetFilter_getNumFilterEntries(this->tcpOnlyFilter) )
   {
      Logger_logFormatted(this->logger, Log_WARNING, logContext, "TCP-only filters: %d",
         (int)NetFilter_getNumFilterEntries(this->tcpOnlyFilter) );
   }

   // print preferred nodes and targets

   if(UInt16List_length(this->preferredMetaNodes) )
   {
      Logger_logFormatted(this->logger, Log_WARNING, logContext, "Preferred metadata servers: %d",
         (int)UInt16List_length(this->preferredMetaNodes) );
   }

   if(UInt16List_length(this->preferredStorageTargets) )
   {
      Logger_logFormatted(this->logger, Log_WARNING, logContext, "Preferred storage targets: %d",
         (int)UInt16List_length(this->preferredStorageTargets) );
   }

   // clean up
   SAFE_KFREE(nicListStr);
   SAFE_KFREE(extendedNicListStr);

}

fhgfs_bool __App_workersInit(App* this)
{
   const size_t workerIDLen = 16;

   unsigned numWorkers = Config_getTuneNumWorkers(this->cfg);
   unsigned numRetryWorkers = Config_getTuneNumRetryWorkers(this->cfg);
   unsigned i;

   // standard workers

   // construct the list
   this->workerList = PointerList_construct();

   // construct the workers and add them to the list
   for(i=0; i < numWorkers; i++)
   {
      Worker* worker;
      char* workerID;

      workerID = (char*)os_kmalloc(workerIDLen);
      os_snprintf(workerID, workerIDLen, BEEGFS_THREAD_NAME_PREFIX_STR "Worker/%u", i+1);

      worker = Worker_construct(this, workerID, this->workQueue);
      PointerList_append(this->workerList, worker);

      os_kfree(workerID);
   }

   // retry workers

   // construct the list
   this->retryWorkerList = PointerList_construct();

   // construct the workers and add them to the list
   for(i=0; i < numRetryWorkers; i++)
   {
      RetryWorker* worker;
      char* workerID;

      workerID = (char*)os_kmalloc(workerIDLen);
      os_snprintf(workerID, workerIDLen, BEEGFS_THREAD_NAME_PREFIX_STR "RtrWrk/%u", i+1);

      worker = RetryWorker_construct(this, workerID, this->retryWorkQueue);
      PointerList_append(this->retryWorkerList, worker);

      os_kfree(workerID);
   }

   return fhgfs_true;
}

void __App_workersStart(App* this)
{
   PointerListIter iter;

   // standard workers

   PointerListIter_init(&iter, this->workerList);

   for(; !PointerListIter_end(&iter); PointerListIter_next(&iter) )
   {
      Worker* worker = (Worker*)PointerListIter_value(&iter);
      Thread_start( (Thread*)worker);
   }

   PointerListIter_uninit(&iter);

   // retry workers

   PointerListIter_init(&iter, this->retryWorkerList);

   for(; !PointerListIter_end(&iter); PointerListIter_next(&iter) )
   {
      RetryWorker* worker = (RetryWorker*)PointerListIter_value(&iter);
      Thread_start( (Thread*)worker);
   }

   PointerListIter_uninit(&iter);
}

void __App_workersStop(App* this)
{
   PointerListIter iter;

   // standard workers

   if(this->workerList) // check whether the list has been initialized
   {
      // note: need two loops here because we don't know if the worker that handles the work will be
      //    the same one that received the self-terminate-request

      PointerListIter_init(&iter, this->workerList);

      for( ; !PointerListIter_end(&iter); PointerListIter_next(&iter) )
      {
         Worker* worker = (Worker*)PointerListIter_value(&iter);

         Thread_selfTerminate( (Thread*)worker);
      }

      PointerListIter_uninit(&iter);


      PointerListIter_init(&iter, this->workerList);

      for( ; !PointerListIter_end(&iter); PointerListIter_next(&iter) )
      {
         WorkQueue_addWork(this->workQueue, (Work*)DummyWork_construct() );
      }

      PointerListIter_uninit(&iter);
   }


   // retry workers

   if(this->retryWorkerList) // check whether the list has been initialized
   {
      // note: need two loops here because we don't know if the worker that handles the work will be
      //    the same one that received the self-terminate-request

      PointerListIter_init(&iter, this->retryWorkerList);

      for( ; !PointerListIter_end(&iter); PointerListIter_next(&iter) )
      {
         RetryWorker* worker = (RetryWorker*)PointerListIter_value(&iter);

         Thread_selfTerminate( (Thread*)worker);
      }

      PointerListIter_uninit(&iter);


      PointerListIter_init(&iter, this->retryWorkerList);

      for( ; !PointerListIter_end(&iter); PointerListIter_next(&iter) )
      {
         WorkQueue_addWork(this->retryWorkQueue, (Work*)DummyRetryableWork_construct() );
      }

      PointerListIter_uninit(&iter);
   }

}

void __App_workersDelete(App* this)
{
   PointerListIter iter;

   // standard workers

   if(this->workerList) // check whether the list has been initialized
   {
      PointerListIter_init(&iter, this->workerList);

      for(; !PointerListIter_end(&iter); PointerListIter_next(&iter) )
      {
         Worker* worker = (Worker*)PointerListIter_value(&iter);

         Worker_destruct(worker);
      }

      PointerListIter_uninit(&iter);

      PointerList_clear(this->workerList);
   }


   // retry workers

   if(this->retryWorkerList) // check whether the list has been initialized
   {
      PointerListIter_init(&iter, this->retryWorkerList);

      for(; !PointerListIter_end(&iter); PointerListIter_next(&iter) )
      {
         RetryWorker* worker = (RetryWorker*)PointerListIter_value(&iter);

         RetryWorker_destruct(worker);
      }

      PointerListIter_uninit(&iter);

      PointerList_clear(this->retryWorkerList);
   }
}

void __App_workersJoin(App* this)
{
   PointerListIter iter;

   // standard workers

   if(this->workerList) // check whether the list has been initialized
   {
      PointerListIter_init(&iter, this->workerList);

      for( ; !PointerListIter_end(&iter); PointerListIter_next(&iter) )
      {
         Worker* worker = (Worker*)PointerListIter_value(&iter);

         __App_waitForComponentTermination(this, (Thread*)worker);
      }

      PointerListIter_uninit(&iter);
   }


   // retry workers

   if(this->retryWorkerList) // check whether the list has been initialized
   {
      PointerListIter_init(&iter, this->retryWorkerList);

      for( ; !PointerListIter_end(&iter); PointerListIter_next(&iter) )
      {
         RetryWorker* worker = (RetryWorker*)PointerListIter_value(&iter);

         __App_waitForComponentTermination(this, (Thread*)worker);
      }

      PointerListIter_uninit(&iter);
   }
}

/**
 * Perform some basic sanity checks to deny mount in case of an error.
 */
fhgfs_bool __App_mountServerCheck(App* this)
{
   const char* logContext = "Mount sanity check";
   Config* cfg = this->cfg;

   unsigned waitMS = Config_getSysMountSanityCheckMS(cfg);

   fhgfs_bool retVal = fhgfs_false;
   fhgfs_bool retriesEnabledOrig = this->connRetriesEnabled;
   char* helperdCheckStr;
   fhgfs_bool mgmtInitDone;
   FhgfsOpsErr statRootErr;
   fhgfs_stat statRootDetails;
   FhgfsOpsErr statStorageErr;
   int64_t storageSpaceTotal;
   int64_t storageSpaceFree;


   if(!waitMS)
      return fhgfs_true; // mount check disabled


   this->connRetriesEnabled = fhgfs_false; // NO _ R E T R I E S

   // check helper daemon

   helperdCheckStr = ExternalHelperd_getHostByName(this->helperd, "localhost");
   if(!helperdCheckStr)
   {
      Logger_logErr(this->logger, logContext, "Communication with user-space helper daemon failed. "
         "Is beegfs-helperd running?");

      goto error_resetRetries;
   }

   os_kfree(helperdCheckStr);


   // wait for management init

   mgmtInitDone = InternodeSyncer_waitForMgmtInit(this->internodeSyncer, waitMS);
   if(!mgmtInitDone)
   {
      Logger_logErr(this->logger, logContext, "Waiting for management server initialization "
         "timed out. Are the server settings correct? Is the management daemon running?");

      goto error_resetRetries;
   }

   // check root metadata server

   statRootErr = FhgfsOpsRemoting_statRoot(this, &statRootDetails);
   if(statRootErr != FhgfsOpsErr_SUCCESS)
   {
      Logger_logErrFormatted(this->logger, logContext, "Retrieval of root directory entry "
         "failed. Are all metadata servers running and registered at the management daemon? "
         "(Error: %s)", FhgfsOpsErr_toErrString(statRootErr) );

      goto error_resetRetries;
   }

   // check storage servers

   statStorageErr = FhgfsOpsRemoting_statStoragePath(this, false,
      &storageSpaceTotal, &storageSpaceFree);

   if(statStorageErr != FhgfsOpsErr_SUCCESS)
   {
      Logger_logErrFormatted(this->logger, logContext, "Retrieval of storage server free space "
         "info failed. Are the storage servers running and registered at the management daemon? "
         "Did you remove a storage target directory on a server? (Error: %s)",
         FhgfsOpsErr_toErrString(statStorageErr) );

      goto error_resetRetries;
   }

   retVal = fhgfs_true;


error_resetRetries:
   this->connRetriesEnabled = retriesEnabledOrig; // R E S E T _ R E T R I E S

   return retVal;
}

/**
 * Note: This is actually a Program version, not an App version, but we have it here now because
 * app provides a stable closed source part and we want this version to be fixed at compile time.
 */
const char* App_getVersionStr(void)
{
   return BEEGFS_VERSION;
}

/**
 * Returns highest feature bit number found in array.
 *
 * @param numArrayElems must be larger than zero.
 */
unsigned __App_featuresGetHighestNum(const unsigned* featuresArray, unsigned numArrayElems)
{
   unsigned i;

   unsigned maxBit = 0;

   for(i=0; i < numArrayElems; i++)
   {
      if(featuresArray[i] > maxBit)
         maxBit = featuresArray[i];
   }

   return maxBit;
}

/**
 * Walk over featuresArray and add found feature bits to outBitStore.
 *
 * @param outBitStore will be resized and cleared before bits are set.
 */
void __App_featuresToBitStore(const unsigned* featuresArray, unsigned numArrayElems,
   BitStore* outBitStore)
{
   unsigned i;

   unsigned maxFeatureBit = __App_featuresGetHighestNum(featuresArray, numArrayElems);

   BitStore_setSize(outBitStore, maxFeatureBit);
   BitStore_clearBits(outBitStore);

   for(i=0; i < numArrayElems; i++)
      BitStore_setBit(outBitStore, featuresArray[i], fhgfs_true);
}
