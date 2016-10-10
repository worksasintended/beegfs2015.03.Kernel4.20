#include <app/App.h>
#include <common/net/message/nodes/RemoveBuddyGroupMsg.h>
#include <common/net/message/nodes/RemoveBuddyGroupRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/NodesTk.h>
#include <program/Program.h>

#include "ModeRemoveBuddyGroup.h"


#define ARG_MIRROR_GROUP_ID "--mirrorgroupid"
#define ARG_NODE_TYPE       "--nodetype"
#define ARG_DRY_RUN         "--dry-run"

int ModeRemoveBuddyGroup::execute()
{
   App* app = Program::getApp();
   StringMap* cfg = app->getConfig()->getUnknownConfigArgs();

   if (!ModeHelper::checkRootPrivileges())
      return APPCODE_RUNTIME_ERROR;

   // check arguments
   StringMapIter iter;

   iter = cfg->find(ARG_MIRROR_GROUP_ID);
   if (iter == cfg->end())
   {
      std::cerr << "Missing argument: " ARG_MIRROR_GROUP_ID << std::endl;
      return APPCODE_INVALID_CONFIG;
   }
   else if (!StringTk::isNumeric(iter->second))
   {
      std::cerr << "Invalid value for " ARG_MIRROR_GROUP_ID << std::endl;
      return APPCODE_INVALID_CONFIG;
   }

   uint16_t groupID = StringTk::strToUInt(iter->second.c_str());

   cfg->erase(iter);

   NodeType nodeType = ModeHelper::nodeTypeFromCfg(cfg);
   if (nodeType != NODETYPE_Storage)
   {
      std::cerr << "Invalid value for " ARG_NODE_TYPE " (must be `storage')" << std::endl;
      return APPCODE_INVALID_CONFIG;
   }

   iter = cfg->find(ARG_DRY_RUN);

   bool dryRun = iter != cfg->end();
   if (dryRun)
      cfg->erase(iter);

   if (ModeHelper::checkInvalidArgs(cfg))
      return APPCODE_INVALID_CONFIG;

   // check mgmt node
   const int mgmtTimeoutMS = 2500;
   if(!NodesTk::waitForMgmtHeartbeat(NULL, app->getDatagramListener(), app->getMgmtNodes(),
            app->getConfig()->getSysMgmtdHost(), app->getConfig()->getConnMgmtdPortUDP(),
            mgmtTimeoutMS))
   {
      std::cerr << "Management node communication failed: " << app->getConfig()->getSysMgmtdHost()
         << std::endl;
      return APPCODE_RUNTIME_ERROR;
   }

   FhgfsOpsErr removeRes = removeGroup(groupID, dryRun);

   if (removeRes == FhgfsOpsErr_SUCCESS)
   {
      std::cout << "Operation successful." << std::endl;
      return APPCODE_NO_ERROR;
   }
   else if (removeRes == FhgfsOpsErr_NOTEMPTY)
   {
      std::cout << "Could not remove buddy group: the group still contains data" << std::endl;
      return APPCODE_RUNTIME_ERROR;
   }
   else
   {
      std::cerr << "Could not remove buddy group: " << FhgfsOpsErrTk::toErrString(removeRes)
         << std::endl;
      return APPCODE_RUNTIME_ERROR;
   }
}

void ModeRemoveBuddyGroup::printHelp()
{
   std::cout <<
      "MODE ARGUMENTS:\n"
      " Mandatory:\n"
      "  --mirrorgroupid=<groupID>    ID of the group that is to be removed.\n"
      "  --nodetype=<type>            type must be `storage'. Buddy group removal is\n"
      "                               not yet supported for meta buddy groups.\n"
      "\n"
      " Optional:\n"
      "  --dry-run                    Only check whether the group could safely be\n"
      "                               removed, but do not actually remove the group.\n"
      "\n"
      "USAGE:\n"
      " To determine whether storage buddy group 7 still contains active data:\n"
      "   $ beegfs-ctl --removemirrorgroup --mirrorgroupid=7 --nodetype=storage \\\n"
      "        --dry-run\n"
      "\n"
      " To actually remove storage buddy group 7:\n"
      "   $ beegfs-ctl --removemirrorgroup --mirrorgroupid=7 --nodetype=storage";
   std::cout << std::flush;
}

FhgfsOpsErr ModeRemoveBuddyGroup::removeGroup(const uint16_t groupID, const bool dryRun)
{
   NodeStoreServers* store = Program::getApp()->getMgmtNodes();

   char* buf = NULL;
   NetMessage* resultMsg = NULL;

   RemoveBuddyGroupMsg msg(NODETYPE_Storage, groupID, dryRun);

   Node* node = store->referenceFirstNode();

   if (!node->getNodeFeatures()->getBitNonAtomic(MGMT_FEATURE_REMOVEBUDDYGROUP))
   {
      store->releaseNode(&node);
      return FhgfsOpsErr_NOTSUPP;
   }

   bool commRes = MessagingTk::requestResponse(node, &msg,
         NETMSGTYPE_RemoveBuddyGroupResp, &buf, &resultMsg);
   if (!commRes)
   {
      store->releaseNode(&node);
      return FhgfsOpsErr_COMMUNICATION;
   }

   FhgfsOpsErr result = static_cast<RemoveBuddyGroupRespMsg*>(resultMsg)->getResult();

   delete resultMsg;
   free(buf);

   store->releaseNode(&node);
   return result;
}
