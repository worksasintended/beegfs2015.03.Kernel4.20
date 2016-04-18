#include <common/net/message/nodes/UnmapTargetRespMsg.h>
#include <common/storage/StorageErrors.h>
#include <common/toolkit/MessagingTk.h>
#include <program/Program.h>
#include "UnmapTargetMsgEx.h"


bool UnmapTargetMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("UnmapTargetMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG_CONTEXT(log, Log_DEBUG, std::string("Received a UnmapTargetMsg from: ") + peer);

   App* app = Program::getApp();
   TargetMapper* targetMapper = app->getTargetMapper();

   uint16_t targetID = getTargetID();

   LOG_DEBUG_CONTEXT(log, Log_WARNING, "Unmapping target: " + StringTk::uintToStr(targetID) );

   bool targetExisted = targetMapper->unmapTarget(targetID);

   if(app->getConfig()->getQuotaEnableEnforcment() )
      app->getQuotaManager()->removeTargetFromQuotaDataStores(targetID);

   if(targetExisted)
   {
      // force update of capacity pools (especially to update buddy mirror capacity pool)
      app->getInternodeSyncer()->setForcePoolsUpdate();
   }


   UnmapTargetRespMsg respMsg(targetExisted ? FhgfsOpsErr_SUCCESS : FhgfsOpsErr_UNKNOWNTARGET);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0, NULL, 0);

   return true;
}

