#include <app/App.h>
#include <common/net/message/nodes/GetNodeCapacityPoolsMsg.h>
#include <common/net/message/nodes/GetNodeCapacityPoolsRespMsg.h>
#include <common/net/message/storage/mirroring/GetStorageResyncStatsMsg.h>
#include <common/net/message/storage/mirroring/GetStorageResyncStatsRespMsg.h>
#include <common/nodes/TargetStateStore.h>
#include <common/toolkit/MetadataTk.h>
#include <common/toolkit/NodesTk.h>
#include <program/Program.h>
#include "ModeListTargets.h"

#include <algorithm>


#define MODELISTTARGETS_ARG_PRINTNODESFIRST     "--nodesfirst"
#define MODELISTTARGETS_ARG_PRINTLONGNODES      "--longnodes"
#define MODELISTTARGETS_ARG_HIDENODEID          "--hidenodeid"
#define MODELISTTARGETS_ARG_PRINTSTATE          "--state"
#define MODELISTTARGETS_ARG_PRINTCAPACITYPOOLS  "--pools"
#define MODELISTTARGETS_ARG_NODETYPE            "--nodetype"
#define MODELISTTARGETS_ARG_NODETYPE_META       "meta"
#define MODELISTTARGETS_ARG_NODETYPE_STORAGE    "storage"
#define MODELISTTARGETS_ARG_FROMMETA            "--frommeta" /* implies provided nodeID */
#define MODELISTTARGETS_ARG_PRINTSPACEINFO      "--spaceinfo"
#define MODELISTTARGETS_ARG_MIRRORGROUPS        "--mirrorgroups"

#define MODELISTTARGETS_MAX_LINE_LENGTH         256

#define MODELISTTARGETS_POOL_LOW                "low"
#define MODELISTTARGETS_POOL_NORMAL             "normal"
#define MODELISTTARGETS_POOL_EMERGENCY          "emergency"
#define MODELISTTARGETS_POOL_UNKNOWN            "unknown"

#define MODELISTTARGETS_BUDDYGROUP_MEMBERTYPE_PRIMARY       "primary"
#define MODELISTTARGETS_BUDDYGROUP_MEMBERTYPE_SECONDARY     "secondary"

#define MODELISTTARGETS_TARGET_CONSISTENCY_STATE_RESYNCING  "resyncing"



int ModeListTargets::execute()
{
   const int mgmtTimeoutMS = 2500;

   int retVal = APPCODE_RUNTIME_ERROR;

   App* app = Program::getApp();
   DatagramListener* dgramLis = app->getDatagramListener();
   NodeStoreServers* mgmtNodes = app->getMgmtNodes();
   std::string mgmtHost = app->getConfig()->getSysMgmtdHost();
   unsigned short mgmtPortUDP = app->getConfig()->getConnMgmtdPortUDP();

   // check arguments
   retVal = checkConfig();
   if(retVal)
      return retVal;

   // check mgmt node

   if(!NodesTk::waitForMgmtHeartbeat(
      NULL, dgramLis, mgmtNodes, mgmtHost, mgmtPortUDP, mgmtTimeoutMS) )
   {
      std::cerr << "Management node communication failed: " << mgmtHost << std::endl;
      return APPCODE_RUNTIME_ERROR;
   }

   Node* mgmtNode = mgmtNodes->referenceFirstNode();

   if(!mgmtNode)
   {
      std::cerr << "Failed to reference management node." << std::endl;
      return APPCODE_RUNTIME_ERROR;
   }

   if(prepareData(mgmtNode) )
   {
      retVal = APPCODE_RUNTIME_ERROR;
      goto cleanup_mgmt;
   }

   retVal = print();

cleanup_mgmt:
   mgmtNodes->releaseNode(&mgmtNode);

   return retVal;
}

/**
 * checks all arguments of the mode and creates the configuration for the mode
 *
 * @return 0 on success, all other values reports an error (APPCODE_...)
 */
