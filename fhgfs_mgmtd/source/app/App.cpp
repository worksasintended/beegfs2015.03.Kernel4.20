#include <common/app/log/LogContext.h>
#include <common/components/worker/DummyWork.h>
#include <common/components/ComponentInitException.h>
#include <common/net/sock/RDMASocket.h>
#include <common/nodes/LocalNode.h>
#include <common/nodes/NodeFeatureFlags.h>
#include <common/nodes/DynamicPoolLimits.h>
#include <common/toolkit/StorageTk.h>
#include <opentk/logging/SyslogLogger.h>
#include <program/Program.h>
#include <testing/StartupTests.h>
#include <testing/ComponentTests.h>
#include <testing/IntegrationTests.h>
#include <toolkit/StorageTkEx.h>
#include "App.h"

#include <signal.h>

#define APP_WORKERS_DIRECT_NUM   1
#define APP_SYSLOG_IDENTIFIER    "beegfs-mgmtd"


/**
 * Array of the feature bit numbers that are supported by this node/service.
 */
unsigned const APP_FEATURES[] =
{
   MGMT_FEATURE_DUMMY,
};


App::App(int argc, char** argv)
{
   this->argc = argc;
   this->argv = argv;

   this->appResult = APPCODE_NO_ERROR;
   this->pidFileLockFD = -1;
   this->workingDirLockFD = -1;

   this->cfg = NULL;
   this->netFilter = NULL;
   this->tcpOnlyFilter = NULL;
   this->logger = NULL;
   this->log = NULL;
   this->allowedInterfaces = NULL;
   this->localNodeNumID = 0; // 0 means invalid/undefined
   this->localNode = NULL;
   this->metaCapacityPools = NULL;
   this->storageCapacityPools = NULL;
   this->targetNumIDMapper = NULL;
   this->targetMapper = NULL;
   this->mirrorBuddyGroupMapper = NULL;
   this->targetStateStore = NULL;
   this->metaStateStore = NULL;
   this->storageBuddyCapacityPools = NULL;
   this->mgmtNodes = NULL;
   this->metaNodes = NULL;
   this->storageNodes = NULL;
   this->clientNodes = NULL;
   this->hsmNodes = NULL;

   this->workQueue = NULL;
   this->ackStore = NULL;
   this->netMessageFactory = NULL;
   this->mgmtdPath = NULL;

   this->dgramListener = NULL;
   this->heartbeatMgr = NULL;
   this->streamListener = NULL;
   this->statsCollector = NULL;
   this->internodeSyncer = NULL;
   this->quotaManager = NULL;

   this->testRunner = NULL;
}

App::~App()
{
   // Note: Logging of the common lib classes is not working here, because this is called
   // from class Program (so the thread-specific app-pointer isn't set in this context).

   workersDelete();

   SAFE_DELETE(this->internodeSyncer);
   SAFE_DELETE(this->statsCollector);
   SAFE_DELETE(this->streamListener);
   SAFE_DELETE(this->heartbeatMgr);
   SAFE_DELETE(this->dgramListener);
   SAFE_DELETE(this->allowedInterfaces);
   SAFE_DELETE(this->mgmtdPath);
   SAFE_DELETE(this->netMessageFactory);
   SAFE_DELETE(this->ackStore);
   SAFE_DELETE(this->workQueue);
   SAFE_DELETE(this->clientNodes);
   SAFE_DELETE(this->hsmNodes);
   SAFE_DELETE(this->storageNodes);
   SAFE_DELETE(this->metaNodes);
   SAFE_DELETE(this->mgmtNodes);
   SAFE_DELETE(this->localNode);
   SAFE_DELETE(this->storageBuddyCapacityPools);
   SAFE_DELETE(this->mirrorBuddyGroupMapper);
   SAFE_DELETE(this->targetMapper);
   SAFE_DELETE(this->targetNumIDMapper);
   SAFE_DELETE(this->metaStateStore);
   SAFE_DELETE(this->targetStateStore);
   SAFE_DELETE(this->storageCapacityPools);
   SAFE_DELETE(this->metaCapacityPools);
   SAFE_DELETE(this->log);
   SAFE_DELETE(this->logger);
   SAFE_DELETE(this->tcpOnlyFilter);
   SAFE_DELETE(this->netFilter);
   SAFE_DELETE(this->quotaManager);

   SAFE_DELETE(this->testRunner);

   if(workingDirLockFD != -1)
      StorageTk::unlockWorkingDirectory(workingDirLockFD);

   unlockPIDFile(pidFileLockFD); // ignored if fd is -1

   SAFE_DELETE(this->cfg);

   SyslogLogger::destroyOnce();
}

