#include <app/App.h>
#include <common/net/message/nodes/SetTargetConsistencyStatesMsg.h>
#include <common/net/message/nodes/SetTargetConsistencyStatesRespMsg.h>
#include <common/net/message/storage/mirroring/SetLastBuddyCommOverrideMsg.h>
#include <common/net/message/storage/mirroring/SetLastBuddyCommOverrideRespMsg.h>
#include <common/nodes/NodeStore.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/NodesTk.h>
#include <common/toolkit/UnitTk.h>
#include <program/Program.h>

#include "ModeStartStorageResync.h"

#define MODESTARTSTORAGERESYNC_ARG_TARGETID      "--targetid"
#define MODESTARTSTORAGERESYNC_ARG_GROUPID       "--mirrorgroupid"
#define MODESTARTSTORAGERESYNC_ARG_TIMESPAN      "--timespan"
#define MODESTARTSTORAGERESYNC_ARG_TIMESTAMP     "--timestamp"
#define MODESTARTSTORAGERESYNC_ARG_RESTART       "--restart"

int ModeStartStorageResync::execute()
{
   const int mgmtTimeoutMS = 2500;

   App* app = Program::getApp();
   DatagramListener* dgramLis = app->getDatagramListener();
   NodeStore* mgmtNodes = app->getMgmtNodes();
   std::string mgmtHost = app->getConfig()->getSysMgmtdHost();
   unsigned short mgmtPortUDP = app->getConfig()->getConnMgmtdPortUDP();
   StringMap* cfg = app->getConfig()->getUnknownConfigArgs();
   uint16_t cfgTargetID = 0;
   uint16_t cfgBuddyID = 0;
   int64_t cfgTimestamp = -1;
   bool cfgRestart = false;
   bool isBuddyGroupID = false;

   // check privileges
   if(!ModeHelper::checkRootPrivileges() )
      return APPCODE_RUNTIME_ERROR;

   // check arguments

   StringMapIter iter = cfg->find(MODESTARTSTORAGERESYNC_ARG_TARGETID);
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

   iter = cfg->find(MODESTARTSTORAGERESYNC_ARG_GROUPID);
   if(iter != cfg->end() )
   { // found a buddyGroupID
      bool isNumericRes = StringTk::isNumeric(iter->second);
      if(!isNumericRes)
      {
         std::cerr << "Invalid buddy group ID given (must be numeric): " << iter->second
            << std::endl;
         return APPCODE_INVALID_CONFIG;
      }

      cfgBuddyID = StringTk::strToUInt(iter->second);
      cfg->erase(iter);
   }


   iter = cfg->find(MODESTARTSTORAGERESYNC_ARG_TIMESTAMP);
   if(iter != cfg->end() )
   { // found a timestamp
      bool isNumericRes = StringTk::isNumeric(iter->second);
      if(!isNumericRes)
      {
         std::cerr << "Invalid timestamp given (must be numeric): " << iter->second
            << std::endl;
         return APPCODE_INVALID_CONFIG;
      }

      cfgTimestamp = StringTk::strToInt64(iter->second);
      cfg->erase(iter);
   }

   iter = cfg->find(MODESTARTSTORAGERESYNC_ARG_TIMESPAN);
   if(iter != cfg->end() )
   { // found a timespan
      if (cfgTimestamp >= 0)
      {
         std::cerr << "--timestamp and --timespan cannot be used at the same time." << std::endl;
         return APPCODE_INVALID_CONFIG;
      }

      bool isValid = UnitTk::isValidHumanTimeString(iter->second);
      if(!isValid)
      {
         std::cerr << "Invalid timespan given (must be numeric with a unit of sec/min/h/d "
            "(seconds/minutes/hours/days): " << iter->second << std::endl;
         return APPCODE_INVALID_CONFIG;
      }

      // valid timespan => convert to secondes and substract from current timestamp
      int64_t timespanSecs = UnitTk::timeStrHumanToInt64(iter->second);
      int64_t currentTime = time(NULL);

      cfgTimestamp = currentTime-timespanSecs;
      cfg->erase(iter);
   }

   iter = cfg->find(MODESTARTSTORAGERESYNC_ARG_RESTART);
   if(iter != cfg->end() )
   { // restart flag set
      if (cfgTimestamp < 0)
      {
         std::cerr << "Resync can only be restarted in combination with --timestamp of --timespan."
            << std::endl;
         return APPCODE_INVALID_CONFIG;
      }

      cfgRestart = true;
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

   int resyncRes = requestResync(cfgTargetID, isBuddyGroupID, cfgTimestamp, cfgRestart);

   return resyncRes;
}

void ModeStartStorageResync::printHelp()
{
   std::cout << "MODE ARGUMENTS:" << std::endl;
   std::cout << " Mandatory:" << std::endl;
   std::cout << "  --targetid=<targetID>           The ID of the target that should be re-synced." << std::endl;
   std::cout << "  --mirrorgroupid=<mirrorGroupID> Resync the secondary target of this" << std::endl;
   std::cout << "                                  mirror buddy group." << std::endl;
   std::cout << std::endl;
   std::cout << " Note: Either --targetid or --mirrorgroupid can be used, not both." << std::endl;
   std::cout << std::endl;
   std::cout << " Optional:" << std::endl;
   std::cout << "  --timestamp=<unixTimestamp>     Override last buddy communication timestamp." << std::endl;
   std::cout << "  --timespan=<timespan>           Resync entries modified in the given timespan." << std::endl;
   std::cout << "  --restart                       Abort any running resyncs and start a new one." << std::endl;
   std::cout << std::endl;
   std::cout << " Note: --restart can only be used in combination with --timestamp or --timespan." << std::endl;
   std::cout << " Note: Either --timestamp or --timespan can be used, not both." << std::endl;
   std::cout << " Note: --timespan must have exactly one unit. Possible units are (s)econds, " << std::endl;
   std::cout << "       (m)inutes, (h)ours and (d)ays." << std::endl;
   std::cout << std::endl;
   std::cout << "USAGE:" << std::endl;
   std::cout << " This mode starts a resync of a storage target from its buddy target." << std::endl;
   std::cout << std::endl;
   std::cout << " Example: Resync target with ID \"10\" from its buddy using the resync timestamp " << std::endl;
   std::cout << "          saved on the server" << std::endl;
   std::cout << "  $ beegfs-ctl --resyncstorage --targetid=10" << std::endl;
   std::cout << std::endl;
   std::cout << " Example: Resync the changes in buddy group with ID 2 of the last 5 days" << std::endl;
   std::cout << "  $ beegfs-ctl --resyncstorage --mirrorgroupid=2 --timespan=5d" << std::endl;
   std::cout << std::endl;
   std::cout << " Example: Resync the changes in buddy group with ID 2 of the last 36 hours" << std::endl;
   std::cout << "  $ beegfs-ctl --resyncstorage --mirrorgroupid=2 --timespan=36h" << std::endl;
   std::cout << std::endl;
   std::cout << " Example: Perform a full resync of target with ID 5" << std::endl;
   std::cout << "  $ beegfs-ctl --resyncstorage --targetid=5 --timestamp=0" << std::endl;
}

/**
 * @return APPCODE_...
 */
int ModeStartStorageResync::requestResync(uint16_t syncToTargetID, bool isBuddyGroupID,
   int64_t timestamp, bool restartResync)
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
   uint16_t syncToNodeID;
   uint16_t syncFromTargetID = 0;
   uint16_t syncFromNodeID;
   UInt16ListIter buddyIter;
   UInt16ListIter primaryIter;
   UInt16ListIter secondaryIter;

   bool downloadNodesRes;
   bool downloadTargetsRes;
   bool downloadBuddyGroupsRes;
   FhgfsOpsErr setResyncStateRes;
   FhgfsOpsErr overrideLastBudyCommRes;

   BuddyResyncJobStats resyncJobStats;
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
      std::cerr << "Download of buddy group mappings failed. " << std::endl;
      retVal = APPCODE_RUNTIME_ERROR;

      goto mgmt_cleanup;
   }

   // find the target IDs to the given buddy group
   buddyIter = buddyGroupIDs.begin();
   primaryIter = buddyPrimaryTargetIDs.begin();
   secondaryIter = buddySecondaryTargetIDs.begin();

   if(isBuddyGroupID)
   {
      for(; buddyIter != buddyGroupIDs.end(); buddyIter++, primaryIter++, secondaryIter++)
      {
         if(*buddyIter == syncToTargetID)
         {
            syncFromTargetID = *primaryIter;
            syncToTargetID = *secondaryIter;
            break;
         }
      }

      if(syncToTargetID == 0)
      {
         std::cerr << "Couldn't find targets for buddy group ID: " << syncToTargetID << "."
            << std::endl;
         retVal = APPCODE_RUNTIME_ERROR;

         goto mgmt_cleanup;
      }
   }
   else
   {
      for(; secondaryIter != buddySecondaryTargetIDs.end(); primaryIter++, secondaryIter++)
      {
         if(*secondaryIter == syncToTargetID)
         {
            syncFromTargetID = *primaryIter;
            break;
         }
      }
   }

   if(syncFromTargetID == 0)
   {
      std::cerr << "Couldn't find primary target. secondary targetID: " << syncToTargetID << "."
         << std::endl;
      retVal = APPCODE_RUNTIME_ERROR;

      goto mgmt_cleanup;
   }

   // check if a resync is already running (but only if restartResync is not set)
   // if it is set we actually don't care if a resync is still running
   if (!restartResync)
   {
      getStatsRes = ModeHelper::getBuddyResyncStats(syncFromTargetID, resyncJobStats);

      if(getStatsRes != FhgfsOpsErr_SUCCESS)
      {
         std::cerr << "Could not check for running jobs. targetID: " << syncFromTargetID
            << "; error: " << FhgfsOpsErrTk::toErrString(getStatsRes) << std::endl;

         retVal = APPCODE_RUNTIME_ERROR;
         goto mgmt_cleanup;
      }
      else if(resyncJobStats.getStatus() == BuddyResyncJobStatus_RUNNING)
      {
         std::cerr << "Resync already running. targetID: " << syncFromTargetID
            << "; Please use option --restart to abort running resync and start over" << std::endl;
         retVal = APPCODE_NO_ERROR;
         goto mgmt_cleanup;
      }
   }

   // get the storage nodes corresponding to the IDs
   syncToNodeID = targetMapper->getNodeID(syncToTargetID);
   syncFromNodeID = targetMapper->getNodeID(syncFromTargetID);

   if(timestamp >= 0)
   {
      overrideLastBudyCommRes = overrideLastBuddyComm(syncFromTargetID, syncFromNodeID, timestamp,
         restartResync);
      if(overrideLastBudyCommRes != FhgfsOpsErr_SUCCESS)
      {
         std::cerr << "Could not override last buddy communication timestamp."
            "Resync request not successful; primaryTargetID: " << syncFromTargetID << "; nodeID: "
            << syncFromNodeID << "; Error: " << FhgfsOpsErrTk::toErrString(overrideLastBudyCommRes)
            << std::endl;

         retVal = APPCODE_RUNTIME_ERROR;
         goto mgmt_cleanup;
      }
   }

   if(restartResync)
   {
      std::cout << "Waiting for running resyncs to abort. " << std::endl;
      TargetConsistencyState secondaryState = TargetConsistencyState_BAD; // silence warning

      // to avoid races we have to wait for the node to leave needs_resync state; this usually
      // happens, because resync will set secondary to state BAD if it gets aborted.
      do
      {
         bool downloadStatesRes;
         UInt16List targetIDs;
         UInt8List reachabilityStates;
         UInt8List consistencyStates;

         downloadStatesRes = NodesTk::downloadTargetStates(mgmtNode, NODETYPE_Storage, &targetIDs,
            &reachabilityStates, &consistencyStates, false);

         if(!downloadStatesRes)
         {
            std::cerr << "Download of target states failed. " << std::endl;
            retVal = APPCODE_RUNTIME_ERROR;

            goto mgmt_cleanup;
         }

         UInt16ListIter targetIDsIter = targetIDs.begin();
         UInt8ListIter consistencyStatesIter = consistencyStates.begin();
         for(; targetIDsIter != targetIDs.end(); targetIDsIter++, consistencyStatesIter++)
         {
            if(*targetIDsIter == syncToTargetID)
            {
               secondaryState = (TargetConsistencyState)*consistencyStatesIter;
               break;
            }
            // if the end of the loop is reached, that means target could not be found
            std::cerr << "Download of target states failed; targetID unknown. " << std::endl;
            retVal = APPCODE_RUNTIME_ERROR;

            goto mgmt_cleanup;
         }

         PThread::sleepMS(2000);
      } while (secondaryState == TargetConsistencyState_NEEDS_RESYNC);
   }

   setResyncStateRes = setResyncState(syncToTargetID, syncToNodeID);

   if(setResyncStateRes != FhgfsOpsErr_SUCCESS)
   {
      std::cerr << "Resync request not successful; " <<
         (isBuddyGroupID ? "buddy group ID: " : "secondary target ID: ") << syncToTargetID
         << "; nodeID: " << syncToNodeID << "; Error: "
         << FhgfsOpsErrTk::toErrString(setResyncStateRes) << std::endl;

      retVal = APPCODE_RUNTIME_ERROR;
   }
   else
   {
      std::cout << "Resync request sent. " << std::endl;
      retVal = APPCODE_NO_ERROR;
   }

   mgmt_cleanup:
   mgmtNodes->releaseNode(&mgmtNode);

   return retVal;
}

