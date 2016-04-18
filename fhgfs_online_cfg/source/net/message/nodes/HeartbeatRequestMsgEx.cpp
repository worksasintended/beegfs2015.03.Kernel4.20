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
   DatagramListener* dgramLis = app->getDatagramListener();

   Node* localNode = app->getLocalNode();
   std::string localNodeID = localNode->getID();
   NicAddressList nicList(localNode->getNicList() );
   const BitStore* nodeFeatureFlags = localNode->getNodeFeatures();


   HeartbeatMsg hbMsg(localNodeID.c_str(), 0, NODETYPE_Client, &nicList, nodeFeatureFlags);
   hbMsg.setPorts(dgramLis->getUDPPort(), 0);

   hbMsg.serialize(respBuf, bufLen);

   if(fromAddr)
   { // datagram => sync via dgramLis send method
      dgramLis->sendto(respBuf, hbMsg.getMsgLength(), 0,
         (struct sockaddr*)fromAddr, sizeof(*fromAddr) );
   }
   else
      sock->sendto(respBuf, hbMsg.getMsgLength(), 0, NULL, 0);

   return true;
}