int ModeListTargets::checkConfig()
{
   App* app = Program::getApp();
   StringMap* cfg = app->getConfig()->getUnknownConfigArgs();

   StringMapIter iter = cfg->find(MODELISTTARGETS_ARG_PRINTNODESFIRST);
   if(iter != cfg->end() )
   {
      cfgPrintNodesFirst = true;
      cfg->erase(iter);
   }

   iter = cfg->find(MODELISTTARGETS_ARG_PRINTLONGNODES);
   if(iter != cfg->end() )
   {
      cfgPrintLongNodes = true;
      cfgPrintNodesFirst = false; // not allowed in combination with long node output format
      cfg->erase(iter);
   }

   iter = cfg->find(MODELISTTARGETS_ARG_HIDENODEID);
   if(iter != cfg->end() )
   {
      cfgHideNodeID = true; // if --hidenodeid is set, the other both nodeID options are ignored
      cfgPrintLongNodes = false;
      cfgPrintNodesFirst = false;
      cfg->erase(iter);
   }

   iter = cfg->find(MODELISTTARGETS_ARG_PRINTSTATE);
   if(iter != cfg->end() )
   {
      cfgPrintState = true;
      cfg->erase(iter);
   }

   iter = cfg->find(MODELISTTARGETS_ARG_PRINTSPACEINFO);
   if(iter != cfg->end() )
   {
      cfgPrintSpaceInfo = true;
      cfg->erase(iter);
   }

   iter = cfg->find(MODELISTTARGETS_ARG_PRINTCAPACITYPOOLS);
   if(iter != cfg->end() )
   {
      cfgPrintCapacityPools = true;
      cfg->erase(iter);
   }

   iter = cfg->find(MODELISTTARGETS_ARG_NODETYPE);
   if(iter == cfg->end() )
   { // default is storage
      cfgPoolQueryType = CapacityPoolQuery_STORAGE;
      cfgNodeType = NODETYPE_Storage;
   }
   else
   { // check given pool query type
      if(iter->second == std::string(MODELISTTARGETS_ARG_NODETYPE_META) )
      {
         cfgPoolQueryType = CapacityPoolQuery_META;
         cfgNodeType = NODETYPE_Meta;
      }
      else
      if(iter->second == std::string(MODELISTTARGETS_ARG_NODETYPE_STORAGE) )
      {
         cfgPoolQueryType = CapacityPoolQuery_STORAGE;
         cfgNodeType = NODETYPE_Storage;
      }
      else
      {
         std::cerr << "Invalid node type." << std::endl;
         return APPCODE_INVALID_CONFIG;
      }

      cfg->erase(iter);
   }

   iter = cfg->find(MODELISTTARGETS_ARG_MIRRORGROUPS);
   if(iter != cfg->end() )
   {
      cfgUseBuddyGroups = true;

      cfg->erase(iter);
   }

   iter = cfg->find(MODELISTTARGETS_ARG_FROMMETA);
   if(iter != cfg->end() )
   {
      cfgGetFromMeta = true;

      cfg->erase(iter);

      // getting from meta server requires specified meta nodeID

      if(cfg->empty() )
      {
         std::cerr << "NodeID missing." << std::endl;
         return APPCODE_INVALID_CONFIG;
      }

      poolSourceNodeID = StringTk::strToUInt(cfg->begin()->first);
      cfg->erase(cfg->begin() );
   }

   if(ModeHelper::checkInvalidArgs(cfg) )
      return APPCODE_INVALID_CONFIG;

   return APPCODE_NO_ERROR;
}

/**
 * Downloads the required data from the management daemon
 *
 * @param mgmtNode the management node, must be referenced before given to this method
 * @return 0 on success, all other values reports an error (APPCODE_...)
 */
