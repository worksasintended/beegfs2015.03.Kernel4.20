#include <common/net/message/nodes/GetNodesRespMsg.h>
#include <program/Program.h>
#include "GetNodesMsgEx.h"

bool GetNodesMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("GetNodes incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername(); 
   LOG_DEBUG_CONTEXT(log, Log_DEBUG, std::string("Received a GetNodesMsg from: ") + peer);

   App* app = Program::getApp();
   NodeType nodeType = getNodeType();
   
   LOG_DEBUG_CONTEXT(log, Log_SPAM, std::string("NodeType: ") + Node::nodeTypeToStr(nodeType) );


   // get corresponding node store

   AbstractNodeStore* nodes = app->getAbstractNodeStoreFromType(nodeType);
   if(!nodes)

   {
      log.logErr(std::string("Invalid node type: ") + StringTk::intToStr(nodeType) );
      return false;
   }

   // get root ID

   uint16_t rootNumID = 0;

   if(nodeType == NODETYPE_Meta)
      rootNumID = app->getMetaNodes()->getRootNodeNumID();

   // reference/retrieve all nodes from the given store and send them

   NodeList nodeList;
   
   nodes->referenceAllNodes(&nodeList);
   
   GetNodesRespMsg respMsg(rootNumID, &nodeList);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );
   
   
   nodes->releaseAllNodes(&nodeList);
   
   return true;
}

