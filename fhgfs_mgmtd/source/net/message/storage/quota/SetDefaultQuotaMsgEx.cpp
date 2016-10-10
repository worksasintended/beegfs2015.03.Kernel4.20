#include <common/net/message/storage/quota/SetDefaultQuotaRespMsg.h>
#include <common/storage/quota/QuotaData.h>
#include <components/quota/QuotaManager.h>
#include <program/Program.h>

#include "SetDefaultQuotaMsgEx.h"


bool SetDefaultQuotaMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("SetDefaultQuotaMsg incoming");

   App* app = Program::getApp();

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG_CONTEXT(log, 4, std::string("Received a SetDefaultQuotaMsg from: ") + peer);

   bool respVal = false;

   if(app->getConfig()->getQuotaEnableEnforcement() )
   {
      QuotaManager* manager = app->getQuotaManager();

      if(type == QuotaDataType_USER)
         respVal = manager->updateDefaultUserLimits(inodes, size);
      else
         respVal = manager->updateDefaultGroupLimits(inodes, size);
   }
   else
   {
      log.log(Log_ERR, "Unable to set default quota limits. Configuration problem detected. Quota "
         "enforcement is not enabled for this management daemon, but a admin tried to set default "
         "quota limits. Fix this configuration problem or quota enforcement with default quota "
         "limits will not work correctly.");
   }

   // send response

   SetDefaultQuotaRespMsg respMsg(respVal);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return true;
}
