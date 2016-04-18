#include <app/App.h>
#include <common/net/message/storage/attribs/GetEntryInfoMsg.h>
#include <common/net/message/storage/attribs/GetEntryInfoRespMsg.h>
#include <common/storage/striping/Raid0Pattern.h>
#include <common/storage/striping/Raid10Pattern.h>
#include <common/toolkit/MetaStorageTk.h>
#include <common/toolkit/MetadataTk.h>
#include <common/toolkit/NodesTk.h>
#include <common/toolkit/UnitTk.h>
#include <program/Program.h>
#include "ModeGetEntryInfo.h"


#define MODEGETENTRYINFO_ARG_UNMOUNTEDPATH          "--unmounted"
#define MODEGETENTRYINFO_ARG_READFROMSTDIN          "-"
#define MODEGETENTRYINFO_ARG_VERBOSE                "--verbose"
#define MODEGETENTRYINFO_ARG_DONTSHOWTARGETMAPPINGS "--nomappings"


int ModeGetEntryInfo::execute()
{
   const int mgmtTimeoutMS = 2500;

   int retVal = APPCODE_NO_ERROR;

   App* app = Program::getApp();
   DatagramListener* dgramLis = app->getDatagramListener();
   NodeStoreServers* mgmtNodes = app->getMgmtNodes();
   NodeStoreServers* metaNodes = app->getMetaNodes();
   std::string mgmtHost = app->getConfig()->getSysMgmtdHost();
   unsigned short mgmtPortUDP = app->getConfig()->getConnMgmtdPortUDP();
   StringMap* cfg = app->getConfig()->getUnknownConfigArgs();

   NodeList metaNodesList;
   uint16_t rootNodeID;
   StringMapIter iter;

   // check arguments

   iter = cfg->find(MODEGETENTRYINFO_ARG_UNMOUNTEDPATH);
   if(iter != cfg->end() )
   {
      cfgUseMountedPath = false;
      cfg->erase(iter);
   }

   iter = cfg->find(MODEGETENTRYINFO_ARG_VERBOSE);
   if(iter != cfg->end() )
   {
      cfgVerbose = true;
      cfg->erase(iter);
   }

   iter = cfg->find(MODEGETENTRYINFO_ARG_DONTSHOWTARGETMAPPINGS);
   if(iter != cfg->end() )
   {
      cfgShowTargetMappings = false;
      cfg->erase(iter);
   }

   if(cfg->empty() )
   {
      std::cerr << "No path specified." << std::endl;
      return APPCODE_RUNTIME_ERROR;
   }

   this->cfgPathStr = cfg->begin()->first;
   cfg->erase(cfg->begin() );

   if(this->cfgPathStr == MODEGETENTRYINFO_ARG_READFROMSTDIN)
      cfgReadFromStdin = true;


   if(ModeHelper::checkInvalidArgs(cfg) )
      return APPCODE_INVALID_CONFIG;


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

   // download storage servers and target mappings (if necessary)
   if(cfgShowTargetMappings)
   {
      TargetMapper* targetMapper = app->getTargetMapper();

      UInt16List mappedTargetIDs;
      UInt16List mappedNodeIDs;

      NodeList storageNodes;

      if(!NodesTk::downloadTargetMappings(mgmtNode, &mappedTargetIDs, &mappedNodeIDs, false) ||
         !NodesTk::downloadNodes(mgmtNode, NODETYPE_Storage, &storageNodes, false, NULL) )
      {
         std::cerr << "Node download failed." << std::endl;
         retVal = APPCODE_RUNTIME_ERROR;
         goto cleanup_mgmt;
      }

      targetMapper->syncTargetsFromLists(mappedTargetIDs, mappedNodeIDs);

      NodesTk::applyLocalNicCapsToList(app->getLocalNode(), &storageNodes);
      NodesTk::moveNodesFromListToStore(&storageNodes, app->getStorageNodes() );
   }

   if(cfgReadFromStdin)
   { // read first path from stdin
      this->cfgPathStr.clear();
      getline(std::cin, this->cfgPathStr);

      if(this->cfgPathStr.empty() )
      {
         retVal = APPCODE_NO_ERROR; // no files given is not an error
         goto cleanup_mgmt;
      }
   }

   do /* loop while we get paths from stdin (or terminate if stdin reading disabled) */
   {
      std::string relPathStr;
      std::string mountRoot;
      bool getInfoRes;

      if(this->cfgUseMountedPath)
      { // make relPath relative to mount root

         bool pathRelativeRes = ModeHelper::makePathRelativeToMount(
            this->cfgPathStr, false, false, &mountRoot, &relPathStr);
         if(!pathRelativeRes)
         {
            retVal = APPCODE_RUNTIME_ERROR;
            goto cleanup_mgmt;
         }
      }
      else
         relPathStr = this->cfgPathStr;

      // find owner node

      Path relPath(relPathStr);
      relPath.setAbsolute(true);

      Node* ownerNode = NULL;
      EntryInfo entryInfo;
      PathInfo pathInfo;
      int isFile;

      FhgfsOpsErr findRes = MetadataTk::referenceOwner(
         &relPath, false, metaNodes, &ownerNode, &entryInfo);

      if(findRes != FhgfsOpsErr_SUCCESS)
      {
         std::cerr << "Unable to find metadata node for path: " << this->cfgPathStr << std::endl;
         std::cerr << "Error: " << FhgfsOpsErrTk::toErrString(findRes) << std::endl;
         retVal = APPCODE_RUNTIME_ERROR;

         if(!cfgReadFromStdin)
            break;

         goto finish_this_entry;
      }

      isFile = DirEntryType_ISFILE(entryInfo.getEntryType() );

      // print some basic info

      std::cout << "Path: " << relPathStr << std::endl;

      if (this->cfgUseMountedPath)
         std::cout << "Mount: " << mountRoot << std::endl;

      std::cout << "EntryID: " << entryInfo.getEntryID() << std::endl;
      if (!isFile && this->cfgVerbose)
         std::cout << "ParentID: " << entryInfo.getParentEntryID() << std::endl;

      std::cout << "Metadata node: " << ownerNode->getTypedNodeID() << std::endl;

      // request and print entry info
      getInfoRes = getAndPrintEntryInfo(ownerNode, &entryInfo, &pathInfo);
      if(!getInfoRes)
         retVal = APPCODE_RUNTIME_ERROR;

      if (this->cfgVerbose && this->cfgUseMountedPath)
      {

         bool printInodeHashPath = false;

         std::string hashPath = MetaStorageTk::getMetaInodePath("", entryInfo.getEntryID());

         if (isFile)
         {
            std::string dentryPath = MetaStorageTk::getMetaDirEntryPath("",
               entryInfo.getParentEntryID() );

            std::string chunkPath =
               StorageTk::getFileChunkPath(&pathInfo, entryInfo.getEntryID() );

            std::cout << "Chunk path: " << chunkPath << std::endl;

            std::cout << "Dentry path: " << dentryPath.substr(1) + '/' << std::endl;

            if (!entryInfo.getIsInlined() )
               printInodeHashPath = true;
         }
         else
         {
            printInodeHashPath = true;
         }

         if (printInodeHashPath)
            std::cout << "Inode hash path: " << hashPath.substr(1) << std::endl;
      }

      // cleanup
      metaNodes->releaseNode(&ownerNode);

      if(!cfgReadFromStdin)
         break;


finish_this_entry:

      std::cout << std::endl;

      // read next relPath from stdin
      this->cfgPathStr.clear();
      getline(std::cin, this->cfgPathStr);

   } while(!this->cfgPathStr.empty() );


cleanup_mgmt:
   mgmtNodes->releaseNode(&mgmtNode);

   return retVal;
}

