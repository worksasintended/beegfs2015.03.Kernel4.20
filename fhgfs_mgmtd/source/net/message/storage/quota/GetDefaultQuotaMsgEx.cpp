#include <common/net/message/storage/quota/GetDefaultQuotaRespMsg.h>
#include <components/quota/QuotaManager.h>
#include <program/Program.h>
#include "GetDefaultQuotaMsgEx.h"



bool GetDefaultQuotaMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("GetDefaultQuotaMsg incoming");

   App* app = Program::getApp();

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG_CONTEXT(log, 4, std::string("Received a GetDefaultQuotaMsg from: ") + peer);

   QuotaDefaultLimits limits;

   if(app->getConfig()->getQuotaEnableEnforcment() )
   {
      QuotaManager* manager = app->getQuotaManager();
      limits = QuotaDefaultLimits(manager->getDefaultLimits() );
   }

   GetDefaultQuotaRespMsg respMsg(limits);

   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return true;
}
