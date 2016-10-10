#include <common/app/log/LogContext.h>
#include <common/Common.h>
#include <common/net/message/storage/quota/SetQuotaRespMsg.h>
#include <program/Program.h>

#include "SetQuotaMsgEx.h"



bool SetQuotaMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("SetQuotaMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG_CONTEXT(log, 4, std::string("Received a SetQuotaMsg from: ") + peer);

   int respVal = processQuotaLimits();

   // send response

   SetQuotaRespMsg respMsg(respVal);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return respVal;
}

/**
 * updates the quota limits
 */
bool SetQuotaMsgEx::processQuotaLimits()
{
   LogContext log("SetQuotaMsg process");
   bool retVal = true;

   App* app = Program::getApp();

   if(app->getConfig()->getQuotaEnableEnforcement() )
   {
      QuotaDataList limitList;
      this->parseQuotaDataList(&limitList);
      retVal = app->getQuotaManager()->updateQuotaLimits(&limitList);

      if(!retVal)
         log.logErr("Failed to update quota limits.");
   }
   else
      log.log(Log_ERR, "Unable to set quota limits. Configuration problem detected. Quota "
         "enforcement is not enabled for this management daemon, but a admin tried to set quota"
         "limits. Fix this configuration problem or quota enforcement will not work correctly.");

   return retVal;
}
