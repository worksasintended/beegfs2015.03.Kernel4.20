#include "RemoveBuddyGroupMsgEx.h"

#include <common/net/message/nodes/RemoveBuddyGroupRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <program/Program.h>

static FhgfsOpsErr removeGroupOnRemoteNode(Node* node, uint16_t groupID, bool onlyCheck)
{
   char* buf = NULL;
   NetMessage* resultMsg = NULL;

   RemoveBuddyGroupMsg msg(NODETYPE_Storage, groupID, onlyCheck);

   bool commRes = MessagingTk::requestResponse(node, &msg,
         NETMSGTYPE_RemoveBuddyGroupResp, &buf, &resultMsg);
   if (!commRes)
      return FhgfsOpsErr_COMMUNICATION;

   FhgfsOpsErr result = static_cast<RemoveBuddyGroupRespMsg*>(resultMsg)->getResult();

   delete resultMsg;
   free(buf);

   return result;
}

bool RemoveBuddyGroupMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
      char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("RemoveBuddyGroupMsg incoming");

   LOG_DEBUG_CONTEXT(log, Log_DEBUG,"Received RemoveBuddyGroupMsg from: " + sock->getPeername());

   if (type != NODETYPE_Storage)
   {
      log.logErr("Attempted to remove non-storage buddy group. groupID: "
           + StringTk::uintToStr(groupID) + ", type: " + Node::nodeTypeToStr(type));
      RemoveBuddyGroupRespMsg result(FhgfsOpsErr_INTERNAL);
      result.serialize(respBuf, bufLen);
      sock->sendto(respBuf, result.getMsgLength(), 0, NULL, 0);
      return true;
   }

   NodeStoreServers* nodes = Program::getApp()->getStorageNodes();
   TargetMapper* targetMapper = Program::getApp()->getTargetMapper();
   MirrorBuddyGroupMapper* bgm = Program::getApp()->getMirrorBuddyGroupMapper();

   const MirrorBuddyGroup group = bgm->getMirrorBuddyGroup(groupID);

   if (group.firstTargetID == 0)
   {
      log.logErr("Attempted to remove unknown storage buddy group. groupID:"
            + StringTk::uintToStr(groupID));
      RemoveBuddyGroupRespMsg result(FhgfsOpsErr_UNKNOWNTARGET);
      result.serialize(respBuf, bufLen);
      sock->sendto(respBuf, result.getMsgLength(), 0, NULL, 0);
      return true;
   }

   Node* primary = NULL;
   Node* secondary = NULL;

   FhgfsOpsErr result;

   primary = nodes->referenceNode(targetMapper->getNodeID(group.firstTargetID));
   if (!primary)
   {
      log.logErr("Could not reference primary of group." + StringTk::uintToStr(groupID));
      result = FhgfsOpsErr_UNKNOWNTARGET;
      goto send_response;
   }

   if (!primary->getNodeFeatures()->getBitNonAtomic(STORAGE_FEATURE_REMOVEBUDDYGROUP))
   {
      result = FhgfsOpsErr_NOTSUPP;
      goto send_response;
   }

   secondary = nodes->referenceNode(targetMapper->getNodeID(group.secondTargetID));
   if (!secondary)
   {
      log.logErr("Could not reference secondary of group." + StringTk::uintToStr(groupID));
      result = FhgfsOpsErr_UNKNOWNTARGET;
      goto send_response;
   }

   if (!secondary->getNodeFeatures()->getBitNonAtomic(STORAGE_FEATURE_REMOVEBUDDYGROUP))
   {
      result = FhgfsOpsErr_NOTSUPP;
      goto send_response;
   }

   {
      FhgfsOpsErr checkPrimaryRes = removeGroupOnRemoteNode(primary, groupID, true);
      if (checkPrimaryRes != FhgfsOpsErr_SUCCESS)
      {
         log.log(Log_NOTICE, "Could not remove storage buddy group, targets are still in use. "
               "groupID: " + StringTk::uintToStr(groupID));
         result = checkPrimaryRes;
         goto send_response;
      }
   }

   {
      FhgfsOpsErr checkSecondaryRes = removeGroupOnRemoteNode(secondary, groupID, true);
      if (checkSecondaryRes != FhgfsOpsErr_SUCCESS)
      {
         log.log(Log_NOTICE, "Could not remove storage buddy group, targets are still in use. "
               "groupID: " + StringTk::uintToStr(groupID));
         result = checkSecondaryRes;
         goto send_response;
      }
   }

   if (!checkOnly)
   {
      bool removeRes = bgm->unmapMirrorBuddyGroup(groupID);
      if (!removeRes)
      {
         log.logErr("Could not unmap storage buddy group. groupID: "
               + StringTk::uintToStr(groupID));
         result = FhgfsOpsErr_INTERNAL;
         goto send_response;
      }

      // we don't care much about the results of group removal on the storage servers. if we fail
      // here, it is possible that the internode syncer has already removed the groups on remote
      // nodes.
      removeGroupOnRemoteNode(primary, groupID, false);
      removeGroupOnRemoteNode(secondary, groupID, false);
   }

   result = FhgfsOpsErr_SUCCESS;

send_response:
   if (primary)
      nodes->releaseNode(&primary);
   if (secondary)
      nodes->releaseNode(&secondary);

   RemoveBuddyGroupRespMsg resultMsg(result);
   resultMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, resultMsg.getMsgLength(), 0, NULL, 0);
   return true;
}
