#include <common/net/message/nodes/SetMirrorBuddyGroupRespMsg.h>
#include <common/nodes/MirrorBuddyGroupMapper.h>
#include <common/toolkit/MessagingTk.h>

#include <program/Program.h>

#include "SetMirrorBuddyGroupMsgEx.h"


bool SetMirrorBuddyGroupMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("SetMirrorBuddyGroupMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG_CONTEXT(log, Log_DEBUG, std::string("Received a SetMirrorBuddyGroupMsg from: ") + peer);

   App* app = Program::getApp();
   MirrorBuddyGroupMapper* buddyGroupMapper = app->getMirrorBuddyGroupMapper();
   InternodeSyncer* internodeSyncer = app->getInternodeSyncer();

   uint16_t buddyGroupID = this->getBuddyGroupID();
   uint16_t primaryTargetID = this->getPrimaryTargetID();
   uint16_t secondaryTargetID = this->getSecondaryTargetID();
   bool allowUpdate = this->getAllowUpdate();
   uint16_t newBuddyGroupID = 0;

   FhgfsOpsErr mapResult = buddyGroupMapper->mapMirrorBuddyGroup(buddyGroupID, primaryTargetID,
      secondaryTargetID, allowUpdate, &newBuddyGroupID);

   if (mapResult == FhgfsOpsErr_SUCCESS)
   {
      app->getHeartbeatMgr()->notifyAsyncAddedMirrorBuddyGroup(newBuddyGroupID, primaryTargetID,
         secondaryTargetID);
      app->getHeartbeatMgr()->notifyAsyncRefreshCapacityPools();
   }

   // Force pools update, so that newly created group is put into correct capacity pool.
   internodeSyncer->setForcePoolsUpdate();

   // send response
   SetMirrorBuddyGroupRespMsg respMsg(mapResult, newBuddyGroupID);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return true;
}

