#include <common/net/message/nodes/ChangeTargetConsistencyStatesRespMsg.h>
#include <common/nodes/TargetStateStore.h>
#include <program/Program.h>

#include "ChangeTargetConsistencyStatesMsgEx.h"

bool ChangeTargetConsistencyStatesMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
      char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG("Change target states incoming", Log_DEBUG,
         "Received a ChangeTargetConsistencyStatesMsg from: " + peer);

   App* app = Program::getApp();
   MgmtdTargetStateStore* targetStateStore = app->getTargetStateStore();
   MgmtdTargetStateStore* metaStateStore = app->getMetaStateStore();
   MirrorBuddyGroupMapper* mirrorBuddyGroupMapper = app->getMirrorBuddyGroupMapper();

   UInt16List targetIDs;
   UInt8List oldStates;
   UInt8List newStates;

   parseTargetIDs(&targetIDs);
   parseOldStates(&oldStates);
   parseNewStates(&newStates);

   FhgfsOpsErr result = FhgfsOpsErr_UNKNOWNTARGET;

   if (getNodeType() == NODETYPE_Storage)
      result = targetStateStore->changeConsistencyStatesFromLists(targetIDs, oldStates, newStates,
         mirrorBuddyGroupMapper);
   if (getNodeType() == NODETYPE_Meta)
      result = metaStateStore->changeConsistencyStatesFromLists(targetIDs, oldStates, newStates,
         NULL);


   ChangeTargetConsistencyStatesRespMsg respMsg(result);
   respMsg.serialize(respBuf, bufLen);

   // send response
   sock->sendto(respBuf, respMsg.getMsgLength(), 0, NULL, 0);

   if (getNodeType() == NODETYPE_Storage)
      targetStateStore->saveStates();

   return true;
}