void App::run()
{
   try
   {
      SyslogLogger::initOnce(APP_SYSLOG_IDENTIFIER);

      this->cfg = new Config(argc, argv);

      if ( this->cfg->getRunUnitTests() )
         runUnitTests();
      else
         runNormal();
   }
   catch (InvalidConfigException& e)
   {
      std::cerr << std::endl;
      std::cerr << "Error: " << e.what() << std::endl;
      std::cerr << std::endl;
      std::cerr << "[BeeGFS Management Node Version: " << BEEGFS_VERSION << std::endl;
      std::cerr << "Refer to the default config file (/etc/beegfs/beegfs-mgmtd.conf)" << std::endl;
      std::cerr << "or visit http://www.beegfs.com to find out about configuration options.]"
         << std::endl;
      std::cerr << std::endl;

      if(this->log)
         log->logErr(e.what() );

      appResult = APPCODE_INVALID_CONFIG;
      return;
   }
   catch (std::exception& e)
   {
      std::cerr << std::endl;
      std::cerr << "Unrecoverable error: " << e.what() << std::endl;
      std::cerr << std::endl;

      if(this->log)
         log->logErr(e.what() );

      appResult = APPCODE_RUNTIME_ERROR;
      return;
   }
}

void App::runNormal()
{
   // init data objects & storage

   initDataObjects(argc, argv);

   bool startupTestsRes = runStartupTests();
   if(!startupTestsRes)
      return;

   // init components

   try
   {
      initComponents();

      bool componentsTestsRes = runComponentTests();
      if(!componentsTestsRes)
         return;
   }
   catch(ComponentInitException& e)
   {
      log->logErr(e.what() );
      log->log(1, "A hard error occurred. Shutting down...");
      appResult = APPCODE_INITIALIZATION_ERROR;
      return;
   }


   // log system and configuration info

   logInfos();


   // detach process

   if(cfg->getRunDaemonized() )
      daemonize();


   // start component threads

   startComponents();

   bool integrationTestsRes = runIntegrationTests();
   if(!integrationTestsRes)
      return;

   // wait for termination

   joinComponents();


   log->log(Log_CRITICAL, "All components stopped. Exiting now!");
}

void App::runUnitTests()
{
   // if unit tests are configured to run, start the testrunner
   TestRunnerOutputFormat testRunnerOutputFormat;

   if(cfg->getTestOutputFormat() == "xml")
      testRunnerOutputFormat = TestRunnerOutputFormat_XML;
   else
      testRunnerOutputFormat = TestRunnerOutputFormat_TEXT;

   this->testRunner = new TestRunner(cfg->getTestOutputFile(), testRunnerOutputFormat);

   this->testRunner->start();

   // wait for it to finish
   this->testRunner->join();
}

/**
 * @return sets appResult on error, returns false on error, true otherwise (also if tests were
 * not configured to run)
 */
bool App::runStartupTests()
{
   if(!cfg->getDebugRunStartupTests() )
      return true;

   // run startup tests

   bool testRes = StartupTests::perform();
   if(!testRes)
   {
      log->log(Log_CRITICAL, "Startup Tests failed => shutting down...");

      appResult = APPCODE_RUNTIME_ERROR;
      return false;
   }

   return true;
}

