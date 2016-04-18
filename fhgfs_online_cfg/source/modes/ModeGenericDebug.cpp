#include <app/App.h>
#include <common/net/message/nodes/GenericDebugMsg.h>
#include <common/net/message/nodes/GenericDebugRespMsg.h>
#include <common/toolkit/MetadataTk.h>
#include <common/toolkit/NodesTk.h>
#include <common/toolkit/TimeAbs.h>
#include <common/toolkit/UnitTk.h>
#include <program/Program.h>
#include "ModeGenericDebug.h"


#define MODEGENERICDEBUG_ARG_NODEID     "--nodeid"


int ModeGenericDebug::execute()
{
   const int mgmtTimeoutMS = 2500;

   int retVal = APPCODE_RUNTIME_ERROR;

   App* app = Program::getApp();
   DatagramListener* dgramLis = app->getDatagramListener();
   NodeStore* mgmtNodes = app->getMgmtNodes();
   std::string mgmtHost = app->getConfig()->getSysMgmtdHost();
   unsigned short mgmtPortUDP = app->getConfig()->getConnMgmtdPortUDP();
   StringMap* cfg = app->getConfig()->getUnknownConfigArgs();

   std::string nodeIDStr;
   NodeList nodes;
   NodeStoreServers* nodeStore;
   std::string commandStr;
   std::string cmdRespStr;
   bool commRes;

   StringMapIter iter;

   // check arguments

   NodeType nodeType = ModeHelper::nodeTypeFromCfg(cfg);
   if(nodeType == NODETYPE_Invalid)
   {
      std::cerr << "Invalid or missing node type." << std::endl;
      return APPCODE_INVALID_CONFIG;
   }
   else
   if( (nodeType != NODETYPE_Storage) && (nodeType != NODETYPE_Meta) &&
       (nodeType != NODETYPE_Mgmt) )
   {
      std::cerr << "Invalid node type." << std::endl;
      return APPCODE_INVALID_CONFIG;
   }

   iter = cfg->find(MODEGENERICDEBUG_ARG_NODEID);
   if(iter == cfg->end() )
   { // nodeID not specified
      std::cerr << "NodeID not specified." << std::endl;
      return APPCODE_INVALID_CONFIG;
   }
   else
   {
      nodeIDStr = iter->second;
      cfg->erase(iter);
   }

   if(cfg->empty() )
   { // command string not specified
      std::cerr << "Command string not specified." << std::endl;
      return APPCODE_INVALID_CONFIG;
   }
   else
   {
      commandStr = cfg->begin()->first;
      cfg->erase(cfg->begin() );
   }

   if(ModeHelper::checkInvalidArgs(cfg) )
      return APPCODE_INVALID_CONFIG;


   // check mgmt node
   if(!NodesTk::waitForMgmtHeartbeat(
      NULL, dgramLis, mgmtNodes, mgmtHost, mgmtPortUDP, mgmtTimeoutMS) )
   {
      std::cerr << "Management node communication failed: " << mgmtHost << std::endl;
      return APPCODE_RUNTIME_ERROR;
   }

   Node* mgmtNode = mgmtNodes->referenceFirstNode();

   if(!NodesTk::downloadNodes(mgmtNode, nodeType, &nodes, NULL) )
   {
      std::cerr << "Node download failed." << std::endl;
      retVal = APPCODE_RUNTIME_ERROR;
      goto cleanup_mgmt;
   }

   nodeStore = app->getServerStoreFromType(nodeType);

   NodesTk::applyLocalNicCapsToList(app->getLocalNode(), &nodes);
   NodesTk::moveNodesFromListToStore(&nodes, nodeStore);


   // send command and recv response

   commRes = communicate(nodeStore, nodeIDStr, commandStr, &cmdRespStr);
   if(!commRes)
   {
      retVal = APPCODE_RUNTIME_ERROR;
      goto cleanup_mgmt;
   }

   // communication succeeded => print response

   std::cout << "Command: " << commandStr << std::endl;
   std::cout << "Response:" << std::endl;
   std::cout << cmdRespStr << std::endl;

   retVal = APPCODE_NO_ERROR;


cleanup_mgmt:
   mgmtNodes->releaseNode(&mgmtNode);

   return retVal;
}

void ModeGenericDebug::printHelp()
{
   std::cout << "MODE ARGUMENTS:" << std::endl;
   std::cout << " Optional:" << std::endl;
   std::cout << "  --nodeid=<nodeID>      Send command to node with this ID." << std::endl;
   std::cout << "  --nodetype=<nodetype>  The node type of the recipient (metadata, storage)." << std::endl;
   std::cout << std::endl;
   std::cout << "USAGE:" << std::endl;
   std::cout << " This mode sends generic debug commands to a certain node." << std::endl;
   std::cout << std::endl;
   std::cout << " Example: Execute debug command" << std::endl;
   std::cout << "  $ beegfs-ctl --genericdebug <debug_command>" << std::endl;
}

/**
 * @param nodeID can be numNodeID (fast) or stringNodeID (slow).
 * @param outCmdRespStr response from the node
 * @return false if communication error occurred
 */
bool ModeGenericDebug::communicate(NodeStoreServers* nodes, std::string nodeID, std::string cmdStr,
   std::string* outCmdRespStr)
{
   bool retVal = false;
   Node* node;

   if(StringTk::isNumeric(nodeID) )
   {
      uint16_t nodeNumID = StringTk::strToUInt(nodeID);
      node = nodes->referenceNode(nodeNumID);
   }
   else
      node = nodes->referenceNodeByStringID(nodeID);

   if(!node)
   {
      std::cerr << "Unknown node: " << nodeID << std::endl;
      return false;
   }

   bool commRes;
   char* respBuf = NULL;
   NetMessage* respMsg = NULL;
   GenericDebugRespMsg* respMsgCast;

   GenericDebugMsg msg(cmdStr.c_str() );

   // request/response
   commRes = MessagingTk::requestResponse(
      node, &msg, NETMSGTYPE_GenericDebugResp, &respBuf, &respMsg);
   if(!commRes)
   {
      std::cerr << "Failed to communicate with node: " << nodeID << std::endl;
      goto err_cleanup;
   }

   respMsgCast = (GenericDebugRespMsg*)respMsg;

   *outCmdRespStr = respMsgCast->getCmdRespStr();

   retVal = true;

err_cleanup:
   SAFE_DELETE(respMsg);
   SAFE_FREE(respBuf);

   nodes->releaseNode(&node);

   return retVal;
}