int ModeListTargets::prepareData(Node* mgmtNode)
{
   App* app = Program::getApp();
   TargetMapper* targetMapper = app->getTargetMapper();

   UInt16List mappedTargetIDs;
   UInt16List mappedNodeIDs;


   // download targets

   if(!NodesTk::downloadTargetMappings(mgmtNode, &mappedTargetIDs, &mappedNodeIDs, false) )
   {
      std::cerr << "Target mappings download failed." << std::endl;
      return APPCODE_RUNTIME_ERROR;
   }
   targetMapper->syncTargetsFromLists(mappedTargetIDs, mappedNodeIDs);


   // download nodes

   if(cfgNodeType == NODETYPE_Meta)
   {
      NodeList metaNodes;

      if(!NodesTk::downloadNodes(mgmtNode, NODETYPE_Meta, &metaNodes, false, NULL) )
      {
         std::cerr << "Node download failed." << std::endl;
         return APPCODE_RUNTIME_ERROR;
      }

      NodesTk::applyLocalNicCapsToList(app->getLocalNode(), &metaNodes);
      NodesTk::moveNodesFromListToStore(&metaNodes, app->getMetaNodes() );
   }
   else
   {
      NodeList storageNodes;

      if(!NodesTk::downloadNodes(mgmtNode, NODETYPE_Storage, &storageNodes, false, NULL) )
      {
         std::cerr << "Node download failed." << std::endl;
         return APPCODE_RUNTIME_ERROR;
      }

      NodesTk::applyLocalNicCapsToList(app->getLocalNode(), &storageNodes);
      NodesTk::moveNodesFromListToStore(&storageNodes, app->getStorageNodes() );
   }


   // download states

   if(cfgPrintState)
   {
      UInt16List targetIDs;
      UInt8List targetReachabilityStates;
      UInt8List targetConsistencyStates;
      TargetStateStore* targetStateStore = app->getTargetStateStore();

      if(!NodesTk::downloadTargetStates(mgmtNode, cfgNodeType, &targetIDs,
         &targetReachabilityStates, &targetConsistencyStates, false) )
      {
         std::cerr << "States download failed." << std::endl;
         return APPCODE_RUNTIME_ERROR;
      }

      targetStateStore->syncStatesFromLists(targetIDs, targetReachabilityStates,
         targetConsistencyStates);
   }


   // download mirror buddy groups

   if(!NodesTk::downloadMirrorBuddyGroups(mgmtNode, NODETYPE_Storage, &buddyGroupIDs,
      &buddyGroupPrimaryTargetIDs, &buddyGroupSecondaryTargetIDs, false) )
   {
      std::cerr << "Download of mirror buddy groups failed." << std::endl;
      return APPCODE_RUNTIME_ERROR;
   }


   // set communication source store for pool data

   if(cfgPrintCapacityPools)
   {
      if(cfgGetFromMeta)
         poolSourceNodeStore = app->getMetaNodes();
      else
      {
         poolSourceNodeStore = app->getMgmtNodes();
         poolSourceNodeID = mgmtNode->getNumID();
      }

      bool getPoolsCommRes = getPoolsComm(poolSourceNodeStore, poolSourceNodeID, cfgPoolQueryType);
      if(!getPoolsCommRes)
      {
         std::cerr << "Download of pools failed." << std::endl;
         return APPCODE_RUNTIME_ERROR;
      }
   }

   return APPCODE_NO_ERROR;
}

void ModeListTargets::printHelp()
{
   std::cout << "MODE ARGUMENTS:" << std::endl;
   std::cout << " Optional:" << std::endl;
   std::cout << "  --nodesfirst            Print nodes in first column." << std::endl;
   std::cout << "  --longnodes             Print node IDs in long format." << std::endl;
   std::cout << "  --state                 Print state of each target." << std::endl;
   std::cout << "  --pools                 Print the capacity pool which the target belongs to." << std::endl;
   std::cout << "  --nodetype=<nodetype>   The node type to list (meta, storage)." << std::endl;
   std::cout << "                          (Default: storage)" << std::endl;
   std::cout << "  --spaceinfo             Print total/free disk space and available inodes." << std::endl;
   std::cout << "  --mirrorgroups          Print mirror buddy groups instead of targets." << std::endl;
   std::cout << std::endl;
   std::cout << "USAGE:" << std::endl;
   std::cout << " This mode lists the available storage/meta targets and the nodes to which they " << std::endl;
   std::cout << " are mapped. The mode can also list all servers/targets and their assignment to " << std::endl;
   std::cout << " one of the three capacity pools (\"normal\", \"low\", \"emergency\"), based on " << std::endl;
   std::cout << " the available disk space." << std::endl;
   std::cout << " Pool assignments influence the storage allocation algorithm of metadata" << std::endl;
   std::cout << " servers. Pool limits can be defined in the management server config file." << std::endl;
   std::cout << std::endl;
   std::cout << " Example: List registered storage targets" << std::endl;
   std::cout << "  $ beegfs-ctl --listtargets" << std::endl;
}

