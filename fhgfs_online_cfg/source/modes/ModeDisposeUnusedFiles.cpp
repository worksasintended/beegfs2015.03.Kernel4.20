#include <app/App.h>
#include <common/net/message/storage/listing/ListDirFromOffsetMsg.h>
#include <common/net/message/storage/listing/ListDirFromOffsetRespMsg.h>
#include <common/net/message/storage/creating/UnlinkFileMsg.h>
#include <common/net/message/storage/creating/UnlinkFileRespMsg.h>
#include <common/storage/Metadata.h>
#include <common/storage/StorageDefinitions.h>
#include <common/toolkit/MetadataTk.h>
#include <common/toolkit/NodesTk.h>
#include <common/toolkit/UnitTk.h>
#include <program/Program.h>
#include "ModeDisposeUnusedFiles.h"


#define NODEINFO_INDENTATION_STR   "   "

#define MODEDISPOSEUNUSED_ARG_PRINTNODES  "--printnodes"
#define MODEDISPOSEUNUSED_ARG_PRINTFILES  "--printfiles"
#define MODEDISPOSEUNUSED_ARG_PRINTSTATS  "--printstats"
#define MODEDISPOSEUNUSED_ARG_DISPOSE     "--dispose"


int ModeDisposeUnusedFiles::execute()
{
   const int mgmtTimeoutMS = 2500;

   int retVal = APPCODE_RUNTIME_ERROR;

   App* app = Program::getApp();
   DatagramListener* dgramLis = app->getDatagramListener();
   NodeStoreServers* mgmtNodes = app->getMgmtNodes();
   NodeStoreServers* metaNodes = app->getMetaNodes();
   std::string mgmtHost = app->getConfig()->getSysMgmtdHost();
   unsigned short mgmtPortUDP = app->getConfig()->getConnMgmtdPortUDP();

   NodeList metaNodesList;
   uint16_t rootNodeID;

   // check privileges
   if(!ModeHelper::checkRootPrivileges() )
      return APPCODE_RUNTIME_ERROR;

   // check arguments
   if(!readConfig() )
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
      retVal = APPCODE_RUNTIME_ERROR;
      goto cleanup_mgmt;
   }

   NodesTk::applyLocalNicCapsToList(app->getLocalNode(), &metaNodesList);
   NodesTk::moveNodesFromListToStore(&metaNodesList, metaNodes);
   metaNodes->setRootNodeNumID(rootNodeID, false);


   // walk over nodes and files
   handleNodes(metaNodes);


   if(cfgPrintStats)
   {
      std::cout << "===" << std::endl;
      std::cout << "Disposed / Total: " << numUnlinkedTotal << "/" << numEntriesTotal << std::endl;
   }

   retVal = APPCODE_NO_ERROR;

cleanup_mgmt:
   mgmtNodes->releaseNode(&mgmtNode);

   return retVal;
}

void ModeDisposeUnusedFiles::printHelp()
{
   std::cout << std::endl;
   std::cout << "MODE ARGUMENTS:" << std::endl;
   std::cout << " Optional:" << std::endl;
   std::cout << "  --printnodes            Print scanned metadata nodes." << std::endl;
   std::cout << "  --printfiles            Print file IDs that are marked for disposal." << std::endl;
   std::cout << "  --printstats            Print statistics (number of files, ...)." << std::endl;
   std::cout << "  --dispose               Try to remove the marked files." << std::endl;
   std::cout << std::endl;
   std::cout << "USAGE:" << std::endl;
   std::cout << " This mode purges remains of old files, which have been unlinked by the user" << std::endl;
   std::cout << " and are no longer needed. (There is normally no need to use this mode.)" << std::endl;
   std::cout << std::endl;
   std::cout << " Example: List disposable files without actually purging them" << std::endl;
   std::cout << "  $ beegfs-ctl --disposeunused --printfiles --printstats" << std::endl;
}

bool ModeDisposeUnusedFiles::readConfig()
{
   App* app = Program::getApp();
   StringMap* cfg = app->getConfig()->getUnknownConfigArgs();
   StringMapIter iter;

   iter = cfg->find(MODEDISPOSEUNUSED_ARG_PRINTNODES);
   if(iter != cfg->end() )
   {
      cfgPrintNodes = true;
      cfg->erase(iter);
   }

   iter = cfg->find(MODEDISPOSEUNUSED_ARG_PRINTFILES);
   if(iter != cfg->end() )
   {
      cfgPrintFiles = true;
      cfg->erase(iter);
   }

   iter = cfg->find(MODEDISPOSEUNUSED_ARG_PRINTSTATS);
   if(iter != cfg->end() )
   {
      cfgPrintStats = true;
      cfg->erase(iter);
   }

   iter = cfg->find(MODEDISPOSEUNUSED_ARG_DISPOSE);
   if(iter != cfg->end() )
   {
      cfgUnlinkFiles = true;
      cfg->erase(iter);
   }

   if(ModeHelper::checkInvalidArgs(cfg) )
      return false;

   // set default options, if noting was selected by the user
   if(!cfgPrintNodes && !cfgPrintFiles && !cfgPrintStats && !cfgUnlinkFiles)
   {
      cfgPrintNodes = true;
      cfgPrintStats = true;
   }

   return true;
}