/**
 * @return sets appResult on error, returns false on error, true otherwise (also if tests were
 * not configured to run)
 */
bool App::runComponentTests()
{
   if(!cfg->getDebugRunComponentTests() )
      return true;

   // run component tests

   bool testRes = ComponentTests::perform();
   if(!testRes)
   {
      log->log(Log_CRITICAL, "Component Tests failed => shutting down...");

      appResult = APPCODE_RUNTIME_ERROR;
      return false;
   }

   return true;
}

/**
 * @return sets appResult on error, returns false on error, true otherwise (also if tests were
 * not configured to run)
 */
bool App::runIntegrationTests()
{
   if(!cfg->getDebugRunIntegrationTests() )
      return true;

   // run integration tests

   bool testRes = IntegrationTests::perform();
   if(!testRes)
   {
      log->log(Log_CRITICAL, "Integration Tests failed => shutting down...");

      appResult = APPCODE_RUNTIME_ERROR;
      return false;
   }

   return true;
}

void App::initDataObjects(int argc, char** argv) throw(InvalidConfigException)
{
   preinitStorage();

   this->netFilter = new NetFilter(cfg->getConnNetFilterFile() );
   this->tcpOnlyFilter = new NetFilter(cfg->getConnTcpOnlyFilterFile() );

   // (check absolute log path to avoid chdir() problems)
   Path logStdPath(cfg->getLogStdFile() );
   Path logErrPath(cfg->getLogErrFile() );
   if( (!logStdPath.getIsEmpty() && !logStdPath.isAbsolute() ) ||
       (!logErrPath.getIsEmpty() && !logErrPath.isAbsolute() ) )
      throw InvalidConfigException("Path to log file must be absolute");

   this->logger = new Logger(cfg);
   this->log = new LogContext("App");

   // prepare filter for published local interfaces
   this->allowedInterfaces = new StringList();

   std::string interfacesList = cfg->getConnInterfacesList();
   if(!interfacesList.empty() )
   {
      log->log(Log_DEBUG, "Allowed interfaces: " + interfacesList);
      StringTk::explodeEx(interfacesList, ',', true, allowedInterfaces);
   }

   RDMASocket::rdmaForkInitOnce();

   DynamicPoolLimits poolLimitsMetaSpace(cfg->getTuneMetaSpaceLowLimit(),
      cfg->getTuneMetaSpaceEmergencyLimit(),
      cfg->getTuneMetaSpaceNormalSpreadThreshold(),
      cfg->getTuneMetaSpaceLowSpreadThreshold(),
      cfg->getTuneMetaSpaceLowDynamicLimit(),
      cfg->getTuneMetaSpaceEmergencyDynamicLimit() );
   DynamicPoolLimits poolLimitsMetaInodes(cfg->getTuneMetaInodesLowLimit(),
      cfg->getTuneMetaInodesEmergencyLimit(),
      cfg->getTuneMetaInodesNormalSpreadThreshold(),
      cfg->getTuneMetaInodesLowSpreadThreshold(),
      cfg->getTuneMetaInodesLowDynamicLimit(),
      cfg->getTuneMetaInodesEmergencyDynamicLimit() );
   this->metaCapacityPools = new NodeCapacityPools(cfg->getTuneMetaDynamicPools(),
      poolLimitsMetaSpace, poolLimitsMetaInodes);

   DynamicPoolLimits poolLimitsStorageSpace(cfg->getTuneStorageSpaceLowLimit(),
      cfg->getTuneStorageSpaceEmergencyLimit(),
      cfg->getTuneStorageSpaceNormalSpreadThreshold(),
      cfg->getTuneStorageSpaceLowSpreadThreshold(),
      cfg->getTuneStorageSpaceLowDynamicLimit(),
      cfg->getTuneStorageSpaceEmergencyDynamicLimit() );
   DynamicPoolLimits poolLimitsStorageInodes(cfg->getTuneStorageInodesLowLimit(),
      cfg->getTuneStorageInodesEmergencyLimit(),
      cfg->getTuneStorageInodesNormalSpreadThreshold(),
      cfg->getTuneStorageInodesLowSpreadThreshold(),
      cfg->getTuneStorageInodesLowDynamicLimit(),
      cfg->getTuneStorageInodesEmergencyDynamicLimit() );
   this->storageCapacityPools = new TargetCapacityPools(cfg->getTuneStorageDynamicPools(),
      poolLimitsStorageSpace, poolLimitsStorageInodes);

   this->storageBuddyCapacityPools = new NodeCapacityPools(
      false, DynamicPoolLimits(0, 0, 0, 0, 0, 0), DynamicPoolLimits(0, 0, 0, 0, 0, 0) );

   this->targetNumIDMapper = new NumericIDMapper();
   this->targetMapper = new TargetMapper();
   this->mirrorBuddyGroupMapper = new MirrorBuddyGroupMapper(this->targetMapper);

   this->mirrorBuddyGroupMapper->attachCapacityPools(storageBuddyCapacityPools);

   this->mgmtNodes = new NodeStoreServersEx(NODETYPE_Mgmt);
   this->metaNodes = new NodeStoreServersEx(NODETYPE_Meta);
   this->storageNodes = new NodeStoreServersEx(NODETYPE_Storage);
   this->clientNodes = new NodeStoreClientsEx();
   this->hsmNodes = new NodeStoreServersEx(NODETYPE_Hsm);

   this->metaNodes->attachCapacityPools(metaCapacityPools);

   this->storageNodes->attachTargetMapper(targetMapper);
   this->targetMapper->attachCapacityPools(storageCapacityPools);

   this->targetStateStore = new MgmtdTargetStateStore(NODETYPE_Storage);
   this->targetMapper->attachStateStore(targetStateStore);

   this->metaStateStore = new MgmtdTargetStateStore(NODETYPE_Meta);
   this->metaNodes->attachStateStore(metaStateStore);

   this->workQueue = new MultiWorkQueue();
   this->ackStore = new AcknowledgmentStore();

   initLocalNodeInfo();
   this->mgmtNodes->setLocalNode(this->localNode);

   registerSignalHandler();

   initStorage(); // (required here for persistent objects)

   // apply forced root (if configured)
   uint16_t forcedRoot = this->cfg->getSysForcedRoot();
   bool overrideStoredRoot = this->cfg->getSysOverrideStoredRoot();
   if(forcedRoot && metaNodes->setRootNodeNumID(forcedRoot, overrideStoredRoot) )
      this->log->log(Log_CRITICAL, "Root (by config): " + StringTk::uintToStr(forcedRoot) );

   this->netMessageFactory = new NetMessageFactory();
}