/**
 * Downloads the 3 lists of the pools with the targetIDs/nodesIDs which belongs to the pool
 *
 * @param nodes The nodeStore which contains to destination node
 * @param nodeID The nodeID of the destination node (meta or mgmtd) to download the polls
 * @param poolQueryType The query type (CapacityPoolQueryType) for the pool download
 * @return true on success, false on error
 */
bool ModeListTargets::getPoolsComm(NodeStoreServers* nodes, uint16_t nodeID,
   CapacityPoolQueryType poolQueryType)
{
   Node* node = nodes->referenceNode(nodeID);
   if(!node)
   {
      std::cerr << "Unknown node: " << nodeID << std::endl;
      return false;
   }

   bool retVal = false;

   bool commRes;
   char* respBuf = NULL;
   NetMessage* respMsg = NULL;
   GetNodeCapacityPoolsRespMsg* respMsgCast;

   GetNodeCapacityPoolsMsg msg(poolQueryType);

  // request/response
   commRes = MessagingTk::requestResponse(
      node, &msg, NETMSGTYPE_GetNodeCapacityPoolsResp, &respBuf, &respMsg);
   if(!commRes)
   {
      std::cerr << "Pools download failed: " << nodeID << std::endl;
      goto err_cleanup;
   }

   respMsgCast = (GetNodeCapacityPoolsRespMsg*)respMsg;

   respMsgCast->parseLists(&poolNormalList, &poolLowList, &poolEmergencyList);

   retVal = true;

err_cleanup:
   nodes->releaseNode(&node);
   SAFE_DELETE(respMsg);
   SAFE_FREE(respBuf);

   return retVal;
}

/**
 * prints the the hole table with the targets, with respect to the configuration
 *
 * @return 0 on success, all other values reports an error (APPCODE_...)
 */
