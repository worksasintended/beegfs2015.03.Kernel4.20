#include <common/net/message/storage/quota/SetDefaultQuotaRespMsg.h>
#include <common/storage/quota/QuotaData.h>
#include <components/quota/QuotaManager.h>
#include <program/Program.h>

#include "SetDefaultQuotaMsgEx.h"


bool SetDefaultQuotaMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("SetDefaultQuotaMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG_CONTEXT(log, 4, std::string("Received a SetDefaultQuotaMsg from: ") + peer);

   bool respVal = false;

   QuotaManager* manager = Program::getApp()->getQuotaManager();

   if(type == QuotaDataType_USER)
      respVal = manager->updateDefaultUserLimits(inodes, size);
   else
      respVal = manager->updateDefaultGroupLimits(inodes, size);

   // send response

   SetDefaultQuotaRespMsg respMsg(respVal);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return true;
}