void App::initLocalNodeInfo() throw(InvalidConfigException)
{
   bool useSDP = cfg->getConnUseSDP();
   bool useRDMA = cfg->getConnUseRDMA();
   unsigned portUDP = cfg->getConnMgmtdPortUDP();
   unsigned portTCP = cfg->getConnMgmtdPortTCP();

   NetworkInterfaceCard::findAll(allowedInterfaces, useSDP, useRDMA, &localNicList);

   if(!localNicList.size() )
      throw InvalidConfigException("Couldn't find any usable NIC");

   localNicList.sort(&NetworkInterfaceCard::nicAddrPreferenceComp);

   // try to load localNodeID from file and fall back to auto-generated ID otherwise

   try
   {
      Path mgmtdPath(cfg->getStoreMgmtdDirectory() );
      Path nodeIDPath(mgmtdPath, STORAGETK_NODEID_FILENAME);
      StringList nodeIDList; // actually, the file would contain only a single line

      ICommonConfig::loadStringListFile(nodeIDPath.getPathAsStr().c_str(), nodeIDList);
      if(!nodeIDList.empty() )
         localNodeID = *nodeIDList.begin();
   }
   catch(InvalidConfigException& e) {}

   // auto-generate nodeID if it wasn't loaded

   if(localNodeID.empty() )
      localNodeID = System::getHostname();

   // hard-wired numID for mgmt, because we only have a single mgmt node

   localNodeNumID = 1;

   // create the local node

   localNode = new LocalNode(localNodeID, localNodeNumID, portUDP, portTCP, localNicList);

   localNode->setNodeType(NODETYPE_Mgmt);
   localNode->setFhgfsVersion(BEEGFS_VERSION_CODE);

   // nodeFeatureFlags
   BitStore nodeFeatureFlags;

   featuresToBitStore(APP_FEATURES, APP_FEATURES_ARRAY_LEN, &nodeFeatureFlags);
   localNode->setFeatureFlags(&nodeFeatureFlags);
}