int ModeListTargets::print()
{
   int retVal = APPCODE_NO_ERROR;

   App* app = Program::getApp();

   printHeader();

   if(cfgUseBuddyGroups && (cfgNodeType == NODETYPE_Storage) )
   {
      FhgfsOpsErr outError = FhgfsOpsErr_SUCCESS;
      NodeStoreServers* storageServers = app->getStorageNodes();
      TargetMapper* targets = app->getTargetMapper();

      UInt16ListIter buddyGroupIDIter = buddyGroupIDs.begin();
      UInt16ListIter primaryTargetIDIter = buddyGroupPrimaryTargetIDs.begin();
      UInt16ListIter secondaryTargetIDIter = buddyGroupSecondaryTargetIDs.begin();
      for( ; (buddyGroupIDIter != buddyGroupIDs.end() ) &&
             (primaryTargetIDIter != buddyGroupPrimaryTargetIDs.end() ) &&
             (secondaryTargetIDIter != buddyGroupSecondaryTargetIDs.end() );
         buddyGroupIDIter++, primaryTargetIDIter++, secondaryTargetIDIter++)
      {
         // print primary target of buddy group
         Node* node = storageServers->referenceNodeByTargetID(*primaryTargetIDIter, targets,
            &outError);
         if(!node || outError)
         {
            fprintf(stderr, " [ERROR: Could not find storage servers for targetID: %u]\n",
               *primaryTargetIDIter);
            retVal = APPCODE_RUNTIME_ERROR;
         }
         else
         {
            retVal = printTarget(*primaryTargetIDIter, true, node, *buddyGroupIDIter,
               *secondaryTargetIDIter);
            storageServers->releaseNode(&node);
         }

         // print secondary target of buddy group
         node = storageServers->referenceNodeByTargetID(*secondaryTargetIDIter, targets,
            &outError);
         if(!node || outError)
         {
            fprintf(stderr, " [ERROR: Could not find storage servers for targetID: %u]\n",
               *secondaryTargetIDIter);
            retVal = APPCODE_RUNTIME_ERROR;
         }
         else
         {
            retVal = printTarget(*secondaryTargetIDIter, false, node, *buddyGroupIDIter,
               *primaryTargetIDIter);
            storageServers->releaseNode(&node);
         }
      }
   }
   else
   if(cfgUseBuddyGroups && (cfgNodeType == NODETYPE_Meta) )
   {
      fprintf(stderr, " [ERROR: %s]",
         "Mirror buddy grousp for metadata servers are not implemented in the current version.\n");
   }
   else
   if(cfgNodeType == NODETYPE_Storage)
   {
      NodeStoreServers* storageServers = app->getStorageNodes();
      TargetMapper* targets = app->getTargetMapper();

      UInt16List targetIDsList;
      UInt16List nodeIDsList;
      targets->getMappingAsLists(targetIDsList, nodeIDsList);

      UInt16ListIter targetIDIter = targetIDsList.begin();
      UInt16ListIter nodeIDIter = nodeIDsList.begin();
      for( ; (targetIDIter != targetIDsList.end() ) && (nodeIDIter != nodeIDsList.end() );
         targetIDIter++, nodeIDIter++)
      {
         Node* node = storageServers->referenceNode(*nodeIDIter);
         if(!node)
         {
            fprintf(stderr, " [ERROR: Unknown storage server ID: %u]\n", *nodeIDIter);
            retVal = APPCODE_RUNTIME_ERROR;
         }
         else
         {
            retVal = printTarget(*targetIDIter, false, node, 0, 0);
            storageServers->releaseNode(&node);
         }
      }
   }
   else
   if(cfgNodeType == NODETYPE_Meta)
   {
      NodeStoreServers* metaServers = app->getMetaNodes();
      Node* node = metaServers->referenceFirstNode();
      if(!node)
      {
         fprintf(stderr, " [ERROR: No metadata servers found]\n");
         retVal = APPCODE_RUNTIME_ERROR;
      }

      while(node)
      {
         retVal = printTarget(node->getNumID(), false, node, 0, 0);
         node = metaServers->referenceNextNodeAndReleaseOld(node);
      }
   }

   return retVal;
}

/**
 * prints the header of the table to the console
 *
 * @return 0 on success, all other values reports an error (APPCODE_...)
 */
int ModeListTargets::printHeader()
{
   char* description = (char*) malloc(MODELISTTARGETS_MAX_LINE_LENGTH);
   char* spacer = (char*) malloc(MODELISTTARGETS_MAX_LINE_LENGTH);

   int sizeDescription = 0;
   int sizeSpacer = 0;

   if(cfgPrintNodesFirst && !cfgPrintLongNodes && !cfgHideNodeID)
   {
      sizeDescription += sprintf(&description[sizeDescription], "%8s ",
         "NodeID");
      sizeSpacer += sprintf(&spacer[sizeSpacer], "%8s ",
         "======");
   }

   if(cfgUseBuddyGroups)
   {
      sizeDescription += sprintf(&description[sizeDescription], "%12s %12s ",
         "MirrorGroupID", "MGMemberType");
      sizeSpacer += sprintf(&spacer[sizeSpacer], "%12s %12s ",
         "=============", "============");
   }

   sizeDescription += sprintf(&description[sizeDescription], "%8s ",
      "TargetID");
   sizeSpacer += sprintf(&spacer[sizeSpacer], "%8s ",
      "========");

   if(cfgPrintState)
   {
      sizeDescription += sprintf(&description[sizeDescription], "%16s %12s ",
         "Reachability", "Consistency");
      sizeSpacer += sprintf(&spacer[sizeSpacer], "%16s %12s ",
         "============", "===========");
   }

   if(cfgPrintCapacityPools)
   {
      sizeDescription += sprintf(&description[sizeDescription], "%11s ",
         "Pool");
      sizeSpacer += sprintf(&spacer[sizeSpacer], "%11s ",
         "====");
   }

   if(cfgPrintSpaceInfo)
   {
      sizeDescription += sprintf(&description[sizeDescription], "%11s %11s %4s %11s %11s %4s ",
         "Total", "Free", "%", "ITotal", "IFree", "%");
      sizeSpacer += sprintf(&spacer[sizeSpacer], "%11s %11s %4s %11s %11s %4s ",
         "=====", "====", "=", "======", "=====", "=");
   }

   if( (!cfgPrintNodesFirst || cfgPrintLongNodes) && !cfgHideNodeID)
   {
      sizeDescription += sprintf(&description[sizeDescription], "%8s ",
         "NodeID");
      sizeSpacer += sprintf(&spacer[sizeSpacer], "%8s ",
         "======");
   }

   printf("%s\n", description);
   printf("%s\n", spacer);

   SAFE_FREE(description);
   SAFE_FREE(spacer);

   return APPCODE_NO_ERROR;
}