FhgfsOpsErr ModeStartStorageResync::setResyncState(uint16_t targetID, uint16_t nodeID)
{
   FhgfsOpsErr retVal = FhgfsOpsErr_SUCCESS;

   App* app = Program::getApp();
   NodeStore* storageNodes = app->getStorageNodes();
   Node* storageNode = storageNodes->referenceNode(nodeID);

   if (!storageNode)
   {
      return FhgfsOpsErr_UNKNOWNNODE;
   }

   UInt16List targetIDs;
   UInt8List states;

   targetIDs.push_back(targetID);
   states.push_back((uint8_t)TargetConsistencyState_NEEDS_RESYNC);

   bool commRes;
   char* respBuf = NULL;
   NetMessage* respMsg = NULL;
   SetTargetConsistencyStatesRespMsg* respMsgCast;

   SetTargetConsistencyStatesMsg msg(&targetIDs, &states, false);

   // request/response
   commRes = MessagingTk::requestResponse(
      storageNode, &msg, NETMSGTYPE_SetTargetConsistencyStatesResp, &respBuf, &respMsg);
   if(!commRes)
   {
      retVal = FhgfsOpsErr_COMMUNICATION;
      goto err_cleanup;
   }

   respMsgCast = (SetTargetConsistencyStatesRespMsg*)respMsg;

   retVal = respMsgCast->getResult();

err_cleanup:
   storageNodes->releaseNode(&storageNode);
   SAFE_DELETE(respMsg);
   SAFE_FREE(respBuf);

   return retVal;
}