void App::preinitStorage() throw(InvalidConfigException)
{
   /* note: this contains things that would actually live inside initStorage() but need to be
      done at an earlier stage (like working dir locking before log file creation) */

   this->mgmtdPath = new Path(cfg->getStoreMgmtdDirectory() );
   std::string mgmtdPathStr = mgmtdPath->getPathAsStr();

   mgmtdPath->initPathStr(); // to init internal path string (and enable later use with "const")

   if(mgmtdPath->getIsEmpty() )
      throw InvalidConfigException("No management storage directory specified");

   if(!mgmtdPath->isAbsolute() ) /* (check to avoid problems after chdir) */
      throw InvalidConfigException("Path to storage directory must be absolute: " + mgmtdPathStr);

   if(!cfg->getStoreAllowFirstRunInit() &&
      !StorageTk::checkStorageFormatFileExists(mgmtdPathStr) )
      throw InvalidConfigException("Storage directory not initialized and "
         "initialization has been disabled: " + mgmtdPathStr);

   this->pidFileLockFD = createAndLockPIDFile(cfg->getPIDFile() ); // ignored if pidFile not defined

   // create storage directory

   if(!StorageTk::createPathOnDisk(*mgmtdPath, false) )
      throw InvalidConfigException("Unable to create mgmt storage directory: " +
         cfg->getStoreMgmtdDirectory() );

   // lock storage directory

   this->workingDirLockFD = StorageTk::lockWorkingDirectory(cfg->getStoreMgmtdDirectory() );
   if(workingDirLockFD == -1)
      throw InvalidConfigException("Invalid working directory: locking failed");
}