/**
 * prints one line of the table; the information of one target
 *
 * @param targetID The targetNumID of the target to print
 * @param isPrimaryTarget true if this target is the primary target of the buddy group
 * @param node the node where the target is located, must be referenced before given to this method
 * @param buddyGroupID the buddy group ID where the target belongs to
 * @param buddyTargetID the targetNumID of the buddy target
 * @return 0 on success, all other values reports an error (APPCODE_...)
 */
int ModeListTargets::printTarget(uint16_t targetID, bool isPrimaryTarget, Node* node,
   uint16_t buddyGroupID, uint16_t buddyTargetID)
{
   int retVal = APPCODE_NO_ERROR;

   char* lineString = (char*) malloc(MODELISTTARGETS_MAX_LINE_LENGTH);
   int sizeLine = 0;

   // print long node IDs forbids --nodesfirst
   if(cfgPrintNodesFirst && !cfgPrintLongNodes && !cfgHideNodeID)
      addNodeIDToLine(lineString, &sizeLine, node);

   if(cfgUseBuddyGroups)
      addBuddyGroupToLine(lineString, &sizeLine, buddyGroupID, isPrimaryTarget);

   addTargetIDToLine(lineString, &sizeLine, targetID);

   if(cfgPrintState)
      addStateToLine(lineString, &sizeLine, targetID, buddyTargetID);

   if(cfgPrintCapacityPools)
      retVal = (retVal == APPCODE_NO_ERROR) ?
         addPoolToLine(lineString, &sizeLine, targetID) : retVal;

   if(cfgPrintSpaceInfo)
      retVal = (retVal == APPCODE_NO_ERROR) ?
         addSpaceToLine(lineString, &sizeLine, targetID, node) : retVal;

   if( (!cfgPrintNodesFirst || cfgPrintLongNodes) && !cfgHideNodeID)
      addNodeIDToLine(lineString, &sizeLine, node);

   printf("%s\n", lineString);

   SAFE_FREE(lineString);

   return retVal;
}

/**
 * add the targetNumID to the output string of a table line
 *
 * @param inOutString the output string of a target for the output table
 * @param inOutOffset the offset in the inOutString to start with the targetNumID
 * @param currentTargetID the targetNumID to print
 * @return 0 on success, all other values reports an error (APPCODE_...)
 */
int ModeListTargets::addTargetIDToLine(char* inOutString, int* inOutOffset,
   uint16_t currentTargetID)
{
   *inOutOffset += sprintf(&inOutString[*inOutOffset],"%8hu ", currentTargetID);

   return APPCODE_NO_ERROR;
}

/**
 * add the nodeNumID to the output string of a table line
 *
 * @param inOutString the output string of a target for the output table
 * @param inOutOffset the offset in the inOutString to start with the nodeNumID
 * @param node the node where the target is located, must be referenced before given to this method
 * @return 0 on success, all other values reports an error (APPCODE_...)
 */
