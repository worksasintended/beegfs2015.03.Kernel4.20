#include <common/app/log/LogContext.h>
#include <common/components/worker/DummyWork.h>
#include <common/components/ComponentInitException.h>
#include <common/net/sock/RDMASocket.h>
#include <common/nodes/LocalNode.h>
#include <common/storage/striping/Raid0Pattern.h>
#include <common/toolkit/TimeAbs.h>
#include <modes/modehelpers/ModeInterruptedException.h>
#include <modes/ModeHelp.h>
#include <modes/ModeReverseLookup.h>
#include <opentk/logging/SyslogLogger.h>
#include <program/Program.h>
#include <testing/StartupTests.h>
#include <testing/ComponentTests.h>
#include <testing/IntegrationTests.h>
#include "App.h"

#include <signal.h>

#define APP_WORKERS_DIRECT_NUM   1
#define APP_SYSLOG_IDENTIFIER    "beegfs-ctl"


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
   this->clientNodes = NULL;
   this->targetMapper = NULL;
   this->mirrorBuddyGroupMapper = NULL;
   this->targetStateStore = NULL;
   this->workQueue = NULL;
   this->ackStore = NULL;
   this->netMessageFactory = NULL;

   this->dgramListener = NULL;
   this->heartbeatMgr = NULL;

   this->testRunner = NULL;
}

App::~App()
{
   // Note: Logging of the common lib classes is not working here, because this is called
   // from class Program (so the thread-specific app-pointer isn't set in this context).

   workersDelete();

   SAFE_DELETE(this->heartbeatMgr);
   SAFE_DELETE(this->dgramListener);
   SAFE_DELETE(this->allowedInterfaces);
   SAFE_DELETE(this->netMessageFactory);
   SAFE_DELETE(this->ackStore);
   SAFE_DELETE(this->workQueue);
   SAFE_DELETE(this->clientNodes);
   SAFE_DELETE(this->storageNodes);
   SAFE_DELETE(this->metaNodes);
   SAFE_DELETE(this->mgmtNodes);
   SAFE_DELETE(this->localNode);
   SAFE_DELETE(this->mirrorBuddyGroupMapper);
   SAFE_DELETE(this->targetMapper);
   SAFE_DELETE(this->targetStateStore);
   SAFE_DELETE(this->log);
   SAFE_DELETE(this->logger);
   SAFE_DELETE(this->tcpOnlyFilter);
   SAFE_DELETE(this->netFilter);
   SAFE_DELETE(this->cfg);
   SAFE_DELETE(this->testRunner);

   SyslogLogger::destroyOnce();
}

