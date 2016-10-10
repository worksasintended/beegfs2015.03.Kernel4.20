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
   RequestExceededQuotaRespMsg respMsg(getQuotaDataType(), getExceededType(), FhgfsOpsErr_INVAL);
   respMsg.addMsgHeaderCompatFeatureFlag(REQUESTEXCEEDEDQUOTARESPMSG_FLAG_QUOTA_CONFIG);

   if(cfg->getQuotaEnableEnforcement() )
   {
      app->getQuotaManager()->getExceededQuotaStore()->getExceededQuota(
         respMsg.getExceededQuotaIDs(), getQuotaDataType(), getExceededType() );
      respMsg.setError(FhgfsOpsErr_SUCCESS);
   }
   else
   {
      log.log(Log_DEBUG, "Unable to provide exceeded quota IDs. "
         "The storage/metadata daemon on " + sock->getPeername() + " has quota enforcement enabled,"
         " but not this management daemon. "
         "Fix this configuration problem or quota enforcement will not work.");
   }

   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return true;
}