void ModeDisposeUnusedFiles::handleNodes(NodeStoreServers* metaNodes)
{
   Node* node;

   node = metaNodes->referenceFirstNode();
   while(node)
   {
      uint16_t currentNodeID = node->getNumID();

      if(cfgPrintNodes)
         std::cout << "Node: " << currentNodeID << std::endl;

      bool filesRes = getAndHandleFiles(node);
      if(!filesRes)
         std::cerr << "Metadata server encoutered an error: " << currentNodeID << std::endl;

      // prepare next round
      node = metaNodes->referenceNextNodeAndReleaseOld(node);

      if(cfgPrintNodes && node)
         std::cout << "---" << std::endl << std::endl;
   }

}

bool ModeDisposeUnusedFiles::getAndHandleFiles(Node* node)
{
   bool retVal = true;

   size_t numEntriesReceived = 0; // received during all RPC rounds (for this particular server)
   size_t numEntriesThisRound = 0; // received during last RPC round
   bool commRes = true;
   char* respBuf = NULL;
   NetMessage* respMsg = NULL;
   ListDirFromOffsetRespMsg* respMsgCast;

   FhgfsOpsErr listRes = FhgfsOpsErr_SUCCESS;
   unsigned maxOutNames = 50;
   StringList entryNames;
   uint64_t currentServerOffset = 0;

   do
   {
      uint16_t ownerNodeID      = node->getNumID();
      std::string parentEntryID = "";
      std::string entryID       = META_DISPOSALDIR_ID_STR;
      std::string fileName      = META_DISPOSALDIR_ID_STR;
      EntryInfo entryInfo(ownerNodeID, parentEntryID, entryID,  fileName,
         DirEntryType_DIRECTORY,  0);

      ListDirFromOffsetMsg listMsg(&entryInfo, currentServerOffset, maxOutNames, true);

      // request/response
      commRes = MessagingTk::requestResponse(
         node, &listMsg, NETMSGTYPE_ListDirFromOffsetResp, &respBuf, &respMsg);
      if(!commRes)
      {
         std::cerr << "Communication error" << std::endl;

         retVal = false;
         goto err_cleanup;
      }

      respMsgCast = (ListDirFromOffsetRespMsg*)respMsg;

      listRes = (FhgfsOpsErr)respMsgCast->getResult();
      if(listRes != FhgfsOpsErr_SUCCESS)
      {
         std::cerr << "Node encountered an error: " << FhgfsOpsErrTk::toErrString(listRes) <<
            std::endl;

         retVal = false;
         goto err_cleanup;
      }

      entryNames.clear();
      respMsgCast->parseNames(&entryNames);

      numEntriesThisRound = entryNames.size();
      currentServerOffset = respMsgCast->getNewServerOffset();

      handleEntries(node, entryNames);

      numEntriesReceived += numEntriesThisRound;


   err_cleanup:
      SAFE_DELETE(respMsg);
      SAFE_FREE(respBuf);

   } while(retVal && (numEntriesThisRound == maxOutNames) );


   numEntriesTotal += numEntriesReceived;

   if(cfgPrintStats)
   {
      if(cfgPrintNodes) // indent
         std::cout << NODEINFO_INDENTATION_STR << "#Entries: " << numEntriesReceived << std::endl;
      else
         std::cout << "#Entries: " << numEntriesReceived << std::endl;
   }

   return retVal;
}

/**
 * Handle received entries (e.g. print, unlink)
 */
void ModeDisposeUnusedFiles::handleEntries(Node* node, StringList& entryNames)
{
   for(StringListIter iter = entryNames.begin(); iter != entryNames.end(); iter++)
   {
      std::string entry = *iter;

      if(cfgPrintFiles)
      {
         if(cfgPrintNodes) // indent
            std::cout << NODEINFO_INDENTATION_STR << entry << std::endl;
         else
            std::cout << entry << std::endl;
      }

      if(cfgUnlinkFiles)
         unlinkEntry(node, entry);
   }
}

bool ModeDisposeUnusedFiles::unlinkEntry(Node* node, std::string entryName)
{
   bool retVal = false;

   bool commRes;
   char* respBuf = NULL;
   NetMessage* respMsg = NULL;
   UnlinkFileRespMsg* respMsgCast;

   FhgfsOpsErr unlinkRes;

   uint16_t ownerNodeID    = node->getNumID();
   std::string parentEntry;
   std::string entryID     = META_DISPOSALDIR_ID_STR;
   std::string fileName    = META_DISPOSALDIR_ID_STR;
   DirEntryType entryType  = DirEntryType_DIRECTORY;
   int flags               = 0;

   EntryInfo parentInfo(ownerNodeID, parentEntry, entryID, fileName, entryType, flags);

   UnlinkFileMsg getInfoMsg(&parentInfo, entryName);

   // request/response
   commRes = MessagingTk::requestResponse(
      node, &getInfoMsg, NETMSGTYPE_UnlinkFileResp, &respBuf, &respMsg);
   if(!commRes)
   {
      std::cerr << "[" << entryName <<  "] Communication error" << std::endl;

      goto err_cleanup;
   }

   respMsgCast = (UnlinkFileRespMsg*)respMsg;

   unlinkRes = (FhgfsOpsErr)respMsgCast->getValue();
   if(unlinkRes == FhgfsOpsErr_INUSE)
   {
      std::cerr << "[" << entryName <<  "] File still in use" << std::endl;
   }
   else
   if(unlinkRes != FhgfsOpsErr_SUCCESS)
   {
      std::cerr << "[" << entryName << "] Node encountered an error: " <<
         FhgfsOpsErrTk::toErrString(unlinkRes) << std::endl;
      goto err_cleanup;
   }
   else
   { // unlinked
      numUnlinkedTotal++;
   }


   retVal = true;

err_cleanup:
   SAFE_DELETE(respMsg);
   SAFE_FREE(respBuf);

   return retVal;
}
