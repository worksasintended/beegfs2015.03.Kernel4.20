#include <common/net/message/nodes/GetStatesAndBuddyGroupsRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <program/Program.h>

#include "GetStatesAndBuddyGroupsMsgEx.h"

bool GetStatesAndBuddyGroupsMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   const char* logContext = "GetStatesAndBuddyGroupsMsg incoming";

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG(logContext, Log_DEBUG, "Received a GetStatesAndBuddyGroupsMsg from: " + peer);

   App* app = Program::getApp();
   TargetStateStore* targetStateStore = app->getTargetStateStore();
   MirrorBuddyGroupMapper* buddyGroupMapper = app->getMirrorBuddyGroupMapper();
   NodeType nodeType = getNodeType();

   // At the moment this is only implemented for storage nodes, because only storage nodes can be
   // part of a buddy group.
   if (nodeType != NODETYPE_Storage)
   {
      LogContext(logContext).logErr("Invalid node type: " + StringTk::intToStr(nodeType) );

      return false;
   }

   UInt16List targetIDs;
   UInt8List targetReachabilityStates;
   UInt8List targetConsistencyStates;

   UInt16List buddyGroupIDs;
   UInt16List primaryTargetIDs;
   UInt16List secondaryTargetIDs;

   targetStateStore->getStatesAndGroupsAsLists(buddyGroupMapper,
      targetIDs, targetReachabilityStates, targetConsistencyStates,
      buddyGroupIDs, primaryTargetIDs, secondaryTargetIDs);

   GetStatesAndBuddyGroupsRespMsg respMsg(&buddyGroupIDs, &primaryTargetIDs, &secondaryTargetIDs,
         &targetIDs, &targetReachabilityStates, &targetConsistencyStates);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0, NULL, 0);

   return true;
}