FhgfsOpsErr ModeStartStorageResync::overrideLastBuddyComm(uint16_t targetID, uint16_t nodeID,
   int64_t timestamp, bool restartResync)
{
   FhgfsOpsErr retVal = FhgfsOpsErr_SUCCESS;

   App* app = Program::getApp();
   NodeStore* storageNodes = app->getStorageNodes();
   Node* storageNode = storageNodes->referenceNode(nodeID);

   if (!storageNode)
   {
      return FhgfsOpsErr_UNKNOWNNODE;
   }

   bool commRes;
   char* respBuf = NULL;
   NetMessage* respMsg = NULL;
   SetLastBuddyCommOverrideRespMsg* respMsgCast;

   SetLastBuddyCommOverrideMsg msg(targetID, timestamp, restartResync);

   // request/response
   commRes = MessagingTk::requestResponse(
      storageNode, &msg, NETMSGTYPE_SetLastBuddyCommOverrideResp, &respBuf, &respMsg);
   if(!commRes)
   {
      retVal = FhgfsOpsErr_COMMUNICATION;
      goto err_cleanup;
   }

   respMsgCast = (SetLastBuddyCommOverrideRespMsg*)respMsg;

   retVal = (FhgfsOpsErr)respMsgCast->getValue();

err_cleanup:
   storageNodes->releaseNode(&storageNode);
   SAFE_DELETE(respMsg);
   SAFE_FREE(respBuf);

   return retVal;
}
