#include <app/App.h>
#include <common/nodes/NodeStore.h>
#include <common/toolkit/NodesTk.h>
#include <program/Program.h>

#include "ModeAddMirrorBuddyGroup.h"



#define MODEADDMIRRORBUDDYGROUP_ARG_PRIMARY      "--primary"
#define MODEADDMIRRORBUDDYGROUP_ARG_SECONDARY    "--secondary"
#define MODEADDMIRRORBUDDYGROUP_ARG_GROUPID      "--groupid"
#define MODEADDMIRRORBUDDYGROUP_ARG_AUTOMATIC    "--automatic"
#define MODEADDMIRRORBUDDYGROUP_ARG_FORCE        "--force"
#define MODEADDMIRRORBUDDYGROUP_ARG_DRYRUN       "--dryrun"
#define MODEADDMIRRORBUDDYGROUP_ARG_UNIQUE_ID    "--uniquegroupid"


int ModeAddMirrorBuddyGroup::execute()
{
   const int mgmtTimeoutMS = 2500;

   int retVal = APPCODE_RUNTIME_ERROR;

   App* app = Program::getApp();
   DatagramListener* dgramLis = app->getDatagramListener();
   NodeStore* mgmtNodes = app->getMgmtNodes();
   NodeStore* storageNodes = app->getStorageNodes();

   std::string mgmtHost = app->getConfig()->getSysMgmtdHost();
   unsigned short mgmtPortUDP = app->getConfig()->getConnMgmtdPortUDP();

   StringMap* cfg = app->getConfig()->getUnknownConfigArgs();

   uint16_t cfgPrimaryTargetID = 0;
   uint16_t cfgSecondaryTargetID = 0;
   uint16_t cfgGroupID = 0;

   // check privileges
   if(!ModeHelper::checkRootPrivileges() )
      return APPCODE_RUNTIME_ERROR;

   // check arguments
   StringMapIter iter;

   // check if automatic mode is selected
   iter = cfg->find(MODEADDMIRRORBUDDYGROUP_ARG_AUTOMATIC);
   if(iter != cfg->end() )
   {
      cfgAutomatic = true;
      cfg->erase(iter);
   }

   // check if the automatic mode should ignore warnings
   iter = cfg->find(MODEADDMIRRORBUDDYGROUP_ARG_FORCE);
   if(iter != cfg->end() )
   {
      cfgForce = true;
      cfg->erase(iter);
   }

   // check if the automatic mode should only print the selected configuration
   iter = cfg->find(MODEADDMIRRORBUDDYGROUP_ARG_DRYRUN);
   if(iter != cfg->end() )
   {
      cfgDryrun = true;
      cfg->erase(iter);
   }

   /* check if the automatic mode should use unique MirrorBuddyGroupIDs.
   The ID is not used as storage targetNumID */
   iter = cfg->find(MODEADDMIRRORBUDDYGROUP_ARG_UNIQUE_ID);
   if(iter != cfg->end() )
   {
      cfgUnique = true;
      cfg->erase(iter);
   }

   if(cfgAutomatic)
   { // sanity checks of automatic mode parameters
      if(cfgForce && cfgDryrun)
      {
         std::cerr << "Invalid arguments only one of " << MODEADDMIRRORBUDDYGROUP_ARG_FORCE <<
            " and " << MODEADDMIRRORBUDDYGROUP_ARG_DRYRUN << " is allowed." << std::endl;
         return APPCODE_INVALID_CONFIG;
      }
   }
   else
   {
      if(cfgForce)
      { // sanity checks of automatic mode parameter
         std::cerr << "Invalid argument "<< MODEADDMIRRORBUDDYGROUP_ARG_FORCE <<
            " is not allowed without " << MODEADDMIRRORBUDDYGROUP_ARG_AUTOMATIC << std::endl;
         return APPCODE_INVALID_CONFIG;
      }

      if(cfgDryrun)
      { // sanity checks of automatic mode parameter
         std::cerr << "Invalid argument "<< MODEADDMIRRORBUDDYGROUP_ARG_DRYRUN <<
            " is not allowed without " << MODEADDMIRRORBUDDYGROUP_ARG_AUTOMATIC << std::endl;
         return APPCODE_INVALID_CONFIG;
      }

      if(cfgUnique)
      { // sanity checks of automatic mode parameter
         std::cerr << "Invalid argument "<< MODEADDMIRRORBUDDYGROUP_ARG_UNIQUE_ID <<
            " is not allowed without " << MODEADDMIRRORBUDDYGROUP_ARG_AUTOMATIC << std::endl;
         return APPCODE_INVALID_CONFIG;
      }

      iter = cfg->find(MODEADDMIRRORBUDDYGROUP_ARG_PRIMARY);
      if(iter != cfg->end())
      { // found a primary target ID
         bool isNumericRes = StringTk::isNumeric(iter->second);
         if(!isNumericRes)
         {
            std::cerr << "Invalid primary target ID given (must be numeric): " << iter->second
               << std::endl;
            return APPCODE_INVALID_CONFIG;
         }

         cfgPrimaryTargetID = StringTk::strToUInt(iter->second);
         cfg->erase(iter);
      }
      else
      { // primary target ID is mandatory but wasn't found
         std::cerr << "No primary target ID specified." << std::endl;
         return APPCODE_INVALID_CONFIG;
      }

      iter = cfg->find(MODEADDMIRRORBUDDYGROUP_ARG_SECONDARY);
      if(iter != cfg->end())
      { // found a secomdary target ID
         bool isNumericRes = StringTk::isNumeric(iter->second);
         if(!isNumericRes)
         {
            std::cerr << "Invalid secondary target ID given (must be numeric): " << iter->second
               << std::endl;
            return APPCODE_INVALID_CONFIG;
         }

         cfgSecondaryTargetID = StringTk::strToUInt(iter->second);
         cfg->erase(iter);
      }
      else
      { // secondary target ID is mandatory but wasn't found
         std::cerr << "No secondary target ID specified." << std::endl;
         return APPCODE_INVALID_CONFIG;
      }

      iter = cfg->find(MODEADDMIRRORBUDDYGROUP_ARG_GROUPID);
      if(iter != cfg->end() )
      { // found a group ID
         bool isNumericRes = StringTk::isNumeric(iter->second);
         if(!isNumericRes)
         {
            std::cerr << "Invalid mirror buddy group ID given (must be numeric): " << iter->second
               << std::endl;
            return APPCODE_INVALID_CONFIG;
         }

         cfgGroupID = StringTk::strToUInt(iter->second);
         cfg->erase(iter);
      }
      else
      {
         cfgGroupID = 0;
      }
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

   if(cfgAutomatic)
      retVal = doAutomaticMode(mgmtNodes, storageNodes, cfgForce, cfgDryrun, cfgUnique);
   else
      retVal = addGroup(mgmtNodes, cfgPrimaryTargetID, cfgSecondaryTargetID, cfgGroupID);

   return retVal;
}

void ModeAddMirrorBuddyGroup::printHelp()
{
   std::cout << "MODE ARGUMENTS:" << std::endl;
   std::cout << " Mandatory for automatic group creation:" << std::endl;
   std::cout << "  --automatic             Generates the mirror buddy groups automatically with" << std::endl;
   std::cout << "                          respect to the location of the targets. This mode" << std::endl;
   std::cout << "                          aborts the creation of the mirror buddy groups if some" << std::endl;
   std::cout << "                          constraints are not fulfilled." << std::endl;
   std::cout << std::endl;
   std::cout << " Mandatory for manual group creation:" << std::endl;
   std::cout << "  --primary=<targetID>    The ID of the primary target for this mirror buddy" << std::endl;
   std::cout << "                          group." << std::endl;
   std::cout << "  --secondary=<targetID>  The ID of the secondary target for this mirror buddy" << std::endl;
   std::cout << "                          group." << std::endl;
   std::cout << std::endl;
   std::cout << " Optional for automatic group creation:" << std::endl;
   std::cout << "  --force                 Ignore the warnings of the automatic mode." << std::endl;
   std::cout << "  --dryrun                Only print the selected configuration of the" << std::endl;
   std::cout << "                          mirror buddy groups." << std::endl;
   std::cout << "  --uniquegroupid         Use unique mirror buddy group IDs, distinct from" << std::endl;
   std::cout << "                          storage target IDs." << std::endl;
   std::cout << std::endl;
   std::cout << " Optional for manual group creation:" << std::endl;
   std::cout << "  --groupid=<groupID>     Use the given ID for the new buddy group." << std::endl;
   std::cout << "                          (Valid range: 1..65535)" << std::endl;
   std::cout << std::endl;
   std::cout << "USAGE:" << std::endl;
   std::cout << " This mode adds a mirror buddy group and maps two storage targets to the group." << std::endl;
   std::cout << std::endl;
   std::cout << " Example: Automatically generate mirror buddy groups from all targets" << std::endl;
   std::cout << "  $ beegfs-ctl --addmirrorgroup --automatic" << std::endl;
}

/**
 * Creates the MirrorBuddyGroups without user interaction
 *
 * @param mgmtNodes NodeStore with the management nodes
 * @param storageNodes NodeStore with all storage servers
 * @param doForce if true ignore warnings and create the selected MirrorBuddyGroups
 * @param doDryrun if true only select MirrorBuddyGroups and print them but do not create them
 * @param useUniqueGroupIDs if true use unique MirrorBuddyGroupIDs. The ID is not used as storage
 *    targetNumID.
 * @return FhgfsOpsErr
 */
FhgfsOpsErr ModeAddMirrorBuddyGroup::doAutomaticMode(NodeStore* mgmtNodes, NodeStore* storageNodes,
   bool doForce, bool doDryrun, bool useUniqueGroupIDs)
{
   FhgfsOpsErr retVal = FhgfsOpsErr_INTERNAL;

   NodeList storageNodesList;

   TargetMapper systemTargetMapper;
   UInt16List mappedTargetIDs;
   UInt16List mappedNodeIDs;

   Node* mgmtNode = mgmtNodes->referenceFirstNode();
   if(!mgmtNode)
   {
      std::cerr << "Failed to reference management node." << std::endl;
      return FhgfsOpsErr_UNKNOWNNODE;
   }


   //download the list of storage servers from the management node
   if(!NodesTk::downloadNodes(mgmtNode, NODETYPE_Storage, &storageNodesList, false, NULL))
   {
      std::cerr << "Node download failed." << std::endl;
      mgmtNodes->releaseNode(&mgmtNode);
      return FhgfsOpsErr_COMMUNICATION;
   }
   NodesTk::moveNodesFromListToStore(&storageNodesList, storageNodes);


   // download the target mappings from management daemon
   if(!NodesTk::downloadTargetMappings(mgmtNode, &mappedTargetIDs, &mappedNodeIDs, false) )
   {
      std::cerr << "Target mappings download failed." << std::endl;
      mgmtNodes->releaseNode(&mgmtNode);

      return FhgfsOpsErr_COMMUNICATION;
   }
   systemTargetMapper.syncTargetsFromLists(mappedTargetIDs, mappedNodeIDs);


   // Download mirror buddy groups
   UInt16List oldBuddyGroupIDs;
   UInt16List oldPrimaryTargetIDs;
   UInt16List oldSecondaryTargetIDs;

   if(!NodesTk::downloadMirrorBuddyGroups(mgmtNode, NODETYPE_Storage, &oldBuddyGroupIDs,
      &oldPrimaryTargetIDs, &oldSecondaryTargetIDs, false) )
   {
      std::cerr << "Download of existing mirror buddy groups failed." << std::endl;
      mgmtNodes->releaseNode(&mgmtNode);

      return FhgfsOpsErr_COMMUNICATION;
   }

   mgmtNodes->releaseNode(&mgmtNode);


   // prepare target list, remove the targets from existing MirrorBuddyGroups
   UInt16List origTargetIDs;
   UInt16List origNodeIDs;

   systemTargetMapper.getMappingAsLists(origTargetIDs, origNodeIDs);

   TargetMapper localTargetMapper;
   localTargetMapper.syncTargetsFromLists(origTargetIDs, origNodeIDs);

   removeTargetsFromExistingMirrorBuddyGroups(&localTargetMapper, &oldBuddyGroupIDs,
      &oldPrimaryTargetIDs, &oldSecondaryTargetIDs);


   UInt16List buddyGroupIDs;
   MirrorBuddyGroupList buddyGroups;

   retVal = generateMirrorBuddyGroups(storageNodes, &systemTargetMapper, &buddyGroupIDs,
      &buddyGroups, &localTargetMapper, &oldBuddyGroupIDs, useUniqueGroupIDs);

   printAutomaticResults(retVal, doForce, storageNodes, &systemTargetMapper, &buddyGroupIDs,
      &buddyGroups, &oldBuddyGroupIDs, &oldPrimaryTargetIDs, &oldSecondaryTargetIDs);

   bool retValCreateGroups = createMirrorBuddyGroups(mgmtNodes, retVal, doForce, doDryrun,
      &buddyGroupIDs, &buddyGroups);

   if(!retValCreateGroups)
      std::cerr << "An error occurred during mirror buddy group creation. "
         "It is possible that some mirror buddy groups have been created anyways. "
         "Please check the messages printed above."
         << std::endl;

   return retVal;
}

/**
 * Prints the given MirrorBuddyGroups on the console
 *
 * @param storageNodes NodeStore with all storage servers
 * @param targetMapper TargetMapper with all storage targets of the system
 * @param buddyGroupIDs A list with the MirrorBuddyGroupIDs to print
 * @param primaryTargetIDs A list with the TargetNumIDs of the primary target to print
 * @param secondaryTargetIDs A list with the TargetNumIDs of the secondary target to print
 */
void ModeAddMirrorBuddyGroup::printMirrorBuddyGroups(NodeStore* storageNodes,
   TargetMapper* targetMapper, UInt16List* buddyGroupIDs, UInt16List* primaryTargetIDs,
   UInt16List* secondaryTargetIDs)
{
   MirrorBuddyGroupList buddyGroups;

   UInt16ListIter primaryIDsIter = primaryTargetIDs->begin();
   UInt16ListIter secondaryIDsIter = secondaryTargetIDs->begin();

   for(; primaryIDsIter != primaryTargetIDs->end() && secondaryIDsIter != secondaryTargetIDs->end();
      primaryIDsIter++, secondaryIDsIter++)
   {
      buddyGroups.push_back(MirrorBuddyGroup(*primaryIDsIter, *secondaryIDsIter) );
   }

   printMirrorBuddyGroups(storageNodes, targetMapper, buddyGroupIDs, &buddyGroups);
}

/**
 * Prints the given MirrorBuddyGroups on the console
 *
 * @param storageNodes NodeStore with all storage servers
 * @param targetMapper TargetMapper with all storage targets of the system
 * @param buddyGroupIDs A list with all MirrorBuddyGroupIDs to print
 * @param buddyGroups A list with all MirrorBuddyGroups to print
 */
void ModeAddMirrorBuddyGroup::printMirrorBuddyGroups(NodeStore* storageNodes,
   TargetMapper* targetMapper, UInt16List* buddyGroupIDs, MirrorBuddyGroupList* buddyGroups)
{
   UInt16ListIter idIter = buddyGroupIDs->begin();
   MirrorBuddyGroupListIter groupIter = buddyGroups->begin();

   printf("%12s %11s %s\n", "BuddyGroupID", "Target type", "Target");
   printf("%12s %11s %s\n", "============", "===========", "======");

   for(; (idIter != buddyGroupIDs->end()) && (groupIter != buddyGroups->end() );
         idIter++, groupIter++)
   {
      Node* firstNode = storageNodes->referenceNodeByTargetID(groupIter->firstTargetID,
         targetMapper);
      Node* secondNode = storageNodes->referenceNodeByTargetID(groupIter->secondTargetID,
         targetMapper);

      std::string firstTarget = StringTk::uintToStr(groupIter->firstTargetID);
      std::string secondTarget = StringTk::uintToStr(groupIter->secondTargetID);

      printf("%12hu %11s %8s @ %s\n", *idIter, "primary", firstTarget.c_str(),
         firstNode->getNodeIDWithTypeStr().c_str() );
      printf("%12s %11s %8s @ %s\n", "", "secondary", secondTarget.c_str(),
         secondNode->getNodeIDWithTypeStr().c_str() );

      storageNodes->releaseNode(&firstNode);
      storageNodes->releaseNode(&secondNode);
   }
}

/**
 * Prints the results of the automatic mode to the console.
 *
 * @param retValGeneration the return value of the MirrorBuddyGroup generation
 * @param doForce true if the creation of the MirrorBuddyGroups is forced
 * @param storageNodes NodeStore with all storage servers
 * @param targetMapper TargetMapper with all storage targets of the system
 * @param newBuddyGroupIDs A list with all new MirrorBuddyGroupIDs to print
 * @param newBuddyGroups A list with all new MirrorBuddyGroups to print
 * @param oldBuddyGroupIDs A list with the MirrorBuddyGroupIDs of existing MirrorBuddyGroups to
 *        print
 * @param oldPrimaryTargetIDs A list with the TargetNumIDs of the primary target of existing
 *        MirrorBuddyGroups to print
 * @param oldSecondaryTargetIDs A list with the TargetNumIDs of the secondary target of existing
 *        MirrorBuddyGroups to print
 */
void ModeAddMirrorBuddyGroup::printAutomaticResults(FhgfsOpsErr retValGeneration, bool doForce,
   NodeStore* storageNodes, TargetMapper* targetMapper, UInt16List* newBuddyGroupIDs,
   MirrorBuddyGroupList* newBuddyGroups, UInt16List* oldBuddyGroupIDs,
   UInt16List* oldPrimaryTargetIDs, UInt16List* oldSecondaryTargetIDs)
{
   std::cout << std::endl;
   if(oldBuddyGroupIDs->size() > 0)
   {
      std::cout << "Existing MirrorBuddyGroups:" << std::endl;
      printMirrorBuddyGroups(storageNodes, targetMapper, oldBuddyGroupIDs, oldPrimaryTargetIDs,
         oldSecondaryTargetIDs);
      std::cout << std::endl;
   }

   std::cout << "New MirrorBuddyGroups:" << std::endl;
   printMirrorBuddyGroups(storageNodes, targetMapper, newBuddyGroupIDs, newBuddyGroups);
   std::cout << std::endl;

   if( (retValGeneration == FhgfsOpsErr_INVAL) && !doForce)
   { // abort if a warning occurred and not the force parameter is given
      std::cerr << "Some issues occurred during the mirror buddy group generation. "
         "No mirror buddy groups were created. "
         "Check the warnings printed above, and restart the command "
         "with --force to ignore these warnings." << std::endl;
   }
   else
   if( (retValGeneration != FhgfsOpsErr_SUCCESS) && (retValGeneration != FhgfsOpsErr_INVAL) )
   {
      std::cerr << "An error occurred during mirror buddy group generation. No mirror buddy groups "
         "were created." << std::endl;
   }
}
