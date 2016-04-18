#include <common/components/RegistrationDatagramListener.h>
#include <program/Program.h>

#include <toolkit/FsckTkEx.h>
#include <opentk/logging/SyslogLogger.h>

#include <signal.h>

#define APP_WORKERS_DIRECT_NUM   1
#define APP_SYSLOG_IDENTIFIER    "beegfs-fsck"

App::App(int argc, char** argv)
{
   this->argc = argc;
   this->argv = argv;

   this->appResult = APPCODE_NO_ERROR;

   this->cfg = NULL;
   this->netFilter = NULL;
   this->tcpOnlyFilter = NULL;
   this->logger = NULL;
   this->log = NULL;
   this->allowedInterfaces = NULL;
   this->localNode = NULL;
   this->mgmtNodes = NULL;
   this->metaNodes = NULL;
   this->storageNodes = NULL;
   this->internodeSyncer = NULL;
   this->targetMapper = NULL;
   this->targetStateStore = NULL;
   this->buddyGroupMapper = NULL;
   this->workQueue = NULL;
   this->ackStore = NULL;
   this->netMessageFactory = NULL;
   this->dgramListener = NULL;
   this->modificationEventHandler = NULL;
   this->testRunner = NULL;
   this->runMode = NULL;

}

App::~App()
{
   // Note: Logging of the common lib classes is not working here, because this is called
   // from class Program (so the thread-specific app-pointer isn't set in this context).

   workersDelete();

   SAFE_DELETE(this->dgramListener);
   SAFE_DELETE(this->allowedInterfaces);
   SAFE_DELETE(this->netMessageFactory);
   SAFE_DELETE(this->ackStore);
   SAFE_DELETE(this->workQueue);
   SAFE_DELETE(this->storageNodes);
   SAFE_DELETE(this->metaNodes);
   SAFE_DELETE(this->mgmtNodes);
   SAFE_DELETE(this->localNode);
   SAFE_DELETE(this->buddyGroupMapper);
   SAFE_DELETE(this->targetStateStore);
   SAFE_DELETE(this->targetMapper);
   SAFE_DELETE(this->internodeSyncer);
   SAFE_DELETE(this->log);
   SAFE_DELETE(this->logger);
   SAFE_DELETE(this->tcpOnlyFilter);
   SAFE_DELETE(this->netFilter);
   SAFE_DELETE(this->cfg);
   SAFE_DELETE(this->testRunner);
   SAFE_DELETE(this->runMode);

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
      std::cerr << std::endl;

      if(this->log)
         log->logErr(e.what() );

      this->appResult = APPCODE_INVALID_CONFIG;
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
   bool componentsStarted = false;

   bool runModeValid = initRunMode();
   if (!runModeValid) // runMode is help; run and exit
   {
      runMode->execute();
      appResult = APPCODE_INVALID_CONFIG;
      return;
   }

   initDataObjects(argc, argv);

   // wait for mgmtd
   if ( !cfg->getSysMgmtdHost().length() )
      throw InvalidConfigException("Management host undefined");

   bool mgmtWaitRes = waitForMgmtNode();
   if(!mgmtWaitRes)
   { // typically user just pressed ctrl+c in this case
      log->logErr("Waiting for beegfs-mgmtd canceled");
      appResult = APPCODE_RUNTIME_ERROR;
      return;
   }

   // init components
   try
   {
      initComponents();
   }
   catch (ComponentInitException& e)
   {
      FsckTkEx::fsckOutput(e.what(), OutputOptions_DOUBLELINEBREAK | OutputOptions_STDERR);

      FsckTkEx::fsckOutput("A hard error occurred. BeeGFS Fsck will abort.",
         OutputOptions_LINEBREAK | OutputOptions_STDERR);
      appResult = APPCODE_INITIALIZATION_ERROR;
      return;
   }

   // log system and configuration info

   logInfos();

   // start component threads
   startComponents();
   componentsStarted = true;

   try
   {
      if ( FsckTkEx::testVersions(this->metaNodes, this->storageNodes) )
         runMode->execute();
      else
      {
         FsckTkEx::printVersionHeader(true);
         FsckTkEx::fsckOutput(
                  "Fsck cannot run if one of the BeeGFS servers is newer than beegfs-fsck",
                  OutputOptions_ADDLINEBREAKBEFORE | OutputOptions_DOUBLELINEBREAK |
                  OutputOptions_STDERR);
      }
   }
   catch (InvalidConfigException& e)
   {
      ModeHelp modeHelp;
      modeHelp.execute();

      appResult = APPCODE_INVALID_CONFIG;
   }
   catch (std::exception &e)
   {
      FsckTkEx::fsckOutput("Unrecoverable error. BeeGFS Fsck will abort.",
         OutputOptions_LINEBREAK | OutputOptions_ADDLINEBREAKBEFORE | OutputOptions_STDERR);
      FsckTkEx::fsckOutput(e.what(), OutputOptions_LINEBREAK | OutputOptions_STDERR);
   }

   // self-termination
   if(componentsStarted)
   {
      stopComponents();
      joinComponents();
   }
}