void ModeGetEntryInfo::printHelp()
{
   std::cout << "MODE ARGUMENTS:" << std::endl;
   std::cout << " Mandatory:" << std::endl;
   std::cout << "  <path>                 Path to a file or directory." << std::endl;
   std::cout << "                         Specify \"-\" here to read multiple paths from" << std::endl;
   std::cout << "                         stdin (separated by newline)." << std::endl;
   std::cout << " Optional:" << std::endl;
   std::cout << "  --unmounted            If this is specified then the given path is relative" << std::endl;
   std::cout << "                         to the root directory of a possibly unmounted FhGFS." << std::endl;
   std::cout << "                         (Symlinks will not be resolved in this case.)" << std::endl;
   std::cout << "  --verbose              Print more entry information, e.g. location on server." << std::endl;
   std::cout << "  --nomappings           Do not print to which storage servers the stripe" << std::endl;
   std::cout << "                         targets of a file are mapped." << std::endl;
   std::cout << std::endl;
   std::cout << "USAGE:" << std::endl;
   std::cout << " This mode retrieves information about a certain file or directory, such as" << std::endl;
   std::cout << " striping information or the entry ID." << std::endl;
   std::cout << std::endl;
   std::cout << " Example: Retrieve information about a certain file" << std::endl;
   std::cout << "  $ beegfs-ctl --getentryinfo /mnt/beegfs/myfile" << std::endl;
}