void App::initStorage() throw(InvalidConfigException)
{
   std::string mgmtdPathStr = mgmtdPath->getPathAsStr();

   // change current working dir to storage directory

   int changeDirRes = chdir(mgmtdPathStr.c_str() );
   if(changeDirRes)
   { // unable to change working directory
      throw InvalidConfigException("Unable to change working directory to: " + mgmtdPathStr +
         "(SysErr: " + System::getErrString() + ")");
   }

   // storage format file

   if(!StorageTk::createStorageFormatFile(mgmtdPathStr, STORAGETK_FORMAT_CURRENT_VERSION) )
      throw InvalidConfigException("Unable to create storage format file in: " +
         cfg->getStoreMgmtdDirectory() );

   StorageTk::checkAndUpdateStorageFormatFile(mgmtdPathStr,
      STORAGETK_FORMAT_MIN_VERSION, STORAGETK_FORMAT_CURRENT_VERSION);

   // check for nodeID changes

   StorageTk::checkOrCreateOrigNodeIDFile(mgmtdPathStr, localNode->getID() );

   // load stored nodes

   Path metaNodesPath(CONFIG_METANODES_FILENAME);
   metaNodes->setStorePath(metaNodesPath.getPathAsStr() );
   if(metaNodes->loadFromFile() )
      log->log(Log_NOTICE, "Loaded metadata nodes: " +
         StringTk::intToStr(metaNodes->getSize() ) );

   Path storageNodesPath(CONFIG_STORAGENODES_FILENAME);
   storageNodes->setStorePath(storageNodesPath.getPathAsStr() );
   if(storageNodes->loadFromFile() )
      log->log(Log_NOTICE, "Loaded storage nodes: " +
         StringTk::intToStr(storageNodes->getSize() ) );

   Path clientNodesPath(CONFIG_CLIENTNODES_FILENAME);
   clientNodes->setStorePath(clientNodesPath.getPathAsStr() );
   if(clientNodes->loadFromFile() )
      log->log(Log_NOTICE, "Loaded clients: " +
         StringTk::intToStr(clientNodes->getSize() ) );

   // load mapped targetNumIDs

   Path targetNumIDsPath(CONFIG_TARGETNUMIDS_FILENAME);
   targetNumIDMapper->setStorePath(targetNumIDsPath.getPathAsStr() );
   bool targetNumIDsLoaded = targetNumIDMapper->loadFromFile();
   if(targetNumIDsLoaded)
      log->log(Log_DEBUG, "Loaded target numeric ID mappings: " +
         StringTk::intToStr(targetNumIDMapper->getSize() ) );


   // load target mappings

   Path targetsPath(CONFIG_TARGETMAPPINGS_FILENAME);
   targetMapper->setStorePath(targetsPath.getPathAsStr() );
   if(targetMapper->loadFromFile() )
      this->log->log(Log_NOTICE, "Loaded target mappings: " +
         StringTk::intToStr(targetMapper->getSize() ) );

   // load mirror buddy group mappings
   Path buddyGroupsPath(CONFIG_BUDDYGROUPMAPPINGS_FILENAME);
   mirrorBuddyGroupMapper->setStorePath(buddyGroupsPath.getPathAsStr() );
   if( mirrorBuddyGroupMapper->loadFromFile() )
      this->log->log(Log_NOTICE, "Loaded mirror buddy group mappings: " +
         StringTk::intToStr(mirrorBuddyGroupMapper->getSize() ) );

   // load targets need resync list
   Path targetsToResyncPath(CONFIG_TARGETSTORESYNC_FILENAME);
   targetStateStore->setTargetsToResyncStorePath(targetsToResyncPath.getPathAsStr() );
   if (targetStateStore->loadTargetsToResyncFromFile() )
      this->log->log(Log_NOTICE, "Loaded targets to resync list.");

   // raise file descriptor limit

   if(cfg->getTuneProcessFDLimit() )
   {
      uint64_t oldLimit;

      bool setFDLimitRes = System::incProcessFDLimit(cfg->getTuneProcessFDLimit(), &oldLimit);
      if(!setFDLimitRes)
         log->log(Log_CRITICAL, std::string("Unable to increase process resource limit for "
            "number of file handles. Proceeding with default limit: ") +
            StringTk::uintToStr(oldLimit) + " " +
            "(SysErr: " + System::getErrString() + ")");
   }

}


void App::initComponents() throw(ComponentInitException)
{
   log->log(Log_DEBUG, "Initializing components...");

   this->dgramListener = new DatagramListener(
      netFilter, localNicList, ackStore, cfg->getConnMgmtdPortUDP() );
   this->heartbeatMgr = new HeartbeatManager(dgramListener);

   unsigned short listenPort = cfg->getConnMgmtdPortTCP();

   this->streamListener = new StreamListener(localNicList, workQueue, listenPort);

   this->statsCollector = new StatsCollector(workQueue, STATSCOLLECTOR_COLLECT_INTERVAL_MS,
      STATSCOLLECTOR_HISTORY_LENGTH);

   this->internodeSyncer = new InternodeSyncer();

   // init the quota related stuff if required
   if (cfg->getQuotaEnableEnforcment() )
      this->quotaManager = new QuotaManager();

   workersInit();

   log->log(Log_DEBUG, "Components initialized.");
}