int ModeListTargets::addNodeIDToLine(char* inOutString, int* inOutOffset, Node* node)
{
   if(cfgPrintLongNodes) // note: two extra spaces to compensate for %8s
      *inOutOffset += sprintf(&inOutString[*inOutOffset], "  %s ",
         node->getNodeIDWithTypeStr().c_str() );
   else
      *inOutOffset += sprintf(&inOutString[*inOutOffset], "%8hu ", node->getNumID() );

   return APPCODE_NO_ERROR;
}

/**
 * add the state of a target to the output string of a table line
 *
 * @param inOutString the output string of a target for the output table
 * @param inOutOffset the offset in the inOutString to start with the state of the target
 * @param currentTargetID the targetNumID of the target where the state is needed
 * @param buddyTargetID the targetNumID of the buddy target
 * @return 0 on success, all other values reports an error (APPCODE_...)
 */
int ModeListTargets::addStateToLine(char* inOutString, int* inOutOffset, uint16_t currentTargetID,
   uint16_t buddyTargetID)
{
   CombinedTargetState state;

   if (!Program::getApp()->getTargetStateStore()->getState(currentTargetID, state) )
   {
      *inOutOffset += sprintf(&inOutString[*inOutOffset], "%29s ", "<invalid_value>");
   }
   else
   {
      std::string consistencyStateString = TargetStateStore::stateToStr(state.consistencyState);

      if(state.consistencyState == TargetConsistencyState_NEEDS_RESYNC)
      {
         // find the buddy target ID, this is required when the option --mirrorgroups is not used
         if(!buddyTargetID)
         {
            UInt16ListIter buddyGroupIDIter = buddyGroupIDs.begin();
            UInt16ListIter primaryTargetIDIter = buddyGroupPrimaryTargetIDs.begin();
            UInt16ListIter secondaryTargetIDIter = buddyGroupSecondaryTargetIDs.begin();
            for( ; (buddyGroupIDIter != buddyGroupIDs.end() ) &&
                   (primaryTargetIDIter != buddyGroupPrimaryTargetIDs.end() ) &&
                   (secondaryTargetIDIter != buddyGroupSecondaryTargetIDs.end() );
               buddyGroupIDIter++, primaryTargetIDIter++, secondaryTargetIDIter++)
            {
               if(*primaryTargetIDIter == currentTargetID)
               {
                  buddyTargetID = *secondaryTargetIDIter;
                  break;
               }
               else
               if(*secondaryTargetIDIter == currentTargetID)
               {
                  buddyTargetID = *primaryTargetIDIter;
                  break;
               }
            }
         }

         BuddyResyncJobStats jobStats;
         ModeHelper::getBuddyResyncStats(buddyTargetID, jobStats);

         if(jobStats.getStatus() == BuddyResyncJobStatus_RUNNING)
            consistencyStateString = MODELISTTARGETS_TARGET_CONSISTENCY_STATE_RESYNCING;
      }

      *inOutOffset += sprintf(&inOutString[*inOutOffset], "%16s %12s ",
         TargetStateStore::stateToStr(state.reachabilityState), consistencyStateString.c_str() );
   }

   return APPCODE_NO_ERROR;
}

/**
 * add the pool where the target belongs to to the output string of a table line
 *
 * @param inOutString the output string of a target for the output table
 * @param inOutOffset the offset in the inOutString to start with the pool of the target
 * @param currentTargetID the targetNumID of the target where the pool is needed
 * @return 0 on success, all other values reports an error (APPCODE_...)
 */
int ModeListTargets::addPoolToLine(char* inOutString, int* inOutOffset, uint16_t currentTargetID)
{
   int retVal = APPCODE_NO_ERROR;

   std::string poolName;

   if(std::find(poolNormalList.begin(), poolNormalList.end(), currentTargetID)
         != poolNormalList.end() )
      poolName = MODELISTTARGETS_POOL_NORMAL;
   else
   if(std::find(poolLowList.begin(), poolLowList.end(), currentTargetID) != poolLowList.end() )
      poolName = MODELISTTARGETS_POOL_LOW;
   else
   if(std::find(poolEmergencyList.begin(), poolEmergencyList.end(), currentTargetID)
         != poolEmergencyList.end() )
      poolName = MODELISTTARGETS_POOL_EMERGENCY;
   else
   {
      poolName = MODELISTTARGETS_POOL_UNKNOWN;
      retVal = APPCODE_RUNTIME_ERROR;
   }

   *inOutOffset += sprintf(&inOutString[*inOutOffset], "%11s ", poolName.c_str() );

   return retVal;
}

