#include <app/App.h>
#include <common/nodes/NodeStore.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/NodesTk.h>
#include <program/Program.h>

#include "ModeStorageResyncStats.h"

#define MODESTORAGERESYNCSTATS_ARG_TARGETID      "--targetid"
#define MODESTORAGERESYNCSTATS_ARG_GROUPID       "--mirrorgroupid"

int ModeStorageResyncStats::execute()
{
   const int mgmtTimeoutMS = 2500;

   int retVal = APPCODE_RUNTIME_ERROR;

   App* app = Program::getApp();
   DatagramListener* dgramLis = app->getDatagramListener();
   NodeStore* mgmtNodes = app->getMgmtNodes();
   std::string mgmtHost = app->getConfig()->getSysMgmtdHost();
   unsigned short mgmtPortUDP = app->getConfig()->getConnMgmtdPortUDP();
   StringMap* cfg = app->getConfig()->getUnknownConfigArgs();
   uint16_t cfgTargetID = 0;
   uint16_t cfgBuddyID = 0;
   bool isBuddyGroupID = false;

   // check privileges
   if(!ModeHelper::checkRootPrivileges() )
      return APPCODE_RUNTIME_ERROR;

   // check arguments

   StringMapIter iter = cfg->find(MODESTORAGERESYNCSTATS_ARG_TARGETID);
   if(iter != cfg->end() )
   { // found a targetID
      bool isNumericRes = StringTk::isNumeric(iter->second);
      if(!isNumericRes)
      {
         std::cerr << "Invalid targetID given (must be numeric): " << iter->second << std::endl;
         return APPCODE_INVALID_CONFIG;
      }

      cfgTargetID = StringTk::strToUInt(iter->second);
      cfg->erase(iter);
   }

   iter = cfg->find(MODESTORAGERESYNCSTATS_ARG_GROUPID);
   if(iter != cfg->end() )
   { // found a buddyGroupID
      bool isNumericRes = StringTk::isNumeric(iter->second);
      if(!isNumericRes)
      {
         std::cerr << "Invalid mirror buddy group ID given (must be numeric): " << iter->second
            << std::endl;
         return APPCODE_INVALID_CONFIG;
      }

      cfgBuddyID = StringTk::strToUInt(iter->second);
      cfg->erase(iter);
   }

   if ( (cfgTargetID != 0) && (cfgBuddyID != 0) )
   {
      std::cerr << "--targetID and --mirrorgroupid cannot be used at the same time." << std::endl;
      return APPCODE_INVALID_CONFIG;
   }
   else
   if ( (cfgTargetID == 0 ) && (cfgBuddyID == 0))
   {
      std::cerr << "Either --targetID or --mirrorgroupid must be given." << std::endl;
      return APPCODE_INVALID_CONFIG;
   }
   else
   if (cfgBuddyID != 0)
   {
      cfgTargetID = cfgBuddyID;
      isBuddyGroupID = true;
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

   retVal = getStats(cfgTargetID, isBuddyGroupID);

   return retVal;
}

void ModeStorageResyncStats::printHelp()
{
   std::cout << "MODE ARGUMENTS:" << std::endl;
   std::cout << " Mandatory:" << std::endl;
   std::cout << "  --targetid=<targetID>          The ID of the secondary target that is being " << std::endl;
   std::cout << "                                 resynced." << std::endl;
   std::cout << "  --buddygroupid=<buddyGroupID>  The ID of the mirror buddy group that is being " << std::endl;
   std::cout << "                                 resynced." << std::endl;
   std::cout << std::endl;
   std::cout << "NOTE: only --targetid or --mirrorgroupid can be used." << std::endl;
   std::cout << std::endl;
   std::cout << "USAGE:" << std::endl;
   std::cout << " Retrieves statistics on a running resync" << std::endl;
   std::cout << std::endl;
   std::cout << " Example: Retrieve statistics of target with ID \"10\"" << std::endl;
   std::cout << "  $ beegfs-ctl --resyncstoragestats --targetid=10" << std::endl;
}

/**
 * @return APPCODE_...
 */
int ModeStorageResyncStats::getStats(uint16_t syncToTargetID, bool isBuddyGroupID)
{
   int retVal = APPCODE_NO_ERROR;

   App* app = Program::getApp();
   NodeStore* mgmtNodes = app->getMgmtNodes();
   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   TargetMapper* targetMapper = app->getTargetMapper();

   // download storage nodes and mappings
   NodeList storageNodeList;
   UInt16List targetIDs;
   UInt16List nodeIDs;
   UInt16List buddyGroupIDs;
   UInt16List buddyPrimaryTargetIDs;
   UInt16List buddySecondaryTargetIDs;
   uint16_t syncFromTargetID = 0;
   BuddyResyncJobStats jobStats;
   bool downloadNodesRes;
   bool downloadTargetsRes;
   bool downloadBuddyGroupsRes;
   FhgfsOpsErr getStatsRes;

   // download nodes
   downloadNodesRes = NodesTk::downloadNodes(mgmtNode, NODETYPE_Storage, &storageNodeList, false);

   if (!downloadNodesRes)
   {
      std::cerr << "Download of storage nodes failed. " << std::endl;
      retVal = APPCODE_RUNTIME_ERROR;

      goto mgmt_cleanup;
   }

   NodesTk::applyLocalNicCapsToList(app->getLocalNode(), &storageNodeList);
   NodesTk::moveNodesFromListToStore(&storageNodeList, app->getStorageNodes() );

   // download target mappings
   downloadTargetsRes = NodesTk::downloadTargetMappings(mgmtNode, &targetIDs, &nodeIDs, false);

   if (!downloadTargetsRes)
   {
      std::cerr << "Download of target mappings failed. " << std::endl;
      retVal = APPCODE_RUNTIME_ERROR;

      goto mgmt_cleanup;
   }

   targetMapper->syncTargetsFromLists(targetIDs, nodeIDs);

   // download buddy groups
   downloadBuddyGroupsRes = NodesTk::downloadMirrorBuddyGroups(mgmtNode, NODETYPE_Storage,
      &buddyGroupIDs, &buddyPrimaryTargetIDs, &buddySecondaryTargetIDs, false);

   if (!downloadBuddyGroupsRes)
   {
      std::cerr << "Download of mirror buddy group mappings failed. " << std::endl;
      retVal = APPCODE_RUNTIME_ERROR;

      goto mgmt_cleanup;
   }

   if(isBuddyGroupID)
   { // find the target IDs to the given buddy group
      UInt16ListIter buddyIter = buddyGroupIDs.begin();
      UInt16ListIter primaryIter = buddyPrimaryTargetIDs.begin();
      UInt16ListIter secondaryIter = buddySecondaryTargetIDs.begin();

      for(; buddyIter != buddyGroupIDs.end(); buddyIter++, primaryIter++, secondaryIter++)
      {
         if(*buddyIter == syncToTargetID)
         {
            syncFromTargetID = *primaryIter;
            syncToTargetID = *secondaryIter;
            break;
         }
      }
   }
   else
   { // find the primary target to the given sync target
      UInt16ListIter primaryIter = buddyPrimaryTargetIDs.begin();
      UInt16ListIter secondaryIter = buddySecondaryTargetIDs.begin();

      for(; primaryIter != buddyPrimaryTargetIDs.end(); primaryIter++, secondaryIter++)
      {
         if(*secondaryIter == syncToTargetID)
         {
            syncFromTargetID = *primaryIter;
            break;
         }
      }
   }

   if (syncFromTargetID == 0)
   {
      std::cerr << "Couldn't find buddy of target ID: " << syncToTargetID << "."
         " Is it the targetID of the secondary?" << std::endl;
      retVal = APPCODE_RUNTIME_ERROR;

      goto mgmt_cleanup;
   }

   // get the storage node corresponding to the ID
   getStatsRes = ModeHelper::getBuddyResyncStats(syncFromTargetID, jobStats);

   if(getStatsRes == FhgfsOpsErr_SUCCESS)
      printStats(jobStats);
   else
   {
      std::cerr << "Statistics could not be retrieved; primaryTargetID: " << syncFromTargetID
         << "; secondaryTargetID: " << syncToTargetID << "; Error: "
         << FhgfsOpsErrTk::toErrString(getStatsRes) << std::endl;

      retVal = APPCODE_RUNTIME_ERROR;
   }

   mgmt_cleanup:
   mgmtNodes->releaseNode(&mgmtNode);

   return retVal;
}

void ModeStorageResyncStats::printStats(BuddyResyncJobStats& jobStats)
{
   time_t startTimeSecs = (time_t)jobStats.getStartTime();
   time_t endTimeSecs = (time_t)jobStats.getEndTime();
   std::cout << "Job status: " << jobStatusToStr(jobStats.getStatus()) << std::endl;
   if (startTimeSecs)
      std::cout << "Job start time: " << ctime(&startTimeSecs);
   if (endTimeSecs)
      std::cout << "Job end time: " << ctime(&endTimeSecs);
   std::cout << "# of discovered dirs: " << jobStats.getDiscoveredDirs() << std::endl;
   std::cout << "# of discovered files: " << jobStats.getDiscoveredFiles() << std::endl;
   std::cout << "# of dir sync candidates: " << jobStats.getMatchedDirs() << std::endl;
   std::cout << "# of file sync candidates: " << jobStats.getMatchedFiles() << std::endl;
   std::cout << "# of synced dirs: " << jobStats.getSyncedDirs() << std::endl;
   std::cout << "# of synced files: " << jobStats.getSyncedFiles() << std::endl;
   std::cout << "# of dir sync errors: " << jobStats.getErrorDirs () << std::endl;
   std::cout << "# of file sync errors: " << jobStats.getErrorFiles() << std::endl;
}

std::string ModeStorageResyncStats::jobStatusToStr(BuddyResyncJobStatus status)
{
   switch (status)
   {
      case BuddyResyncJobStatus_NOTSTARTED: return "Not started"; break;
      case BuddyResyncJobStatus_RUNNING: return "Running"; break;
      case BuddyResyncJobStatus_SUCCESS: return "Completed successfully"; break;
      case BuddyResyncJobStatus_INTERRUPTED: return "Interrupted"; break;
      case BuddyResyncJobStatus_FAILURE: return "Failed"; break;
      case BuddyResyncJobStatus_ERRORS: return "Completed with errors"; break;
      default: return "Unknown"; break;
   }
}
