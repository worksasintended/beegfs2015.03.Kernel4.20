#include "GetNodeInfoWork.h"
#include <program/Program.h>
#include <common/toolkit/StringTk.h>


void GetNodeInfoWork::process(char* bufIn, unsigned bufInLen, char* bufOut,
   unsigned bufOutLen)
{
   App* app = Program::getApp();

   GeneralNodeInfo info;

   NodeStoreServers* nodes = app->getServerStoreFromType(nodeType);
   if(!nodes)
   {
      log.logErr("Invalid node type: " + Node::nodeTypeToStr(nodeType) );
      return;
   }

   Node* node = nodes->referenceNode(nodeID);
   if(!node)
   {
      log.logErr("Unknown nodeID: " + StringTk::uintToStr(nodeID) );
      return;
   }

   if(!sendMsg(node, &info) )
   {
      nodes->releaseNode(&node);
      return;
   }

   switch(nodeType)
   {
      case NODETYPE_Mgmt:
      {
         ( (MgmtNodeEx*)node)->setGeneralInformation(info);
      } break;

      case NODETYPE_Meta:
      {
         ( (MetaNodeEx*)node)->setGeneralInformation(info);
      } break;

      case NODETYPE_Storage:
      {
         ( (StorageNodeEx*)node)->setGeneralInformation(info);
      } break;

      default: break;
   }

   nodes->releaseNode(&node);
}

bool GetNodeInfoWork::sendMsg(Node *node, GeneralNodeInfo *outInfo)
{
   bool commRes;
   GetNodeInfoMsg getNodeInfoMsg;
   char *outBuf = NULL;
   NetMessage *respMsg = NULL;

   // send a GetNodeInfoMsg to the node

   commRes = MessagingTk::requestResponse(node, &getNodeInfoMsg,
      NETMSGTYPE_GetNodeInfoResp, &outBuf, &respMsg);

   if(!commRes)
   { // comm failed
      log.logErr(std::string("Communication failed with node: ") +
         node->getNodeIDWithTypeStr() );
      return false;
   }

   // comm succeeded

   GetNodeInfoRespMsg* getNodeInfoRespMsg = (GetNodeInfoRespMsg*)respMsg;
   *outInfo = getNodeInfoRespMsg->getInfo();

   delete(respMsg);
   free(outBuf);

   return true;
}