/**
 * add the space/inode information of the target to the output string of a table line
 *
 * @param inOutString the output string of a target for the output table
 * @param inOutOffset the offset in the inOutString to start with the space/inode information
 * @param currentTargetID the targetNumID of the target where the space/inode information is needed
 * @param node the node where the target is located, must be referenced before given to this method
 * @return 0 on success, all other values reports an error (APPCODE_...)
 */
int ModeListTargets::addSpaceToLine(char* inOutString, int* inOutOffset, uint16_t currentTargetID,
   Node* node)
{
   // print total/free space information

   // get and print space info

   FhgfsOpsErr statRes;
   int64_t freeSpace = 0;
   int64_t totalSpace = 0;
   double freeSpaceGB;
   double totalSpaceGB;
   int64_t freeInodes = 0;
   int64_t totalInodes = 0;
   double freeInodesM; // m=mega (as in mega byte)
   double totalInodesM; // m=mega (as in mega byte)
   double freeSpacePercent;
   double freeInodesPercent;
   double gb = 1024*1024*1024;
   double mb = 1024*1024;

   // retrieve space info from server

   statRes = StorageTargetInfo::statStoragePath(node, currentTargetID, &freeSpace, &totalSpace,
      &freeInodes, &totalInodes);
   if(statRes != FhgfsOpsErr_SUCCESS)
   {
      // don't log to stderr directly, because that would distort console output if server down
      fprintf(stderr, " [ERROR from %s: %s]\n", node->getNodeIDWithTypeStr().c_str(),
         FhgfsOpsErrTk::toErrString(statRes) );
   }

   // make values human readable

   freeSpaceGB = freeSpace / gb;
   totalSpaceGB = totalSpace / gb;
   freeSpacePercent = totalSpace ? (100.0 * freeSpace / totalSpace) : 0;
   freeInodesM = freeInodes / mb;
   totalInodesM = totalInodes / mb;
   freeInodesPercent = totalInodes ? (100.0 * freeInodes / totalInodes) : 0;

   *inOutOffset += sprintf(&inOutString[*inOutOffset],
      "%9.1fGB %9.1fGB %3.0f%% %10.1fM %10.1fM %3.0f%% ",
      (float)totalSpaceGB, (float)freeSpaceGB, (float)freeSpacePercent,
      (float)totalInodesM, (float)freeInodesM, (float)freeInodesPercent);

   return (statRes == FhgfsOpsErr_SUCCESS) ? APPCODE_NO_ERROR : APPCODE_RUNTIME_ERROR;
}

/**
 * add the space/inode information of the target to the output string of a table line
 *
 * @param inOutString the output string of a target for the output table
 * @param inOutOffset the offset in the inOutString to start with the space/inode information
 * @param currentBuddyGroupID the buddy group ID where the target belongs to
 * @param isPrimaryTarget true if this target is the primary target of the buddy group
 * @return 0 on success, all other values reports an error (APPCODE_...)
 */
int ModeListTargets::addBuddyGroupToLine(char* inOutString, int* inOutOffset,
   uint16_t currentBuddyGroupID, bool isPrimaryTarget)
{
   *inOutOffset += sprintf(&inOutString[*inOutOffset], "%13hu %12s ", currentBuddyGroupID,
      isPrimaryTarget ? MODELISTTARGETS_BUDDYGROUP_MEMBERTYPE_PRIMARY :
         MODELISTTARGETS_BUDDYGROUP_MEMBERTYPE_SECONDARY);

   return APPCODE_NO_ERROR;
}
