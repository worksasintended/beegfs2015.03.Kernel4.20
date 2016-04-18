#include <common/net/message/nodes/GetTargetStatesRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <program/Program.h>

#include "GetTargetStatesMsgEx.h"


bool GetTargetStatesMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   const char* logContext = "GetTargetStatesMsg incoming";

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG(logContext, Log_DEBUG, "Received a GetTargetStatesMsg from: " + peer);

   App* app = Program::getApp();

   NodeType nodeType = getNodeType();

   TargetStateStore* targetStates;

   switch(nodeType)
   {
      case NODETYPE_Meta:
      {
         targetStates = app->getMetaStateStore();
      } break;

      case NODETYPE_Storage:
      {
         targetStates = app->getTargetStateStore();
      } break;

      default:
      {
         LogContext(logContext).logErr("Invalid node type: " + StringTk::intToStr(nodeType) );

         return false;
      } break;
   }


   UInt16List targetIDs;
   UInt8List reachabilityStates;
   UInt8List consistencyStates;

   targetStates->getStatesAsLists(targetIDs, reachabilityStates, consistencyStates);

   GetTargetStatesRespMsg respMsg(&targetIDs, &reachabilityStates, &consistencyStates);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0, NULL, 0);

   return true;
}