void App::runUnitTests()
{
   this->logger = new Logger(cfg);

   this->log = new LogContext("App");

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

/*
 * @return true if runMode is valid and NOT the help mode
 */
bool App::initRunMode()
{
   switch(cfg->determineRunMode())
   {
      case RunMode_CHECKFS:
      {
         this->runMode = new ModeCheckFS();
         return true;
      }
      break;

      case RunMode_ENABLEQUOTA:
      {
         this->runMode = new ModeEnableQuota();
         return true;
      }
      break;

      default:
      {
         this->runMode = new ModeHelp();
         return false;
      }
      break;
   }
}

void App::initDataObjects(int argc, char** argv)
{
   this->netFilter = new NetFilter(cfg->getConnNetFilterFile() );
   this->tcpOnlyFilter = new NetFilter(cfg->getConnTcpOnlyFilterFile() );

   this->logger = new Logger(cfg);

   this->log = new LogContext("App");

   this->allowedInterfaces = new StringList();
   std::string interfacesFilename = this->cfg->getConnInterfacesFile();
   if ( interfacesFilename.length() )
      this->cfg->loadStringListFile(interfacesFilename.c_str(), *this->allowedInterfaces);

   RDMASocket::rdmaForkInitOnce();

   this->targetMapper = new TargetMapper();
   this->targetStateStore = new TargetStateStore();
   this->buddyGroupMapper = new MirrorBuddyGroupMapper(targetMapper);

   this->mgmtNodes = new NodeStoreServers(NODETYPE_Mgmt, false);
   this->metaNodes = new NodeStoreServers(NODETYPE_Meta, false);
   this->storageNodes = new NodeStoreServers(NODETYPE_Storage, false);

   this->workQueue = new MultiWorkQueue();
   this->ackStore = new AcknowledgmentStore();

   initLocalNodeInfo();

   registerSignalHandler();

   // apply forced root (if configured)
   uint16_t forcedRoot = this->cfg->getSysForcedRoot();
   if ( forcedRoot )
      metaNodes->setRootNodeNumID(forcedRoot, true);

   this->netMessageFactory = new NetMessageFactory();
}

void App::initLocalNodeInfo()
{
   bool useSDP = this->cfg->getConnUseSDP();
   bool useRDMA = this->cfg->getConnUseRDMA();

   NetworkInterfaceCard::findAll(this->allowedInterfaces, useSDP, useRDMA, &(this->localNicList));

   if ( this->localNicList.empty() )
      throw InvalidConfigException("Couldn't find any usable NIC");

   this->localNicList.sort(&NetworkInterfaceCard::nicAddrPreferenceComp);

   std::string nodeID = System::getHostname();

   this->localNode = new LocalNode(nodeID, 0, 0, 0, this->localNicList);

   this->localNode->setNodeType(NODETYPE_Client);
   this->localNode->setFhgfsVersion(BEEGFS_VERSION_CODE);
}

void App::initComponents()
{
   this->log->log(Log_DEBUG, "Initializing components...");

   // Note: We choose a random udp port here to avoid conflicts with the client
   unsigned short udpListenPort = 0;

   this->dgramListener = new DatagramListener(netFilter, localNicList, ackStore, udpListenPort);
   // update the local node info with udp port
   this->localNode->updateInterfaces(dgramListener->getUDPPort(), 0, this->localNicList);

   this->internodeSyncer = new InternodeSyncer();

   workersInit();

   this->log->log(Log_DEBUG, "Components initialized.");
}

void App::startComponents()
{
   log->log(Log_SPAM, "Starting up components...");

   // make sure child threads don't receive SIGINT/SIGTERM (blocked signals are inherited)
   PThread::blockInterruptSignals();

   dgramListener->start();

   internodeSyncer->start();

   workersStart();

   PThread::unblockInterruptSignals(); // main app thread may receive SIGINT/SIGTERM

   log->log(Log_DEBUG, "Components running.");
}

void App::stopComponents()
{
   // note: this method may not wait for termination of the components, because that could
   //    lead to a deadlock (when calling from signal handler)
   workersStop();

   this->internodeSyncer->selfTerminate();

   if ( dgramListener )
   {
      dgramListener->selfTerminate();
      dgramListener->sendDummyToSelfUDP();
   }

   this->selfTerminate();
}

void App::joinComponents()
{
   log->log(4, "Joining component threads...");

   this->internodeSyncer->join();

   /* (note: we need one thread for which we do an untimed join, so this should be a quite reliably
      terminating thread) */
   this->dgramListener->join();

   workersJoin();
}

void App::workersInit() throw (ComponentInitException)
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
   for ( WorkerListIter iter = workerList.begin(); iter != workerList.end(); iter++ )
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
   log->log(Log_NOTICE, std::string("Version: ") + BEEGFS_VERSION);

   // print debug version info
   LOG_DEBUG_CONTEXT(*log, Log_CRITICAL, "--DEBUG VERSION--");

   // print local nodeID
   log->log(Log_NOTICE, std::string("LocalNode: ") + localNode->getTypedNodeID() );

   // list usable network interfaces
   NicAddressList nicList(localNode->getNicList());

   std::string nicListStr;
   std::string extendedNicListStr;
   for ( NicAddressListIter nicIter = nicList.begin(); nicIter != nicList.end(); nicIter++ )
   {
      std::string nicTypeStr;

      if ( nicIter->nicType == NICADDRTYPE_RDMA )
         nicTypeStr = "RDMA";
      else
      if ( nicIter->nicType == NICADDRTYPE_SDP )
         nicTypeStr = "SDP";
      else
      if ( nicIter->nicType == NICADDRTYPE_STANDARD )
         nicTypeStr = "TCP";
      else
         nicTypeStr = "Unknown";

      nicListStr += std::string(nicIter->name) + "(" + nicTypeStr + ")" + " ";

      extendedNicListStr += "\n+ ";
      extendedNicListStr += NetworkInterfaceCard::nicAddrToString(&(*nicIter)) + " ";
   }

   this->log->log(3, std::string("Usable NICs: ") + nicListStr);
   this->log->log(4, std::string("Extended List of usable NICs: ") + extendedNicListStr);

   // print net filters
   if ( netFilter->getNumFilterEntries() )
   {
      this->log->log(2,
         std::string("Net filters: ") + StringTk::uintToStr(netFilter->getNumFilterEntries()));
   }

   if(tcpOnlyFilter->getNumFilterEntries() )
   {
      this->log->log(Log_WARNING, std::string("TCP-only filters: ") +
         StringTk::uintToStr(tcpOnlyFilter->getNumFilterEntries() ) );
   }
}