void App::startComponents()
{
   log->log(Log_DEBUG, "Starting up components...");

   // make sure child threads don't receive SIGINT/SIGTERM (blocked signals are inherited)
   PThread::blockInterruptSignals();

   this->dgramListener->start();
   this->heartbeatMgr->start();

   this->streamListener->start();

   this->statsCollector->start();

   this->internodeSyncer->start();

   // start the quota related stuff if required
   if (cfg->getQuotaEnableEnforcment() )
      this->quotaManager->start();

   workersStart();

   PThread::unblockInterruptSignals(); // main app thread may receive SIGINT/SIGTERM

   log->log(Log_DEBUG, "Components running.");
}

void App::stopComponents()
{
   // note: this method may not wait for termination of the components, because that could
   //    lead to a deadlock (when calling from signal handler)

   workersStop();

   if(internodeSyncer)
      internodeSyncer->selfTerminate();

   if(quotaManager)
      quotaManager->selfTerminate();

   if(statsCollector)
      statsCollector->selfTerminate();

   if(streamListener)
      streamListener->selfTerminate();

   if(dgramListener)
   {
      dgramListener->selfTerminate();
      dgramListener->sendDummyToSelfUDP(); // for faster termination
   }

   if(heartbeatMgr)
      heartbeatMgr->selfTerminate();


   this->selfTerminate(); /* this flag can be noticed by thread-independent methods and is also
      required e.g. to let App::waitForMgmtNode() know that it should cancel */
}

/**
 * Handles expections that lead to the termination of a component.
 * Initiates an application shutdown.
 */
void App::handleComponentException(std::exception& e)
{
   const char* logContext = "App (component exception handler)";
   LogContext log(logContext);

   log.logErr(std::string("This component encountered an unrecoverable error. ") +
      std::string("[SysErr: ") + System::getErrString() + "] " +
      std::string("Exception message: ") + e.what() );

   log.log(2, "Shutting down...");

   stopComponents();
}


void App::joinComponents()
{
   log->log(4, "Joining component threads...");

   /* (note: we need one thread for which we do an untimed join, so this should be a quite reliably
      terminating thread) */
   this->statsCollector->join();

   workersJoin();

   waitForComponentTermination(dgramListener);
   waitForComponentTermination(heartbeatMgr);
   waitForComponentTermination(streamListener);
   waitForComponentTermination(internodeSyncer);

   if(quotaManager)
      waitForComponentTermination(quotaManager);
}

void App::workersInit() throw(ComponentInitException)
{
   unsigned numWorkers = cfg->getTuneNumWorkers();

   for(unsigned i=0; i < numWorkers; i++)
   {
      Worker* worker = new Worker(
         std::string("Worker") + StringTk::intToStr(i+1), workQueue, QueueWorkType_INDIRECT);
      workerList.push_back(worker);
   }

   for(unsigned i=0; i < APP_WORKERS_DIRECT_NUM; i++)
   {
      Worker* worker = new Worker(
         std::string("DirectWorker") + StringTk::intToStr(i+1), workQueue, QueueWorkType_DIRECT);
      workerList.push_back(worker);
   }
}

void App::workersStart()
{
   for(WorkerListIter iter = workerList.begin(); iter != workerList.end(); iter++)
   {
      (*iter)->start();
   }
}

void App::workersStop()
{
   // need two loops because we don't know if the worker that handles the work will be the same that
   // received the self-terminate-request
   for(WorkerListIter iter = workerList.begin(); iter != workerList.end(); iter++)
   {
      (*iter)->selfTerminate();
   }

   for(WorkerListIter iter = workerList.begin(); iter != workerList.end(); iter++)
   {
      workQueue->addDirectWork(new DummyWork() );
   }
}

void App::workersDelete()
{
   for(WorkerListIter iter = workerList.begin(); iter != workerList.end(); iter++)
   {
      delete(*iter);
   }

   workerList.clear();
}

void App::workersJoin()
{
   for(WorkerListIter iter = workerList.begin(); iter != workerList.end(); iter++)
   {
      waitForComponentTermination(*iter);
   }
}

