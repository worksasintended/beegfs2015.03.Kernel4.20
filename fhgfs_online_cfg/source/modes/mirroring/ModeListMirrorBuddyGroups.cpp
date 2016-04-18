#include <common/nodes/MirrorBuddyGroupMapper.h>
#include <common/toolkit/NodesTk.h>

#include <program/Program.h>

#include "ModeListMirrorBuddyGroups.h"

int ModeListMirrorBuddyGroups::execute()
{
   const int mgmtTimeoutMS = 2500;

   int retVal = APPCODE_RUNTIME_ERROR;

   App* app = Program::getApp();
   DatagramListener* dgramLis = app->getDatagramListener();
   NodeStoreServers* mgmtNodes = app->getMgmtNodes();
   std::string mgmtHost = app->getConfig()->getSysMgmtdHost();
   unsigned short mgmtPortUDP = app->getConfig()->getConnMgmtdPortUDP();
   StringMap* cfg = app->getConfig()->getUnknownConfigArgs();

   UInt16List buddyGroupIDs;
   UInt16List primaryTargetIDs;
   UInt16List secondaryTargetIDs;

   if(ModeHelper::checkInvalidArgs(cfg) )
      return APPCODE_INVALID_CONFIG;

   // check mgmt node

   if(!NodesTk::waitForMgmtHeartbeat(
      NULL, dgramLis, mgmtNodes, mgmtHost, mgmtPortUDP, mgmtTimeoutMS) )
   {
      std::cerr << "Management node communication failed: " << mgmtHost << std::endl;
      return APPCODE_RUNTIME_ERROR;
   }

   // download buddy groups

   Node* mgmtNode = mgmtNodes->referenceFirstNode();

   if(!NodesTk::downloadMirrorBuddyGroups(mgmtNode, NODETYPE_Storage, &buddyGroupIDs,
      &primaryTargetIDs, &secondaryTargetIDs, false) )
   {
      std::cerr << "Download of mirror buddy groups failed." << std::endl;
      retVal = APPCODE_RUNTIME_ERROR;
      goto cleanup_mgmt;
   }

   // print results
   printGroups(buddyGroupIDs, primaryTargetIDs, secondaryTargetIDs);

   retVal = APPCODE_NO_ERROR;


cleanup_mgmt:
   mgmtNodes->releaseNode(&mgmtNode);

   return retVal;
}

void ModeListMirrorBuddyGroups::printHelp()
{
   std::cout << "USAGE:" << std::endl;
   std::cout << " This mode lists the available mirror buddy groups with their mapped targets." << std::endl;
   std::cout << std::endl;
   std::cout << " Example: List registered mirror buddy groups" << std::endl;
   std::cout << "  $ beegfs-ctl --listmirrorgroups" << std::endl;
}

void ModeListMirrorBuddyGroups::printGroups(UInt16List& buddyGroupIDs, UInt16List& primaryTargetIDs,
   UInt16List& secondaryTargetIDs)
{
   if(buddyGroupIDs.empty() )
   {
      std::cerr << "No mirror buddy groups defined." << std::endl;
      return;
   }

   // print header
   printf("%17s %17s %17s\n", "BuddyGroupID", "PrimaryTargetID", "SecondaryTargetID");
   printf("%17s %17s %17s\n", "============", "===============", "=================");


   // print buddy group mappings

   UInt16ListConstIter groupsIter = buddyGroupIDs.begin();
   UInt16ListConstIter primaryTargetsIter = primaryTargetIDs.begin();
   UInt16ListConstIter secondaryTargetsIter = secondaryTargetIDs.begin();

   for(; groupsIter != buddyGroupIDs.end();
      groupsIter++, primaryTargetsIter++, secondaryTargetsIter++)
   {
      printf("%17hu %17hu %17hu\n", *groupsIter, *primaryTargetsIter, *secondaryTargetsIter);
   }

}

