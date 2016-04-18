#include <common/app/log/LogContext.h>
#include <common/net/message/storage/quota/RequestExceededQuotaRespMsg.h>
#include "program/Program.h"

#include "RequestExceededQuotaMsgEx.h"


bool RequestExceededQuotaMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("RequestExceededQuotaMsgEx incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG_CONTEXT(log, Log_DEBUG, std::string("Received a RequestExceededQuotaMsgEx from: ") +
      peer);

   App* app = Program::getApp();
   Config* cfg = app->getConfig();

   // get exceeded quota ID list
   RequestExceededQuotaRespMsg respMsg(getQuotaDataType(), getExceededType() );

   if(cfg->getQuotaEnableEnforcment() )
      app->getQuotaManager()->getExceededQuotaStore()->getExceededQuota(
         respMsg.getExceededQuotaIDs(), getQuotaDataType(), getExceededType() );
   else
      log.log(Log_ERR, "Unable to provide exceeded quota IDs. "
         "The storage daemon on " + sock->getPeername() + " has quota enforcement enabled, "
         "but not this management daemon. Fix this configuration problem or quota enforcement will "
         "not work.");

   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return true;
}
