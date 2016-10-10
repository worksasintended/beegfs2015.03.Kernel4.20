#include <common/net/message/nodes/RemoveNodeRespMsg.h>
#include <common/net/msghelpers/MsgHelperAck.h>
#include <common/storage/StorageErrors.h>
#include <common/toolkit/MessagingTk.h>
#include <program/Program.h>
#include "RemoveNodeMsgEx.h"


bool RemoveNodeMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("RemoveNodeMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername(); 
   LOG_DEBUG_CONTEXT(log, Log_DEBUG, std::string("Received a RemoveNodeMsg from: ") + peer);
   
   LOG_DEBUG_CONTEXT(log, Log_SPAM, std::string("NodeID of removed node: ") + getNodeID() );

   App* app = Program::getApp();
   bool nodeRemoved = false;


   // delete node based on type

   if(getNodeType() == NODETYPE_Client)
   {
      NodeStoreClients* nodes = app->getClientNodes();
      bool delRes = nodes->deleteNode(getNodeID() );
      if(delRes)
         nodeRemoved = true;
   }
   else
   { // not a client => server node
      NodeStoreServers* nodes = app->getServerStoreFromType(getNodeType() );
      if(!nodes)
      { // invalid node type
         log.logErr(
            "Invalid node type: " + StringTk::intToStr(getNodeType() ) + "; "
            "NodeID: " + getNodeID() );
      }
      else
      { // found the corresponding server store => delete by ID

         UInt16List targetIDs;
         TargetMapper* mapper = app->getTargetMapper();
         mapper->getTargetsByNode(getNodeNumID(), targetIDs);

         bool delRes = nodes->deleteNode(getNodeNumID() );
         if(delRes)
            nodeRemoved = true;

         if(app->getConfig()->getQuotaEnableEnforcement() )
         {
            for(UInt16ListIter iter = targetIDs.begin(); iter != targetIDs.end(); iter++)
               app->getQuotaManager()->removeTargetFromQuotaDataStores(*iter);
         }
      }
   }

   // log message and notification of other nodes
   
   if(nodeRemoved)
   {
      log.log(Log_WARNING, "Node removed: " +
         Node::getNodeIDWithTypeStr(getNodeID(), getNodeNumID(), getNodeType() ) );

      if(getNodeType() != NODETYPE_Client) // don't print stats for every client unmount
         log.log(Log_NOTICE, std::string("Number of nodes: ") +
            "Meta: " + StringTk::uintToStr(app->getMetaNodes()->getSize() ) + "; "
            "Storage: " + StringTk::uintToStr(app->getStorageNodes()->getSize() ) + "; "
            "Client: " + StringTk::uintToStr(app->getClientNodes()->getSize() ) + "; "
            "Mgmt: " + StringTk::uintToStr(app->getMgmtNodes()->getSize() ) );

      // force update of capacity pools (especially to update buddy mirror capacity pool)
      app->getInternodeSyncer()->setForcePoolsUpdate();

      app->getHeartbeatMgr()->notifyAsyncRemovedNode(
         getNodeID(), getNodeNumID(), getNodeType() );
   }

   // send response

   if(!MsgHelperAck::respondToAckRequest(this, fromAddr, sock,
      respBuf, bufLen, app->getDatagramListener() ) )
   {
      RemoveNodeRespMsg respMsg(nodeRemoved ? FhgfsOpsErr_SUCCESS : FhgfsOpsErr_UNKNOWNNODE);
      respMsg.serialize(respBuf, bufLen);

      if(fromAddr)
      { // datagram => sync via dgramLis send method
         app->getDatagramListener()->sendto(respBuf, respMsg.getMsgLength(), 0,
            (struct sockaddr*)fromAddr, sizeof(*fromAddr) );
      }
      else
         sock->sendto(respBuf, respMsg.getMsgLength(), 0, NULL, 0);
   }

   return true;
}

