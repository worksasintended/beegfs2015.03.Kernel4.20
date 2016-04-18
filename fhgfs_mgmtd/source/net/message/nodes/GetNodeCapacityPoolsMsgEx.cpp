#include <common/net/message/nodes/GetNodeCapacityPoolsRespMsg.h>
#include <common/nodes/TargetCapacityPools.h>
#include <program/Program.h>
#include "GetNodeCapacityPoolsMsgEx.h"

bool GetNodeCapacityPoolsMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   const char* logContext = "GetNodeCapacityPools incoming";

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG(logContext, Log_DEBUG, "Received a GetNodeCapacityPoolsMsg from: " + peer);

   CapacityPoolQueryType poolType = getCapacityPoolQueryType();

   LOG_DEBUG(logContext, Log_SPAM, "PoolType: " + StringTk::intToStr(poolType) );

   App* app = Program::getApp();

   UInt16List listNormal;
   UInt16List listLow;
   UInt16List listEmergency;

   switch(poolType)
   {
      case CapacityPoolQuery_META:
      {
         NodeCapacityPools* pools = app->getMetaCapacityPools();
         pools->getPoolsAsLists(listNormal, listLow, listEmergency);
      } break;

      case CapacityPoolQuery_STORAGE:
      {
         TargetCapacityPools* pools = app->getStorageCapacityPools();
         pools->getPoolsAsLists(listNormal, listLow, listEmergency);
      } break;

      case CapacityPoolQuery_STORAGEBUDDIES:
      {
         NodeCapacityPools* pools = app->getStorageBuddyCapacityPools();
         pools->getPoolsAsLists(listNormal, listLow, listEmergency);
      } break;

      default:
      {
         LogContext(logContext).logErr("Invalid pool type: " + StringTk::intToStr(poolType) );

         return false;
      } break;
   }


   GetNodeCapacityPoolsRespMsg respMsg(&listNormal, &listLow, &listEmergency);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );


   return true;
}

