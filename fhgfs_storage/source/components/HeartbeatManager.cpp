#include <common/net/message/nodes/HeartbeatMsg.h>
#include <common/net/message/nodes/MapTargetsMsg.h>
#include <common/net/message/nodes/MapTargetsRespMsg.h>
#include <common/net/message/storage/quota/RequestExceededQuotaMsg.h>
#include <common/net/message/storage/quota/RequestExceededQuotaRespMsg.h>
#include <common/storage/StorageErrors.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/Random.h>
#include <common/toolkit/SocketTk.h>
#include <common/toolkit/NodesTk.h>
#include <net/msghelpers/MsgHelperIO.h>
#include <program/Program.h>
#include "HeartbeatManager.h"


HeartbeatManager::HeartbeatManager()
   throw(ComponentInitException) : PThread("HBeatMgr")
{
   log.setContext("HBeatMgr");
}

HeartbeatManager::~HeartbeatManager()
{
}

void HeartbeatManager::run()
{
   try
   {
      registerSignalHandler();

      requestLoop();

      log.log(Log_DEBUG, "Component stopped.");
   }
   catch(std::exception& e)
   {
      Program::getApp()->handleComponentException(e);
   }
}


void HeartbeatManager::requestLoop()
{
   int sleepTimeMS = 5000;

   while(!waitForSelfTerminateOrder(sleepTimeMS) )
   {
      // nothing to be done here currently
   } // end of while loop
}

/**
 * Send a HeartbeatMsg to mgmt.
 *
 * @return true if node and targets registration successful
 */
bool HeartbeatManager::registerNode(AbstractDatagramListener* dgramLis)
{
   static bool registrationFailureLogged = false; // to avoid log spamming

   const char* logContext = "Register node";

   App* app = Program::getApp();
   Config* cfg = app->getConfig();
   NodeStoreServers* mgmtNodes = app->getMgmtNodes();

   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if(!mgmtNode)
      return false;

   Node* localNode = app->getLocalNode();
   std::string localNodeID = localNode->getID();
   uint16_t localNodeNumID = localNode->getNumID();
   NicAddressList nicList(localNode->getNicList() );
   const BitStore* nodeFeatureFlags = localNode->getNodeFeatures();

   HeartbeatMsg msg(localNodeID.c_str(), localNodeNumID, NODETYPE_Storage, &nicList,
      nodeFeatureFlags);
   msg.setPorts(cfg->getConnStoragePortUDP(), cfg->getConnStoragePortTCP() );
   msg.setFhgfsVersion(BEEGFS_VERSION_CODE);

   bool registered = dgramLis->sendToNodeUDPwithAck(mgmtNode, &msg);

   mgmtNodes->releaseNode(&mgmtNode);

   if(registered)
      LogContext(logContext).log(Log_WARNING, "Node registration successful.");
   else
   if(!registrationFailureLogged)
   {
      LogContext(logContext).log(Log_CRITICAL, "Node registration not successful. "
         "Management node offline? Will keep on trying...");
      registrationFailureLogged = true;
   }

   return registered;
}

/**
 * Sent a MapTargetsMsg to mgmt.
 *
 * @return true if targets mapping successful.
 */
bool HeartbeatManager::registerTargetMappings()
{
   static bool registrationFailureLogged = false; // to avoid log spamming

   const char* logContext = "Register target mappings";

   App* app = Program::getApp();
   NodeStoreServers* mgmtNodes = app->getMgmtNodes();

   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if(!mgmtNode)
      return false;

   bool registered = false;

   Node* localNode = app->getLocalNode();
   uint16_t localNodeID = localNode->getNumID();
   StorageTargets* targets = Program::getApp()->getStorageTargets();
   UInt16List targetIDs;

   bool commRes;
   char* respBuf = NULL;
   NetMessage* respMsg = NULL;
   MapTargetsRespMsg* respMsgCast;

   targets->getAllTargetIDs(&targetIDs);

   MapTargetsMsg msg(&targetIDs, localNodeID);


   // connect & communicate
   commRes = MessagingTk::requestResponse(
      mgmtNode, &msg, NETMSGTYPE_MapTargetsResp, &respBuf, &respMsg);
   if(commRes)
   {
      // handle result
      respMsgCast = (MapTargetsRespMsg*)respMsg;

      FhgfsOpsErr serverErr = (FhgfsOpsErr)respMsgCast->getValue();
      if(serverErr != FhgfsOpsErr_SUCCESS)
      {
         if(!registrationFailureLogged)
         {
            LogContext(logContext).log(Log_CRITICAL, "Storage targets registration rejected. "
               "Will keep on trying... "
               "(Server Error: " + std::string(FhgfsOpsErrTk::toErrString(serverErr) ) + ")");
            registrationFailureLogged = true;
         }
      }
      else
         registered = true;

      // cleanup
      SAFE_DELETE(respMsg);
      SAFE_FREE(respBuf);
   }

   mgmtNodes->releaseNode(&mgmtNode);

   if(registered)
      LogContext(logContext).log(Log_WARNING, "Storage targets registration successful.");
   else
   if(!registrationFailureLogged)
   {
      LogContext(logContext).log(Log_CRITICAL, "Storage targets registration not successful. "
         "Management node offline? Will keep on trying...");
      registrationFailureLogged = true;
   }

   return registered;
}