bool ModeGetEntryInfo::getAndPrintEntryInfo(Node* ownerNode, EntryInfo* entryInfo,
   PathInfo* outPathInfo)
{
   NodeStoreServers* metaNodes = Program::getApp()->getMetaNodes();

   bool retVal = false;

   bool commRes;
   char* respBuf = NULL;
   NetMessage* respMsg = NULL;
   GetEntryInfoRespMsg* respMsgCast;

   FhgfsOpsErr getInfoRes;
   StripePattern* pattern = NULL;
   uint16_t mirrorNodeID;

   GetEntryInfoMsg getInfoMsg(entryInfo);

   // request/response
   commRes = MessagingTk::requestResponse(
      ownerNode, &getInfoMsg, NETMSGTYPE_GetEntryInfoResp, &respBuf, &respMsg);
   if(!commRes)
   {
      std::cerr << "Communication with server failed: " << ownerNode->getNodeIDWithTypeStr() <<
         std::endl;
      goto err_cleanup;
   }

   respMsgCast = (GetEntryInfoRespMsg*)respMsg;

   getInfoRes = (FhgfsOpsErr)respMsgCast->getResult();
   if(getInfoRes != FhgfsOpsErr_SUCCESS)
   {
      std::cerr << "Server encountered an error: " << ownerNode->getNodeIDWithTypeStr() << "; " <<
         "Error: " << FhgfsOpsErrTk::toErrString(getInfoRes) << std::endl;
      goto err_cleanup;
   }

   *outPathInfo = *(respMsgCast->getPathInfo() );

   // print received information...

   // print metadata mirror node
   mirrorNodeID = respMsgCast->getMirrorNodeID();
   if(mirrorNodeID)
      std::cout << "Metadata mirror node: " << metaNodes->getTypedNodeID(mirrorNodeID) << std::endl;

   pattern = respMsgCast->createPattern();

   // print stripe pattern details
   printPattern(pattern);

   retVal = true;

err_cleanup:
   SAFE_DELETE(respMsg);
   SAFE_FREE(respBuf);

   SAFE_DELETE(pattern);

   return retVal;
}

void ModeGetEntryInfo::printPattern(StripePattern* pattern)
{
   unsigned patternType = pattern->getPatternType();

   switch(patternType)
   {
      case STRIPEPATTERN_Raid0:
      case STRIPEPATTERN_Raid10:
      case STRIPEPATTERN_BuddyMirror:
      {
         const UInt16Vector* stripeTargetIDs = pattern->getStripeTargetIDs();
         const UInt16Vector* mirrorTargetIDs =
            (patternType == STRIPEPATTERN_Raid10) ?
               ( (Raid10Pattern*)pattern)->getMirrorTargetIDs() : NULL;

         std::cout << "Stripe pattern details:" << std::endl;

         std::cout << "+ Type: " << pattern->getPatternTypeStr() << std::endl;

         std::cout << "+ Chunksize: " << UnitTk::int64ToHumanStr(pattern->getChunkSize() ) <<
            std::endl;

         std::cout << "+ Number of storage targets: ";

         // print number of "desired" and "actual" targets

         if(stripeTargetIDs->empty() ) // print "desired"-only for dirs
            std::cout << "desired: " << pattern->getDefaultNumTargets();
         else
         if( (patternType == STRIPEPATTERN_Raid10) && !mirrorTargetIDs->empty() )
            std::cout << "desired: " << pattern->getDefaultNumTargets() << "; " <<
               "actual: " << stripeTargetIDs->size() << "+" << mirrorTargetIDs->size();
         else
            std::cout << "desired: " << pattern->getDefaultNumTargets() << "; " <<
               "actual: " << stripeTargetIDs->size();

         std::cout << std::endl;

         /* note: we check size of stripeTargetIDs, because this might be a directory and thus
            it doesn't have any stripe targets */

         if(stripeTargetIDs->size() )
         { // list storage targets
            if (patternType == STRIPEPATTERN_BuddyMirror)
               std::cout << "+ Storage mirror buddy groups: " << std::endl;
            else
               std::cout << "+ Storage targets: " << std::endl;

            UInt16VectorConstIter iter = stripeTargetIDs->begin();
            for( ; iter != stripeTargetIDs->end(); iter++)
            {
               std::cout << "  + " << *iter;

               if (patternType != STRIPEPATTERN_BuddyMirror)
                  printTargetMapping(*iter);

               std::cout << std::endl;
            }
         }

         if(patternType == STRIPEPATTERN_Raid10)
         { // list mirror targets (if any)

            if(!mirrorTargetIDs->empty() )
            {
               std::cout << "+ Mirror targets: " << std::endl;

               UInt16VectorConstIter iter = mirrorTargetIDs->begin();
               for( ; iter != mirrorTargetIDs->end(); iter++)
               {
                  std::cout << "  + " << *iter;

                  printTargetMapping(*iter);

                  std::cout << std::endl;
               }
            }
         }
      } break;

      case STRIPEPATTERN_Invalid:
      {
         std::cerr << "Received an invalid stripe pattern." << std::endl;
      } break;

      default:
      {
         std::cerr << "Received an unknown stripe pattern type: " << patternType << std::endl;
      } break;
   } // end of switch
}

void ModeGetEntryInfo::printTargetMapping(uint16_t targetID)
{
   App* app = Program::getApp();
   NodeStoreServers* storageNodes = app->getStorageNodes();
   TargetMapper* targetMapper = app->getTargetMapper();


   if(!cfgShowTargetMappings)
      return;

   std::cout << " @ ";

   // show to which server this targetID is mapped
   uint16_t nodeNumID = targetMapper->getNodeID(targetID);
   if(!nodeNumID)
   {
      std::cout << "<unmapped>";
      return;
   }

   Node* node = storageNodes->referenceNode(nodeNumID);
   if(!node)
   {
      std::cout << "<unknown(" << nodeNumID << ")>";
      return;
   }

   std::cout << node->getTypedNodeID();

   storageNodes->releaseNode(&node);
}
