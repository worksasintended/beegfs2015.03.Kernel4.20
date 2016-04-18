#include <common/net/message/nodes/GetMirrorBuddyGroupsRespMsg.h>
#include <common/nodes/MirrorBuddyGroupMapper.h>
#include <program/Program.h>
#include "GetMirrorBuddyGroupsMsgEx.h"


bool GetMirrorBuddyGroupsMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   const char* logContext = "GetMirrorBuddyGroupsMsg incoming";

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG(logContext, Log_DEBUG, "Received a GetMirrorBuddyGroupsMsg from: " + peer);

   App* app = Program::getApp();

   MirrorBuddyGroupMapper* buddyGroupMapper;

   NodeType nodeType = getNodeType();

   switch(nodeType)
   {
      case NODETYPE_Storage:
      {
         buddyGroupMapper = app->getMirrorBuddyGroupMapper();
      } break;

      default:
      {
         LogContext(logContext).logErr("Invalid node type: " + StringTk::intToStr(nodeType) );

         return false;
      } break;
   }

   UInt16List buddyGroupIDs;
   UInt16List primaryTargetIDs;
   UInt16List secondaryTargetIDs;

   buddyGroupMapper->getMappingAsLists(buddyGroupIDs, primaryTargetIDs, secondaryTargetIDs);

   // send response
   GetMirrorBuddyGroupsRespMsg respMsg(&buddyGroupIDs, &primaryTargetIDs, &secondaryTargetIDs);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return true;
}

