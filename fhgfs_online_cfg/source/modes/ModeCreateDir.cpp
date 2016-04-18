#include <app/App.h>
#include <common/net/message/storage/creating/MkDirMsg.h>
#include <common/net/message/storage/creating/MkDirRespMsg.h>
#include <common/storage/striping/Raid0Pattern.h>
#include <common/toolkit/MetadataTk.h>
#include <common/toolkit/NodesTk.h>
#include <common/toolkit/UnitTk.h>
#include <program/Program.h>
#include "ModeCreateDir.h"

#define MODECREATEDIR_ARG_NODES           "--nodes"
#define MODECREATEDIR_ARG_PERMISSIONS     "--access"
#define MODECREATEDIR_ARG_USERID          "--uid"
#define MODECREATEDIR_ARG_GROUPID         "--gid"
#define MODECREATEDIR_ARG_NOMIRROR        "--nomirror"
#define MODECREATEDIR_ARG_UNMOUNTEDPATH   "--unmounted"


int ModeCreateDir::execute()
{
   const int mgmtTimeoutMS = 2500;

   int retVal = APPCODE_RUNTIME_ERROR;

   App* app = Program::getApp();
   AbstractDatagramListener* dgramLis = app->getDatagramListener();
   NodeStoreServers* mgmtNodes = app->getMgmtNodes();
   NodeStoreServers* metaNodes = app->getMetaNodes();
   NodeStoreServers* storageNodes = Program::getApp()->getStorageNodes();
   std::string mgmtHost = app->getConfig()->getSysMgmtdHost();
   unsigned short mgmtPortUDP = app->getConfig()->getConnMgmtdPortUDP();

   NodeList metaNodesList;
   uint16_t rootNodeID;
   NodeList storageNodesList;

   DirSettings settings;

   // check privileges
   if(!ModeHelper::checkRootPrivileges() )
      return APPCODE_RUNTIME_ERROR;


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

   if(!NodesTk::downloadNodes(mgmtNode, NODETYPE_Storage, &storageNodesList, false, NULL) )
   {
      std::cerr << "Node download failed." << std::endl;
      mgmtNodes->releaseNode(&mgmtNode);

      return APPCODE_RUNTIME_ERROR;
   }

   NodesTk::applyLocalNicCapsToList(app->getLocalNode(), &storageNodesList);
   NodesTk::moveNodesFromListToStore(&storageNodesList, storageNodes);


   // check arguments
   if(!initDirSettings(&settings) )
   {
      mgmtNodes->releaseNode(&mgmtNode);

      return APPCODE_RUNTIME_ERROR;
   }

   // find owner node
   Node* ownerNode = NULL;
   EntryInfo entryInfo;

   FhgfsOpsErr findRes = MetadataTk::referenceOwner(settings.path, true, metaNodes, &ownerNode,
      &entryInfo);
   if(findRes != FhgfsOpsErr_SUCCESS)
   {
      std::cerr << "Unable to find metadata node for path: " << settings.path->getPathAsStr() <<
         std::endl;
      std::cerr << "Error: " << FhgfsOpsErrTk::toErrString(findRes) << std::endl;
      retVal = APPCODE_RUNTIME_ERROR;
      goto cleanup_settings;
   }

   // create the dir
   if(communicate(ownerNode, &entryInfo, &settings) )
   {
      std::cout << "Operation succeeded." << std::endl;

      retVal = APPCODE_NO_ERROR;
   }

   // cleanup
   metaNodes->releaseNode(&ownerNode);

cleanup_settings:
   freeDirSettings(&settings);

   mgmtNodes->releaseNode(&mgmtNode);

   return retVal;
}

void ModeCreateDir::printHelp()
{
   std::cout << "MODE ARGUMENTS:" << std::endl;
   std::cout << " Mandatory:" << std::endl;
   std::cout << "  <path>                 Path of the new directory." << std::endl;
   std::cout << " Optional:" << std::endl;
   std::cout << "  --nodes=<nodelist>     Comma-separated list of metadata node IDs" << std::endl;
   std::cout << "                         to choose from for the new dir." << std::endl;
   std::cout << "  --access=<mode>        The octal permissions value for user, " << std::endl;
   std::cout << "                         group and others. (Default: 0755)" << std::endl;
   std::cout << "  --uid=<userid_num>     User ID of the dir owner." << std::endl;
   std::cout << "  --gid=<groupid_num>    Group ID of the dir." << std::endl;
   std::cout << "  --nomirror             Do not mirror metadata, even if parent dir has" << std::endl;
   std::cout << "                         mirroring enabled." << std::endl;
   std::cout << std::endl;
   std::cout << "USAGE:" << std::endl;
   std::cout << " This mode creates a new directory." << std::endl;
   std::cout << std::endl;
   std::cout << " Example: Create new directory on node \"storage01\"" << std::endl;
   std::cout << "  $ beegfs-ctl --createdir --nodes=storage01 /mnt/beegfs/mydir" << std::endl;
}

/**
 * Note: Remember to call freeDirSettings() when you're done.
 */
