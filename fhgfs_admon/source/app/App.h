#ifndef APP_H_
#define APP_H_

#include <common/app/log/LogContext.h>
#include <common/app/log/Logger.h>
#include <common/app/AbstractApp.h>
#include <common/components/worker/Worker.h>
#include <common/components/worker/DummyWork.h>
#include <common/components/ComponentInitException.h>
#include <common/nodes/LocalNode.h>
#include <common/nodes/NodeStoreClients.h>
#include <common/nodes/Node.h>
#include <common/toolkit/NetFilter.h>
#include <common/toolkit/AcknowledgmentStore.h>
#include <common/toolkit/MessagingTk.h>
#include <common/Common.h>
#include <components/DatagramListener.h>
#include <components/InternodeSyncer.h>
#include <components/requestor/NodeListRequestor.h>
#include <components/Mailer.h>
#include <components/ExternalJobRunner.h>
#include <components/requestor/DataRequestorIOStats.h>
#include <components/WebServer.h>
#include <database/Database.h>
#include <net/message/NetMessageFactory.h>
#include <nodes/NodeStoreMetaEx.h>
#include <nodes/NodeStoreStorageEx.h>
#include <nodes/NodeStoreMgmtEx.h>
#include <testing/TestRunner.h>
#include "config/Config.h"
#include "config/RuntimeConfig.h"

#include <signal.h>
#include "../components/StatsOperator.h"


#ifndef BEEGFS_VERSION
   #error BEEGFS_VERSION undefined
#endif

#if !defined(BEEGFS_VERSION_CODE) || (BEEGFS_VERSION_CODE == 0)
   #error BEEGFS_VERSION_CODE undefined
#endif


// program return codes
#define APPCODE_NO_ERROR               0
#define APPCODE_INVALID_CONFIG         1
#define APPCODE_INITIALIZATION_ERROR   2
#define APPCODE_RUNTIME_ERROR          3


typedef std::list<Worker*> WorkerList;
typedef WorkerList::iterator WorkerListIter;

// forward declarations
class LogContext;

class App : public AbstractApp
{
   public:
      App(int argc, char** argv);
      virtual ~App();

      virtual void run();

      void stopComponents();
      void handleComponentException(std::exception& e);

   private:
      int appResult;
      int argc;
      char** argv;

      Config*  cfg;
      RuntimeConfig *runtimeCfg;
      Logger*  logger;
      LogContext* log;

      int pidFileLockFD; // -1 if unlocked, >=0 otherwise

      NetFilter* netFilter; // empty filter means "all nets allowed"
      NetFilter* tcpOnlyFilter; // for IPs that allow only plain TCP (no RDMA etc)
      StringList* allowedInterfaces; // empty list means "all interfaces accepted"
      NicAddressList localNicList; // intersection set of dicsovered NICs and allowedInterfaces
      Node* localNode;

      MultiWorkQueue* workQueue;
      AcknowledgmentStore* ackStore;
      NetMessageFactory* netMessageFactory;
      InternodeSyncer* internodeSyncer;

      DatagramListener* datagramListener;
      WebServer* webServer;
      WorkerList workerList;

      NodeListRequestor *nodeListRequestor;

      NodeStoreStorageEx *storageNodes;
      NodeStoreMetaEx *metaNodes;
      NodeStoreMgmtEx *mgmtNodes;
      NodeStoreClients *clientNodes;

      TargetMapper* targetMapper;

      StatsOperator *clientStatsOperator;
      DataRequestorIOStats *dataRequestorIOStats;
      Database *db;

      Mailer *mailer;
      ExternalJobRunner *jobRunner;

      TestRunner* testRunner;

      void runNormal();
      void runUnitTests();

      void workersInit() throw(ComponentInitException);
      void workersStart();
      void workersStop();
      void workersDelete();
      void workersJoin();

      void initDataObjects(int argc, char** argv) throw(InvalidConfigException, DatabaseException);
      void initLocalNodeInfo() throw(InvalidConfigException);
      void initComponents() throw(ComponentInitException);
      void startComponents();
      void joinComponents();

      void logInfos();

      void registerSignalHandler();
      static void signalHandler(int sig);
      void daemonize() throw(InvalidConfigException);

   public:
      // inliners

      /**
       * @return NULL for invalid node types
       */
      NodeStoreServers* getServerStoreFromType(NodeType nodeType)
      {
         switch(nodeType)
         {
            case NODETYPE_Meta:
               return metaNodes;

            case NODETYPE_Storage:
               return storageNodes;

            case NODETYPE_Mgmt:
               return mgmtNodes;

            default:
               return NULL;
         }
      }

      /**
       * @return NULL for invalid node types
       */
      AbstractNodeStore* getAbstractNodeStoreFromType(NodeType nodeType)
      {
         switch(nodeType)
         {
            case NODETYPE_Meta:
               return metaNodes;

            case NODETYPE_Storage:
               return storageNodes;

            case NODETYPE_Client:
               return clientNodes;

            case NODETYPE_Mgmt:
               return mgmtNodes;

            default:
               return NULL;
         }
      }

      // getters & setters

      void getStorageNodesAsStringList(StringList *outList);
      void getMetaNodesAsStringList(StringList *outList);
      void getStorageNodeNumIDs(UInt16List *outList);
      void getMetaNodeNumIDs(UInt16List *outList);

      virtual Logger* getLogger()
      {
         return logger;
      }

      virtual ICommonConfig* getCommonConfig()
      {
         return cfg;
      }

      virtual NetFilter* getNetFilter()
      {
         return netFilter;
      }

      virtual NetFilter* getTcpOnlyFilter()
      {
         return tcpOnlyFilter;
      }

      virtual AbstractNetMessageFactory* getNetMessageFactory()
      {
         return netMessageFactory;
      }

      AcknowledgmentStore* getAckStore() const
      {
         return ackStore;
      }

      Config* getConfig()
      {
         return cfg;
      }

      RuntimeConfig* getRuntimeConfig()
      {
    	  return runtimeCfg;
      }

      Node* getLocalNode()
      {
         return localNode;
      }

      DatagramListener* getdatagramListener()
      {
         return datagramListener;
      }

      int getAppResult()
      {
         return appResult;
      }

      Mailer *getMailer()
      {
    	  return mailer;
      }

      ExternalJobRunner *getJobRunner()
      {
    	  return jobRunner;
      }

      NodeStoreStorageEx *getStorageNodes()
      {
         return storageNodes;
      }

      NodeStoreMetaEx *getMetaNodes()
      {
         return metaNodes;
      }

      NodeStoreMgmtEx *getMgmtNodes()
      {
         return mgmtNodes;
      }

      NodeStoreClients *getClientNodes()
      {
         return clientNodes;
      }

      Database *getDB()
      {
         return db;
      }

      DatagramListener *getDatagramListener()
      {
         return datagramListener;
      }

      MultiWorkQueue *getWorkQueue()
      {
         return workQueue;
      }

      StatsOperator *getClientStatsOperator()
      {
         return clientStatsOperator;
      }

      InternodeSyncer* getInternodeSyncer() const
      {
         return internodeSyncer;
      }

      TargetMapper* getTargetMapper() const
      {
         return targetMapper;
      }
};


#endif /*APP_H_*/
