#include <common/net/message/nodes/HeartbeatMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <program/Program.h>
#include "HeartbeatRequestMsgEx.h"

bool HeartbeatRequestMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("HeartbeatRequest incoming");

   //std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername(); 
   //LOG_DEBUG_CONTEXT(log, 5, std::string("Received a HeartbeatRequestMsg from: ") + peer);
   
   App* app = Program::getApp();
   Config* cfg = app->getConfig();
   
   Node* localNode = app->getLocalNode();
   std::string localNodeID = localNode->getID();
   uint16_t localNodeNumID = localNode->getNumID();
   NicAddressList nicList(localNode->getNicList() );
   const BitStore* nodeFeatureFlags = localNode->getNodeFeatures();
   
   HeartbeatMsg hbMsg(localNodeID.c_str(), localNodeNumID, NODETYPE_Storage, &nicList,
      nodeFeatureFlags);
   hbMsg.setPorts(cfg->getConnStoragePortUDP(), cfg->getConnStoragePortTCP() );
   hbMsg.setFhgfsVersion(BEEGFS_VERSION_CODE);
   
   hbMsg.serialize(respBuf, bufLen);
      
   if(fromAddr)
   { // datagram => sync via dgramLis send method
      app->getDatagramListener()->sendto(respBuf, hbMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(*fromAddr) );
   }
   else
      sock->sendto(respBuf, hbMsg.getMsgLength(), 0, NULL, 0);

   app->getNodeOpStats()->updateNodeOp(
      sock->getPeerIP(), StorageOpCounter_HEARTBEAT, getMsgHeaderUserID() );

   return true;
}