bool ModeCreateDir::initDirSettings(DirSettings* settings)
{
   App* app = Program::getApp();
   StringMap* cfg = app->getConfig()->getUnknownConfigArgs();

   settings->preferredNodes = NULL;
   settings->path = NULL;

   StringMapIter iter;

   // parse and validate command line args

   settings->mode = 0755;
   iter = cfg->find(MODECREATEDIR_ARG_PERMISSIONS);
   if(iter != cfg->end() )
   {
      settings->mode = StringTk::strOctalToUInt(iter->second);
      settings->mode = settings->mode & 0777; // trim invalid flags from mode
      cfg->erase(iter);
   }

   settings->mode |= S_IFDIR; // make sure mode contains the "dir" flag

   settings->userID = 0;
   iter = cfg->find(MODECREATEDIR_ARG_USERID);
   if(iter != cfg->end() )
   {
      settings->userID = StringTk::strToUInt(iter->second);
      cfg->erase(iter);
   }

   settings->groupID = 0;
   iter = cfg->find(MODECREATEDIR_ARG_GROUPID);
   if(iter != cfg->end() )
   {
      settings->groupID = StringTk::strToUInt(iter->second);
      cfg->erase(iter);
   }

   settings->noMirroring = false;
   iter = cfg->find(MODECREATEDIR_ARG_NOMIRROR);
   if(iter != cfg->end() )
   {
      settings->noMirroring = true;
      cfg->erase(iter);
   }

   // parse preferred nodes

   StringList preferredNodesUntrimmed;
   settings->preferredNodes = new UInt16List();
   iter = cfg->find(MODECREATEDIR_ARG_NODES);
   if(iter != cfg->end() )
   {
      StringTk::explode(iter->second, ',', &preferredNodesUntrimmed);
      cfg->erase(iter);

      // trim nodes (copy to other list)
      for(StringListIter nodesIter = preferredNodesUntrimmed.begin();
          nodesIter != preferredNodesUntrimmed.end();
          nodesIter++)
      {
         std::string nodeTrimmedStr = StringTk::trim(*nodesIter);
         uint16_t nodeTrimmed = StringTk::strToUInt(nodeTrimmedStr);

         settings->preferredNodes->push_back(nodeTrimmed);
      }
   }

   bool useMountedPath = true;
   iter = cfg->find(MODECREATEDIR_ARG_UNMOUNTEDPATH);
   if(iter != cfg->end() )
   {
      useMountedPath = false;
      cfg->erase(iter);
   }

   iter = cfg->begin();
   if(iter == cfg->end() )
   {
      std::cerr << "No path specified." << std::endl;
      return false;
   }
   else
   {
      std::string pathStr = iter->first;

      if(useMountedPath)
      { // make path relative to mount root
         std::string mountRoot;
         std::string relativePath;

         bool pathRelativeRes = ModeHelper::makePathRelativeToMount(
            pathStr, true, false, &mountRoot, &relativePath);
         if(!pathRelativeRes)
            return false;

         pathStr = relativePath;

         std::cout << "Mount: '" << mountRoot << "'; Path: '" << relativePath << "'" << std::endl;
      }

      settings->path = new Path(pathStr);
      settings->path->setAbsolute(true);

      if(settings->path->getIsEmpty() )
      {
         std::cerr << "Invalid path specified." << std::endl;
         return false;
      }
   }

   return true;
}

void ModeCreateDir::freeDirSettings(DirSettings* settings)
{
   SAFE_DELETE(settings->preferredNodes);
   SAFE_DELETE(settings->path);
}

std::string ModeCreateDir::generateServerPath(DirSettings* settings, std::string entryID)
{
   return entryID + "/" + settings->path->getLastElem();
}

bool ModeCreateDir::communicate(Node* ownerNode, EntryInfo* parentInfo, DirSettings* settings)
{
   bool retVal = false;

   bool commRes;
   char* respBuf = NULL;
   NetMessage* respMsg = NULL;
   MkDirRespMsg* respMsgCast;

   FhgfsOpsErr mkDirRes;

   // FIXME: Switch to ioctl

   std::string newDirName = settings->path->getLastElem();

   MkDirMsg msg(parentInfo, newDirName, settings->userID, settings->groupID, settings->mode,
      settings->preferredNodes);

   if(settings->noMirroring)
      msg.addMsgHeaderFeatureFlag(MKDIRMSG_FLAG_NOMIRROR);

   // request/response
   commRes = MessagingTk::requestResponse(
      ownerNode, &msg, NETMSGTYPE_MkDirResp, &respBuf, &respMsg);
   if(!commRes)
   {
      std::cerr << "Communication with server failed: " << ownerNode->getNodeIDWithTypeStr() <<
         std::endl;
      goto err_cleanup;
   }

   respMsgCast = (MkDirRespMsg*)respMsg;

   mkDirRes = (FhgfsOpsErr)respMsgCast->getResult();
   if(mkDirRes != FhgfsOpsErr_SUCCESS)
   {
      std::cerr << "Node encountered an error: " << FhgfsOpsErrTk::toErrString(mkDirRes) <<
         std::endl;
      goto err_cleanup;
   }

   retVal = true;

err_cleanup:
   SAFE_DELETE(respMsg);
   SAFE_FREE(respBuf);

   return retVal;
}