void App::logInfos()
{
   // print software version (BEEGFS_VERSION)
   log->log(Log_CRITICAL, std::string("Version: ") + BEEGFS_VERSION);

   // print debug version info
   LOG_DEBUG_CONTEXT(*log, Log_CRITICAL, "--DEBUG VERSION--");

   // print local nodeIDs
   log->log(Log_WARNING, "LocalNode: " + localNode->getNodeIDWithTypeStr() );

   // list usable network interfaces
   std::string nicListStr;
   std::string extendedNicListStr;
   for(NicAddressListIter nicIter = localNicList.begin(); nicIter != localNicList.end(); nicIter++)
   {
      std::string nicTypeStr;

      if(nicIter->nicType == NICADDRTYPE_RDMA)
         nicTypeStr = "RDMA";
      else
      if(nicIter->nicType == NICADDRTYPE_SDP)
         nicTypeStr = "SDP";
      else
      if(nicIter->nicType == NICADDRTYPE_STANDARD)
         nicTypeStr = "TCP";
      else
         nicTypeStr = "Unknown";

      nicListStr += std::string(nicIter->name) + "(" + nicTypeStr + ")" + " ";

      extendedNicListStr += "\n+ ";
      extendedNicListStr += NetworkInterfaceCard::nicAddrToString(&(*nicIter) ) + " ";
   }

   this->log->log(Log_WARNING, std::string("Usable NICs: ") + nicListStr);
   this->log->log(Log_DEBUG, std::string("Extended list of usable NICs: ") + extendedNicListStr);

   // print net filters
   if(netFilter->getNumFilterEntries() )
   {
      this->log->log(Log_WARNING, std::string("Net filters: ") +
         StringTk::uintToStr(netFilter->getNumFilterEntries() ) );
   }

   if(tcpOnlyFilter->getNumFilterEntries() )
   {
      this->log->log(Log_WARNING, std::string("TCP-only filters: ") +
         StringTk::uintToStr(tcpOnlyFilter->getNumFilterEntries() ) );
   }
}

void App::daemonize() throw(InvalidConfigException)
{
   int nochdir = 1; // 1 to keep working directory
   int noclose = 0; // 1 to keep stdin/-out/-err open

   this->log->log(4, std::string("Detaching process...") );

   int detachRes = daemon(nochdir, noclose);
   if(detachRes == -1)
      throw InvalidConfigException(std::string("Unable to detach process. SysErr: ") +
         System::getErrString() );

   updateLockedPIDFile(pidFileLockFD); // ignored if pidFileFD is -1
}

void App::registerSignalHandler()
{
   signal(SIGINT, App::signalHandler);
   signal(SIGTERM, App::signalHandler);
}

void App::signalHandler(int sig)
{
   App* app = Program::getApp();

   Logger* log = app->getLogger();
   const char* logContext = "App::signalHandler";

   // note: this might deadlock if the signal was thrown while the logger mutex is locked by the
   //    application thread (depending on whether the default mutex style is recursive). but
   //    even recursive mutexes are not acceptable in this case.
   //    we need something like a timed lock for the log mutex. if it succeeds within a
   //    few seconds, we know that we didn't hold the mutex lock. otherwise we simply skip the
   //    log message. this will only work if the mutex is non-recusive (which is unknown for
   //    the default mutex style).
   //    but it is very unlikely that the application thread holds the log mutex, because it
   //    joins the component threads and so it doesn't do anything else but sleeping!

   switch(sig)
   {
      case SIGINT:
      {
         signal(sig, SIG_DFL); // reset the handler to its default
         log->log(1, logContext, "Received a SIGINT. Shutting down...");
      } break;

      case SIGTERM:
      {
         signal(sig, SIG_DFL); // reset the handler to its default
         log->log(1, logContext, "Received a SIGTERM. Shutting down...");
      } break;

      default:
      {
         signal(sig, SIG_DFL); // reset the handler to its default
         log->log(1, logContext, "Received an unknown signal. Shutting down...");
      } break;
   }

   app->stopComponents();
}

