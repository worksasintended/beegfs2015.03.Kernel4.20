#include <common/net/message/fsck/SetRootMDSRespMsg.h>
#include <program/Program.h>
#include "SetRootMDSMsgEx.h"

bool SetRootMDSMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("SetRootMDS incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername(); 
   LOG_DEBUG_CONTEXT(log, 4, std::string("Received a SetRootMDSMsg from: ") + peer);

   App* app = Program::getApp();
   NodeStoreServers* metaNodes = app->getMetaNodes();

   uint16_t newRootID = getRootNumID();

   int successResp = 1; // quasi-boolean
   
   bool setRes = metaNodes->setRootNodeNumID(newRootID, true);
   if(!setRes)
      successResp = 0;
   
   SetRootMDSRespMsg respMsg(successResp);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );
   
   return true;
}

