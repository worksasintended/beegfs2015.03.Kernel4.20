#include <app/App.h>
#include <common/toolkit/MetadataTk.h>
#include <common/toolkit/NodesTk.h>
#include <common/toolkit/TimeAbs.h>
#include <common/toolkit/UnitTk.h>
#include <program/Program.h>
#include "ModeRefreshAllEntries.h"


#define MODEREFRESHALLENTRIES_ARG_NODEID     "--nodeid"
#define MODEREFRESHALLENTRIES_COMMAND_START  "--start"
#define MODEREFRESHALLENTRIES_COMMAND_STOP   "--stop"

int ModeRefreshAllEntries::execute()
{
   const int mgmtTimeoutMS = 2500;

   int retVal = APPCODE_RUNTIME_ERROR;

   App* app = Program::getApp();
   DatagramListener* dgramLis = app->getDatagramListener();
   NodeStore* mgmtNodes = app->getMgmtNodes();
   std::string mgmtHost = app->getConfig()->getSysMgmtdHost();
   unsigned short mgmtPortUDP = app->getConfig()->getConnMgmtdPortUDP();
   StringMap* cfg = app->getConfig()->getUnknownConfigArgs();

   uint16_t nodeID;
   NodeList nodes;
   std::string commandStr;
   RefresherControlType controlCommand;
   bool commRes;

   StringMapIter iter;

   // check privileges

   if(!ModeHelper::checkRootPrivileges() )
      return APPCODE_RUNTIME_ERROR;

   // check arguments

   iter = cfg->find(MODEREFRESHALLENTRIES_ARG_NODEID);
   if(iter == cfg->end() )
   { // nodeID not specified
      std::cerr << "NodeID not specified." << std::endl;
      return APPCODE_INVALID_CONFIG;
   }
   else
   {
      bool isNumericRes = StringTk::isNumeric(iter->second);
      if(!isNumericRes)
      {
         std::cerr << "Invalid nodeID given (must be numeric): " << iter->second << std::endl;
         return APPCODE_INVALID_CONFIG;
      }

      nodeID = StringTk::strToUInt(iter->second);
      cfg->erase(iter);
   }

   commandStr = "status"; // default to "status"
   if(!cfg->empty() )
   { // command string
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

   if(!NodesTk::downloadNodes(mgmtNode, NODETYPE_Meta, &nodes, NULL) )
   {
      std::cerr << "Node download failed." << std::endl;
      retVal = APPCODE_RUNTIME_ERROR;
      goto cleanup_mgmt;
   }

   NodesTk::applyLocalNicCapsToList(app->getLocalNode(), &nodes);
   NodesTk::moveNodesFromListToStore(&nodes, app->getMetaNodes() );


   // send command, recv and print response

   controlCommand = convertCommandStrToControlType(commandStr);

   printCommand(controlCommand); // print command for user verification

   commRes = sendCmdAndPrintResult(nodeID, controlCommand);
   if(!commRes)
   {
      retVal = APPCODE_RUNTIME_ERROR;
      goto cleanup_mgmt;
   }

   // communication succeeded

   retVal = APPCODE_NO_ERROR;


cleanup_mgmt:
   mgmtNodes->releaseNode(&mgmtNode);

   return retVal;
}

void ModeRefreshAllEntries::printHelp()
{
   std::cout << "MODE ARGUMENTS:" << std::endl;
   std::cout << " Mandatory:" << std::endl;
   std::cout << "  --nodeid=<nodeID>   Command will be sent to this metadata node." << std::endl;
   std::cout << std::endl;
   std::cout << " Optional:" << std::endl;
   std::cout << "  --<command>         Command to be sent. (Values: --start, --stop, --status)" << std::endl;
   std::cout << "                      (Default: --status)" << std::endl;
   std::cout << std::endl;
   std::cout << "USAGE:" << std::endl;
   std::cout << " This mode initiates a refresh of the metadata information of all files and" << std::endl;
   std::cout << " directories on a certain metadata server." << std::endl;
   std::cout << std::endl;
   std::cout << " Example: Start refreshing all metadata on server with ID \"5\"" << std::endl;
   std::cout << "  $ beegfs-ctl --refreshallentries --nodeid=5 --start" << std::endl;
}

/**
 * Note: Defaults to status query for unknown command string.
 */
RefresherControlType ModeRefreshAllEntries::convertCommandStrToControlType(std::string commandStr)
{
   if(!strcasecmp(commandStr.c_str(), MODEREFRESHALLENTRIES_COMMAND_START) )
      return RefresherControl_START;
   else
   if(!strcasecmp(commandStr.c_str(), MODEREFRESHALLENTRIES_COMMAND_STOP) )
      return RefresherControl_STOP;

   return RefresherControl_STATUS;
}

/**
 * Print command (as a verification output for the user).
 */
void ModeRefreshAllEntries::printCommand(RefresherControlType controlCommand)
{
   std::cout << "Command: ";

   if(controlCommand == RefresherControl_START)
      std::cout << "Start refreshing";
   else
   if(controlCommand == RefresherControl_STOP)
      std::cout << "Stop refreshing";
   else
      std::cout << "Query status";

   std::cout << std::endl;
}

void ModeRefreshAllEntries::printStatusResult(bool isRunning, uint64_t numDirsRefreshed,
   uint64_t numFilesRefreshed)
{
   std::cout << "Status:" << std::endl;
   std::cout << "+ Currently running: " << StringTk::boolToStr(isRunning) << std::endl;
   std::cout << "+ Refreshed directories: " << numDirsRefreshed << std::endl;
   std::cout << "+ Refreshed files: " << numFilesRefreshed << std::endl;
}

/**
 * @param outCmdRespStr response from the node
 * @return false if communication error occurred
 */
bool ModeRefreshAllEntries::sendCmdAndPrintResult(uint16_t nodeID,
   RefresherControlType controlCommand)
{
   bool retVal = false;
   NodeStore* nodes = Program::getApp()->getMetaNodes();

   Node* node = nodes->referenceNode(nodeID);
   if(!node)
   {
      std::cerr << "Unknown node: " << nodeID << std::endl;
      return false;
   }

   bool commRes;
   char* respBuf = NULL;
   NetMessage* respMsg = NULL;
   RefresherControlRespMsg* respMsgCast;

   RefresherControlMsg msg(controlCommand);

   // request/response
   commRes = MessagingTk::requestResponse(
      node, &msg, NETMSGTYPE_RefresherControlResp, &respBuf, &respMsg);
   if(!commRes)
   {
      std::cerr << "Failed to communicate with node: " << nodeID << std::endl;
      goto err_cleanup;
   }

   respMsgCast = (RefresherControlRespMsg*)respMsg;

   // print status
   printStatusResult(respMsgCast->getIsRunning(), respMsgCast->getNumDirsRefreshed(),
      respMsgCast->getNumFilesRefreshed() );

   retVal = true;

err_cleanup:
   SAFE_DELETE(respMsg);
   SAFE_FREE(respBuf);

   nodes->releaseNode(&node);

   return retVal;
}