void App::run()
{
   /* drop effective user and group ID, in case this executable has the setuid/setgid bit set
      (privileges will be re-elevated when necessary, e.g. for authenticat file reading) */
   System::dropUserAndGroupEffectiveID();

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
      std::cerr << "[BeeGFS Control Tool Version: " << BEEGFS_VERSION << std::endl;
      std::cerr << "Refer to the default config file (/etc/beegfs/beegfs-client.conf)" << std::endl;
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
   bool componentsStarted = false;
   RunMode runMode;

   // init data objects
   initDataObjects(argc, argv);

   // check if mgmt host is defined if mode is not "help"
   runMode = this->cfg->determineRunMode();
   if ( (runMode != RunMode_INVALID) && !cfg->getSysMgmtdHost().length() )
      throw InvalidConfigException("Management host undefined");

   // tests
   if ( cfg->getDebugRunStartupTests() )
   {
      if ( !StartupTests::perform() )
      {
         this->log->log(1, "Startup Tests failed => shutting down...");
         appResult = APPCODE_RUNTIME_ERROR;
         return;
      }
   }

   // init components

   try
   {
      initComponents();

      // tests
      if(cfg->getDebugRunComponentTests() )
      {
         if(!ComponentTests::perform() )
         {
            this->log->log(1, "Component Tests failed => shutting down...");
            appResult = APPCODE_RUNTIME_ERROR;
            return;
         }
      }
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

   try
   {
      if(this->cfg->getRunDaemonized() )
         daemonize();
   }
   catch(InvalidConfigException& e)
   {
      log->logErr(e.what() );
      log->log(1, "A hard error occurred. Shutting down...");
      appResult = APPCODE_INVALID_CONFIG;
      return;
   }


   // start component threads

   if(this->cfg->getDebugRunComponentThreads() )
   {
      startComponents();

      componentsStarted = true;

      // tests
      if(cfg->getDebugRunIntegrationTests() )
      {
         if(!IntegrationTests::perform() )
         {
            this->log->log(1, "Integration Tests failed => shutting down...");
            appResult = APPCODE_RUNTIME_ERROR;

            stopComponents();
            goto err_joinAndExit;
         }
      }

      appResult = executeMode(runMode);

      // self-termination
      stopComponents();
   }


err_joinAndExit:
   if(componentsStarted)
   {
      joinComponents();
      log->log(3, "All components stopped. Exiting now!");
   }
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

int App::executeMode(RunMode runMode)
{
   try
   {
      switch(runMode)
      {
         case RunMode_GETENTRYINFO:
         {
            ModeGetEntryInfo mode;
            return mode.execute();
         } break;

         case RunMode_SETPATTERN:
         {
            ModeSetPattern mode;
            return mode.execute();
         } break;

         case RunMode_LISTNODES:
         {
            ModeGetNodes mode;
            return mode.execute();
         } break;

         case RunMode_DISPOSEUNUSED:
         {
            ModeDisposeUnusedFiles mode;
            return mode.execute();
         } break;

         case RunMode_CREATEFILE:
         {
            ModeCreateFile mode;
            return mode.execute();
         } break;

         case RunMode_CREATEDIR:
         {
            ModeCreateDir mode;
            return mode.execute();
         } break;

         case RunMode_REMOVENODE:
         {
            ModeRemoveNode mode;
            return mode.execute();
         } break;

         case RunMode_IOSTAT:
         case RunMode_SERVERSTATS: // serverstats is an alias for iostat
         {
            ModeIOStat mode;
            return mode.execute();
         } break;

         case RunMode_IOTEST:
         {
            ModeIOTest mode;
            return mode.execute();
         } break;

         case RunMode_REFRESHENTRYINFO:
         {
            ModeRefreshEntryInfo mode;
            return mode.execute();
         } break;

         case RunMode_REMOVEDIRENTRY:
         {
            ModeRemoveDirEntry mode;
            return mode.execute();
         } break;

         case RunMode_MAPTARGET:
         {
            ModeMapTarget mode;
            return mode.execute();
         } break;

         case RunMode_REMOVETARGET:
         {
            ModeRemoveTarget mode;
            return mode.execute();
         } break;

         case RunMode_LISTTARGETS:
         {
            ModeListTargets mode;
            return mode.execute();
         } break;

         case RunMode_IOCTL:
         {
            ModeIoctl mode;
            return mode.execute();
         } break;

         case RunMode_GENERICDEBUG:
         {
            ModeGenericDebug mode;
            return mode.execute();
         } break;

         case RunMode_CLIENTSTATS:
         case RunMode_USERSTATS: // userstats is an alias for "--clientstats --peruser"
         {
            ModeClientStats mode;

            if(runMode == RunMode_USERSTATS)
               mode.setPerUser(true);

            return mode.execute();
         } break;

         case RunMode_REFRESHALLENTRIES:
         {
            ModeRefreshAllEntries mode;
            return mode.execute();
         } break;

         case RunMode_FIND:
         {
            ModeFind mode;
            return mode.execute();
         } break;

         case RunMode_MIGRATE:
         {
            ModeMigrate mode;
            return mode.execute();
         } break;

         case RunMode_REVERSELOOKUP:
         {
            ModeReverseLookup mode;
            return mode.execute();
         } break;

         case RunMode_STORAGEBENCH:
         {
            ModeStorageBench mode;
            return mode.execute();
         } break;

         case RunMode_SETMETADATAMIRRORING:
         {
            ModeSetMetadataMirroring mode;
            return mode.execute();
         } break;

         case RunMode_HASHDIR:
         {
            ModeHashDir mode;
            return mode.execute();
         } break;

         case RunMode_GETQUOTAINFO:
         {
            ModeGetQuotaInfo mode;
            return mode.execute();
         } break;

         case RunMode_SETQUOTALIMITS:
         {
            ModeSetQuota mode;
            return mode.execute();
         } break;

         case RunMode_ADDMIRRORBUDDYGROUP:
         {
            ModeAddMirrorBuddyGroup mode;
            return mode.execute();
         } break;

         case RunMode_LISTMIRRORBUDDYGROUPS:
         {
            ModeListMirrorBuddyGroups mode;
            return mode.execute();
         } break;

         case RunMode_STARTSTORAGERESYNC:
         {
            ModeStartStorageResync mode;
            return mode.execute();
         } break;

         case RunMode_STORAGERESYNCSTATS:
         {
            ModeStorageResyncStats mode;
            return mode.execute();
         } break;

         default:
         {
            ModeHelp mode;
            return mode.execute();
         } break;

      } // end of switch(runMode)
   }
   catch(ModeInterruptedException& e)
   {
      log->log(1, "Execution interrupted by signal!");
      return APPCODE_RUNTIME_ERROR;
   }
}

void App::initDataObjects(int argc, char** argv) throw(InvalidConfigException)
{
  // this->cfg = new Config(argc, argv);

   this->netFilter = new NetFilter(cfg->getConnNetFilterFile() );
   this->tcpOnlyFilter = new NetFilter(cfg->getConnTcpOnlyFilterFile() );

   this->logger = new Logger(cfg);

   // mute the standard logger if it has not been explicitly enabled
   if(!this->cfg->getLogEnabled() )
      this->logger->setLogLevel(0);

   this->log = new LogContext("App");

   this->allowedInterfaces = new StringList();
   std::string interfacesFilename = this->cfg->getConnInterfacesFile();
   if(interfacesFilename.length() )
      this->cfg->loadStringListFile(interfacesFilename.c_str(), *this->allowedInterfaces);

   RDMASocket::rdmaForkInitOnce();

   this->targetMapper = new TargetMapper();
   this->mirrorBuddyGroupMapper = new MirrorBuddyGroupMapper(this->targetMapper);
   this->targetStateStore = new TargetStateStore();

   this->mgmtNodes = new NodeStoreServers(NODETYPE_Mgmt, false);
   this->metaNodes = new NodeStoreServers(NODETYPE_Meta, false);
   this->storageNodes = new NodeStoreServers(NODETYPE_Storage, false);
   this->clientNodes = new NodeStoreClients(false);
   this->workQueue = new MultiWorkQueue();
   this->ackStore = new AcknowledgmentStore();

   initLocalNodeInfo();

   registerSignalHandler();

   this->netMessageFactory = new NetMessageFactory();
}

void App::initLocalNodeInfo() throw(InvalidConfigException)
{
   bool useSDP = this->cfg->getConnUseSDP();
   bool useRDMA = this->cfg->getConnUseRDMA();

   NetworkInterfaceCard::findAll(allowedInterfaces, useSDP, useRDMA, &localNicList);

   if(localNicList.empty() )
      throw InvalidConfigException("Couldn't find any usable NIC");

   localNicList.sort(&NetworkInterfaceCard::nicAddrPreferenceComp);

   std::string nodeID = System::getHostname() + "-" + StringTk::uint64ToHexStr(System::getPID() ) +
      "-" + StringTk::uintToHexStr(TimeAbs().getTimeMS() ) + "-" "ctl";

   localNode = new LocalNode(nodeID, 0, 0, 0, localNicList);

   localNode->setNodeType(NODETYPE_Client);
   localNode->setFhgfsVersion(BEEGFS_VERSION_CODE);
}

void App::initComponents() throw(ComponentInitException)
{
   log->log(Log_DEBUG, "Initializing components...");

   // Note: We choose a random udp port here to avoid conflicts with the client
   unsigned short udpListenPort = 0; //this->cfg->getConnClientPortUDP();
   this->dgramListener = new DatagramListener(
      netFilter, localNicList, ackStore, udpListenPort);
   this->dgramListener->setRecvTimeoutMS(20);

   this->heartbeatMgr = new HeartbeatManager(this->dgramListener);

   workersInit();

   log->log(Log_DEBUG, "Components initialized.");
}

void App::startComponents()
{
   log->log(Log_DEBUG, "Starting up components...");

   // make sure child threads don't receive SIGINT/SIGTERM (blocked signals are inherited)
   PThread::blockInterruptSignals();

   this->dgramListener->start();

   // Note: We use the heartbeatmgr inline, not in a separate thread
   //this->heartbeatMgr->start();
   //this->heartbeatMgr->waitForInitialHeartbeatReqs();

   workersStart();

   PThread::unblockInterruptSignals(); // main app thread may receive SIGINT/SIGTERM

   log->log(Log_DEBUG, "Components running.");
}

void App::stopComponents()
{
   // note: this method may not wait for termination of the components, because that could
   //    lead to a deadlock (when calling from signal handler)

   workersStop();

   if(dgramListener)
   {
      dgramListener->selfTerminate();
      dgramListener->sendDummyToSelfUDP(); // for faster termination
   }
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
   this->dgramListener->join();

   workersJoin();
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
   log->log(Log_NOTICE, std::string("Version: ") + BEEGFS_VERSION);

   // print debug version info
   LOG_DEBUG_CONTEXT(*log, Log_CRITICAL, "--DEBUG VERSION--");

   // print local nodeID
   log->log(Log_NOTICE, std::string("LocalNode: ") + localNode->getTypedNodeID() );

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

   log->log(Log_NOTICE, std::string("Usable NICs: ") + nicListStr);
   log->log(Log_DEBUG, std::string("Extended list of usable NICs: ") + extendedNicListStr);

   // print net filters
   if(netFilter->getNumFilterEntries() )
   {
      log->log(Log_WARNING, std::string("Net filters: ") +
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

   log->log(Log_DEBUG, std::string("Detaching process...") );

   int detachRes = daemon(nochdir, noclose);
   if(detachRes == -1)
      throw InvalidConfigException(std::string("Unable to detach process. SysErr: ") +
         System::getErrString() );
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

