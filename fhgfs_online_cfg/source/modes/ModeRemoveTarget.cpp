#include <app/App.h>
#include <common/net/message/nodes/UnmapTargetMsg.h>
#include <common/net/message/nodes/UnmapTargetRespMsg.h>
#include <common/toolkit/MetadataTk.h>
#include <common/toolkit/NodesTk.h>
#include <common/toolkit/UnitTk.h>
#include <modes/modehelpers/ModeHelperGetNodes.h>
#include <program/Program.h>
#include "ModeRemoveTarget.h"


#define MODEREMOVETARGET_ARG_FORCE  "--force"



int ModeRemoveTarget::execute()
{
   const int mgmtTimeoutMS = 2500;

   int retVal = APPCODE_NO_ERROR;

   App* app = Program::getApp();
   DatagramListener* dgramLis = app->getDatagramListener();
   NodeStore* mgmtNodes = app->getMgmtNodes();
   std::string mgmtHost = app->getConfig()->getSysMgmtdHost();
   unsigned short mgmtPortUDP = app->getConfig()->getConnMgmtdPortUDP();
   StringMap* cfg = app->getConfig()->getUnknownConfigArgs();
   uint16_t cfgTargetID;

   // check privileges
   if(!ModeHelper::checkRootPrivileges() )
      return APPCODE_RUNTIME_ERROR;

   // check arguments

   StringMapIter iter = cfg->find(MODEREMOVETARGET_ARG_FORCE);
   if(iter != cfg->end() )
   {
      cfgForce = true;
      cfg->erase(iter);
   }

   if(cfg->empty() )
   {
      std::cerr << "No target ID specified." << std::endl;
      return APPCODE_RUNTIME_ERROR;
   }

   std::string targetIDStr = cfg->begin()->first;
   cfgTargetID = StringTk::strToUInt(targetIDStr);
   cfg->erase(cfg->begin() );

   bool isNumericRes = StringTk::isNumeric(targetIDStr);
   if(!isNumericRes)
   {
      std::cerr << "Invalid targetID given (must be numeric): " << targetIDStr << std::endl;
      return APPCODE_INVALID_CONFIG;
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

   bool isPartOfMBG = false;
   if(!cfgForce)
   {
      Node* mgmtNode = mgmtNodes->referenceFirstNode();
      if(!mgmtNode)
      {
         std::cerr << "Failed to reference management node." << std::endl;
         retVal = APPCODE_RUNTIME_ERROR;
      }

      if(isPartOfMirrorBuddyGroup(mgmtNode, cfgTargetID, &isPartOfMBG) )
         retVal = APPCODE_RUNTIME_ERROR;

      mgmtNodes->releaseNode(&mgmtNode);
   }

   if(!retVal && (cfgForce || !isPartOfMBG) )
      retVal = removeTarget(cfgTargetID);

   return retVal;
}

void ModeRemoveTarget::printHelp()
{
   std::cout << "MODE ARGUMENTS:" << std::endl;
   std::cout << " Mandatory:" << std::endl;
   std::cout << "  <targetID>             The ID of the target that should be removed." << std::endl;
   std::cout << std::endl;
   std::cout << " Optional:" << std::endl;
   std::cout << "  --force                Remove the target also when it is a member of a" << std::endl;
   std::cout << "                         mirror buddy group." << std::endl;
   std::cout << std::endl;
   std::cout << "USAGE:" << std::endl;
   std::cout << " This mode removes the mapping of a certain target to the corresponding storage" << std::endl;
   std::cout << " server." << std::endl;
   std::cout << std::endl;
   std::cout << " Example: Remove target with ID \"5\"" << std::endl;
   std::cout << "  $ beegfs-ctl --removetarget 5" << std::endl;
}

/**
 * @return APPCODE_...
 */
int ModeRemoveTarget::removeTarget(uint16_t targetID)
{
   int retVal = APPCODE_RUNTIME_ERROR;

   FhgfsOpsErr removeRes = removeTargetComm(targetID);

   if(removeRes != FhgfsOpsErr_SUCCESS)
   {
      std::cerr << "Failed to remove target: " << targetID <<
         " (Error: " << FhgfsOpsErrTk::toErrString(removeRes) << ")" << std::endl;
   }
   else
   {
      std::cout << "Removal of target succeeded: " << targetID << std::endl;
      retVal = APPCODE_NO_ERROR;
   }

   return retVal;
}

FhgfsOpsErr ModeRemoveTarget::removeTargetComm(uint16_t targetID)
{
   FhgfsOpsErr retVal = FhgfsOpsErr_INTERNAL;

   App* app = Program::getApp();
   NodeStore* mgmtNodes = app->getMgmtNodes();
   Node* mgmtNode = mgmtNodes->referenceFirstNode();

   bool commRes;
   char* respBuf = NULL;
   NetMessage* respMsg = NULL;
   UnmapTargetRespMsg* respMsgCast;

   UnmapTargetMsg msg(targetID);

   // request/response
   commRes = MessagingTk::requestResponse(
      mgmtNode, &msg, NETMSGTYPE_UnmapTargetResp, &respBuf, &respMsg);
   if(!commRes)
   {
      //std::cerr << "Network error." << std::endl;
      retVal = FhgfsOpsErr_COMMUNICATION;
      goto err_cleanup;
   }

   respMsgCast = (UnmapTargetRespMsg*)respMsg;

   retVal = (FhgfsOpsErr)respMsgCast->getValue();

err_cleanup:
   mgmtNodes->releaseNode(&mgmtNode);
   SAFE_DELETE(respMsg);
   SAFE_FREE(respBuf);

   return retVal;
}

/**
 * Checks if a target is part of a MirrorBuddyGroup
 *
 * @param mgmtNode the management node, must be referenced before given to this method
 * @param targetID the targetID to check
 * @param outIsPartOfMBG out value which is true if the target is part of a MBG, else false
 * @return 0 on success, all other values reports an error (FhgfsOpsErr_...)
 */
FhgfsOpsErr ModeRemoveTarget::isPartOfMirrorBuddyGroup(Node* mgmtNode, uint16_t targetID,
   bool* outIsPartOfMBG)
{
   *outIsPartOfMBG = false;

   // download MirrorBuddyGroups

   UInt16List buddyGroupIDs;
   UInt16List buddyGroupPrimaryTargetIDs;
   UInt16List buddyGroupSecondaryTargetIDs;

   if(!NodesTk::downloadMirrorBuddyGroups(mgmtNode, NODETYPE_Storage, &buddyGroupIDs,
      &buddyGroupPrimaryTargetIDs, &buddyGroupSecondaryTargetIDs, false) )
   {
      std::cerr << "Download of mirror buddy groups failed." << std::endl;
      return FhgfsOpsErr_COMMUNICATION;
   }


   UInt16ListIter buddyGroupIDIter = buddyGroupIDs.begin();
   UInt16ListIter primaryTargetIDIter = buddyGroupPrimaryTargetIDs.begin();
   UInt16ListIter secondaryTargetIDIter = buddyGroupSecondaryTargetIDs.begin();
   for( ; (buddyGroupIDIter != buddyGroupIDs.end() ) &&
          (primaryTargetIDIter != buddyGroupPrimaryTargetIDs.end() ) &&
          (secondaryTargetIDIter != buddyGroupSecondaryTargetIDs.end() );
         buddyGroupIDIter++, primaryTargetIDIter++, secondaryTargetIDIter++)
   {
      if( (*primaryTargetIDIter == targetID) || (*secondaryTargetIDIter== targetID) )
      {
         std::cerr << "Removing target " << targetID << " will damage the mirror buddy group " <<
            *buddyGroupIDIter << ". Restart the remove command with the parameter --force to "
            "ignore this warning." << std::endl;

         *outIsPartOfMBG = true;
         break;
      }
   }

   return FhgfsOpsErr_SUCCESS;
}
