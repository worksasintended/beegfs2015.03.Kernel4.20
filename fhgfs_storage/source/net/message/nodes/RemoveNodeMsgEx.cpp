#include <common/net/message/nodes/RemoveNodeRespMsg.h>
#include <common/net/msghelpers/MsgHelperAck.h>
#include <common/toolkit/MessagingTk.h>
#include <program/Program.h>
#include "RemoveNodeMsgEx.h"


bool RemoveNodeMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("RemoveNodeMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername(); 
   LOG_DEBUG_CONTEXT(log, Log_DEBUG, std::string("Received a RemoveNodeMsg from: ") + peer);
   
   LOG_DEBUG_CONTEXT(log, Log_SPAM, std::string("NodeID of removed node: ") +
      getNodeID() + " / " + StringTk::uintToStr(getNodeNumID() ) );

   App* app = Program::getApp();
   bool logNodeRemoved = false;
   bool logNumNodes = false;


   switch(getNodeType() )
   {
      case NODETYPE_Storage:
      {
         NodeStoreServersEx* storageNodes = app->getStorageNodes();
         bool delRes = storageNodes->deleteNode(getNodeNumID() );
         if(delRes)
         {
            logNodeRemoved = true;
            logNumNodes = true;
         }
      } break;

      case NODETYPE_Client:
      {
         // note: we ignore this push info, but handle client removal via heartbeatmgr poll sync
      } break;

      default: break;
   }

   // log

   if(logNodeRemoved)
   {
      log.log(Log_WARNING, "Node removed: " +
         Node::getNodeIDWithTypeStr(getNodeID(), getNodeNumID(), getNodeType() ) );
   }

   if(logNumNodes)
   {
      log.log(Log_WARNING, std::string("Number of nodes in the system: ") +
         "Storage: " + StringTk::intToStr(app->getStorageNodes()->getSize() ) );
   }

   // send response

   if(!MsgHelperAck::respondToAckRequest(this, fromAddr, sock,
      respBuf, bufLen, app->getDatagramListener() ) )
   {
      RemoveNodeRespMsg respMsg(0);
      respMsg.serialize(respBuf, bufLen);

      if(fromAddr)
      { // datagram => sync via dgramLis send method
         app->getDatagramListener()->sendto(respBuf, respMsg.getMsgLength(), 0,
            (struct sockaddr*)fromAddr, sizeof(*fromAddr) );
      }
      else
         sock->sendto(respBuf, respMsg.getMsgLength(), 0, NULL, 0);
   }

   app->getNodeOpStats()->updateNodeOp(
      sock->getPeerIP(), StorageOpCounter_REMOVENODE, getMsgHeaderUserID() );

   return true;
}

