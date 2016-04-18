#include <app/App.h>
#include <common/net/message/storage/mirroring/SetMetadataMirroringMsg.h>
#include <common/net/message/storage/mirroring/SetMetadataMirroringRespMsg.h>
#include <common/toolkit/MetadataTk.h>
#include <common/toolkit/NodesTk.h>
#include <program/Program.h>
#include "ModeSetMetadataMirroring.h"


#define MODESETMDMIRRORING_ARG_UNMOUNTEDPATH  "--unmounted"


int ModeSetMetadataMirroring::execute()
{
   const int mgmtTimeoutMS = 2500;

   int retVal = APPCODE_RUNTIME_ERROR;

   App* app = Program::getApp();
   AbstractDatagramListener* dgramLis = app->getDatagramListener();
   NodeStoreServers* mgmtNodes = app->getMgmtNodes();
   NodeStoreServers* metaNodes = app->getMetaNodes();
   std::string mgmtHost = app->getConfig()->getSysMgmtdHost();
   unsigned short mgmtPortUDP = app->getConfig()->getConnMgmtdPortUDP();
   StringMap* cfg = app->getConfig()->getUnknownConfigArgs();

   NodeList metaNodesList;
   uint16_t rootNodeID;
   StringMapIter iter;

   // check privileges
   if(!ModeHelper::checkRootPrivileges() )
      return APPCODE_RUNTIME_ERROR;

   // check arguments

   bool useMountedPath = true;
   iter = cfg->find(MODESETMDMIRRORING_ARG_UNMOUNTEDPATH);
   if(iter != cfg->end() )
   {
      useMountedPath = false;
      cfg->erase(iter);
   }

   if(cfg->empty() )
   {
      std::cerr << "No path specified." << std::endl;
      return APPCODE_RUNTIME_ERROR;
   }

   std::string pathStr = cfg->begin()->first;
   cfg->erase(cfg->begin() );


   if(ModeHelper::checkInvalidArgs(cfg) )
      return APPCODE_INVALID_CONFIG;


   if(useMountedPath)
   { // make path relative to mount root
      std::string mountRoot;
      std::string relativePath;

      bool pathRelativeRes = ModeHelper::makePathRelativeToMount(
         pathStr, false, false, &mountRoot, &relativePath);
      if(!pathRelativeRes)
         return APPCODE_RUNTIME_ERROR;

      pathStr = relativePath;

      std::cout << "Mount: '" << mountRoot << "'; Path: '" << relativePath << "'" << std::endl;
   }

   // check mgmt node
   if(!NodesTk::waitForMgmtHeartbeat(
      NULL, dgramLis, mgmtNodes, mgmtHost, mgmtPortUDP, mgmtTimeoutMS) )
   {
      std::cerr << "Management node communication failed: " << mgmtHost << std::endl;
      return APPCODE_RUNTIME_ERROR;
   }

   // download nodes
   Node* mgmtNode = mgmtNodes->referenceFirstNode();

   if(!NodesTk::downloadNodes(mgmtNode, NODETYPE_Meta, &metaNodesList, false, &rootNodeID) )
   {
      std::cerr << "Node download failed." << std::endl;
      mgmtNodes->releaseNode(&mgmtNode);

      return APPCODE_RUNTIME_ERROR;
   }

   NodesTk::applyLocalNicCapsToList(app->getLocalNode(), &metaNodesList);
   NodesTk::moveNodesFromListToStore(&metaNodesList, metaNodes);
   metaNodes->setRootNodeNumID(rootNodeID, false);

   // find owner node

   Path path(pathStr);
   path.setAbsolute(true);

   Node* ownerNode = NULL;
   EntryInfo entryInfo;

   FhgfsOpsErr findRes = MetadataTk::referenceOwner(&path, false, metaNodes, &ownerNode, &entryInfo);
   if(findRes != FhgfsOpsErr_SUCCESS)
   {
      std::cerr << "Unable to find metadata node for path: " << pathStr << std::endl;
      std::cerr << "Error: " << FhgfsOpsErrTk::toErrString(findRes) << std::endl;
      retVal = APPCODE_RUNTIME_ERROR;
      goto cleanup_mgmt;
   }

   // apply mirroring
   if(setMirroring(ownerNode, &entryInfo) )
      retVal = APPCODE_NO_ERROR;

   // cleanup
   metaNodes->releaseNode(&ownerNode);

cleanup_mgmt:
   mgmtNodes->releaseNode(&mgmtNode);

   return retVal;
}

void ModeSetMetadataMirroring::printHelp()
{
   std::cout << "MODE ARGUMENTS:" << std::endl;
   std::cout << " Mandatory:" << std::endl;
   std::cout << "  <path>                  Path to a directory." << std::endl;
   std::cout << std::endl;
   std::cout << "USAGE:" << std::endl;
   std::cout << " This mode enables metadata mirroring for the given directory. (The mirroring" << std::endl;
   std::cout << " setting will only be effective for the given directory and new files or" << std::endl;
   std::cout << " subdirectories of this directory.)" << std::endl;
   std::cout << std::endl;
   std::cout << "Note: Client side ACLs and extended attributes will not be mirrored." << std::endl;
   std::cout << std::endl;
   std::cout << " Example: Enable mirroring for directory \"/mnt/beegfs/mydir\"" << std::endl;
   std::cout << "  $ beegfs-ctl --mirrormd /mnt/beegfs/mydir" << std::endl;
}

bool ModeSetMetadataMirroring::setMirroring(Node* ownerNode, EntryInfo* entryInfo)
{
   bool retVal = false;

   bool commRes;
   char* respBuf = NULL;
   NetMessage* respMsg = NULL;
   SetMetadataMirroringRespMsg* respMsgCast;

   FhgfsOpsErr setRemoteRes;

   SetMetadataMirroringMsg setMsg(entryInfo);

   // request/response
   commRes = MessagingTk::requestResponse(
      ownerNode, &setMsg, NETMSGTYPE_SetMetadataMirroringResp, &respBuf, &respMsg);
   if(!commRes)
   {
      std::cerr << "Communication with server failed: " << ownerNode->getNodeIDWithTypeStr() <<
         std::endl;
      goto err_cleanup;
   }

   respMsgCast = (SetMetadataMirroringRespMsg*)respMsg;

   setRemoteRes = respMsgCast->getResult();
   if(setRemoteRes != FhgfsOpsErr_SUCCESS)
   {
      std::cerr << "Operation failed on server: " << ownerNode->getNodeIDWithTypeStr() << "; " <<
         "Error: " << FhgfsOpsErrTk::toErrString(setRemoteRes) << std::endl;
      goto err_cleanup;
   }

   std::cout << "Operation succeeded." << std::endl;

   retVal = true;

err_cleanup:
   SAFE_DELETE(respMsg);
   SAFE_FREE(respBuf);

   return retVal;
}