/**
 * Request mgmt heartbeat and wait for the mgmt node to appear in nodestore.
 *
 * @return true if mgmt heartbeat received, false on error or thread selftermination order
 */
bool App::waitForMgmtNode()
{
   int waitTimeoutMS = 0; // infinite wait

   // choose a random udp port here
   unsigned udpListenPort = 0;
   unsigned udpMgmtdPort = cfg->getConnMgmtdPortUDP();
   std::string mgmtdHost = cfg->getSysMgmtdHost();

   RegistrationDatagramListener regDGramLis(this->netFilter, this->localNicList, this->ackStore,
      udpListenPort);

   regDGramLis.start();

   log->log(Log_CRITICAL, "Waiting for beegfs-mgmtd@" +
      mgmtdHost + ":" + StringTk::uintToStr(udpMgmtdPort) + "...");

   bool gotMgmtd = NodesTk::waitForMgmtHeartbeat(
      this, &regDGramLis, this->mgmtNodes, mgmtdHost, udpMgmtdPort, waitTimeoutMS);

   regDGramLis.selfTerminate();
   regDGramLis.sendDummyToSelfUDP(); // for faster termination

   regDGramLis.join();

   return gotMgmtd;
}

/*
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

   shallAbort.set(1);
   stopComponents();
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
      }
         break;

      case SIGTERM:
      {
         signal(sig, SIG_DFL); // reset the handler to its default
         log->log(1, logContext, "Received a SIGTERM. Shutting down...");
      }
         break;

      default:
      {
         signal(sig, SIG_DFL); // reset the handler to its default
         log->log(1, logContext, "Received an unknown signal. Shutting down...");
      }
         break;
   }

   app->abort();
}

void App::abort()
{
   shallAbort.set(1);
   stopComponents();
}
