#include <common/app/log/LogContext.h>
#include "common/toolkit/StringTk.h"
#include "common/toolkit/MessagingTk.h"
#include <program/Program.h>
#include <nodes/NodeStoreServersEx.h>
#include <common/net/message/nodes/HeartbeatRequestMsg.h>
#include <common/net/message/nodes/HeartbeatMsg.h>
#include <components/HeartbeatManager.h>
#include "IntegrationTests.h"

bool IntegrationTests::perform()
{
   LogContext log("IntegrationTests");
   
   log.log(1, "Running integration tests...");

   bool testRes = 
      testStreamWorkers();
   
   log.log(1, "Integration tests complete");
   
   return testRes;
}

bool IntegrationTests::testStreamWorkers()
{
   bool result = true;
   
   LogContext log("testStreamWorkers");
   
   NodeStoreServers* metaNodes = Program::getApp()->getMetaNodes();
   Node* localNode = Program::getApp()->getLocalNode();
   std::string localNodeID = localNode->getID();
   uint16_t localNodeNumID = localNode->getNumID();
   uint16_t rootNodeID = metaNodes->getRootNodeNumID();
   NodeConnPool* connPool = localNode->getConnPool();
   NicAddressList nicList(localNode->getNicList() );
   const BitStore* nodeFeatureFlags = localNode->getNodeFeatures();

   // create request and response buf
   HeartbeatRequestMsg hbReqMsg;
   char* sendBuf = MessagingTk::createMsgBuf(&hbReqMsg);
   unsigned hbReqMsgLen = hbReqMsg.getMsgLength();


   HeartbeatMsg localHBMsg(
      localNodeID.c_str(), localNodeNumID, NODETYPE_Mgmt, &nicList, nodeFeatureFlags);
   localHBMsg.setRootNumID(rootNodeID);
   char* localHBMsgBuf = MessagingTk::createMsgBuf(&localHBMsg);
   
   unsigned localHBMsgLen = localHBMsg.getMsgLength();
   char* recvBuf = (char*)malloc(localHBMsgLen);

   // send & receive
   Socket* sock = NULL;
   try
   {
      sock = connPool->acquireStreamSocket();

      sock->send(sendBuf, hbReqMsgLen, 0);
      
      log.log(4, "Request sent");
      
      sock->recvExactT(recvBuf, localHBMsgLen, 0, CONN_MEDIUM_TIMEOUT);

      log.log(4, "Response received");

      log.log(4, "Invalidating connection...");
      
      //connPool->releaseStreamSocket(sock);
      connPool->invalidateStreamSocket(sock);
      
      log.log(4, "Disconnected");
      
      if(memcmp(localHBMsgBuf, recvBuf, localHBMsgLen) )
      {
         log.logErr("Heartbeat comparison failed");
         result = false;
      }
      else
         log.log(3, "Heartbeats OK");
   }
   catch(SocketConnectException& e)
   {
      log.logErr("Unable to connect to localhost");
      result = false;
   }
   catch(SocketException& e)
   {
      log.logErr(std::string("SocketException occurred: ") + e.what() );
      
      connPool->invalidateStreamSocket(sock);
      result = false;
   }
   
   free(sendBuf);
   free(localHBMsgBuf);
   free(recvBuf);
   
   return result;
}










