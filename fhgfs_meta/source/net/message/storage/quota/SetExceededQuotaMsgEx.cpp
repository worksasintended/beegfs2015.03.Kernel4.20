#include <common/app/log/LogContext.h>
#include <common/net/message/storage/quota/SetExceededQuotaRespMsg.h>
#include "program/Program.h"

#include "SetExceededQuotaMsgEx.h"



bool SetExceededQuotaMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("SetExceededQuotaMsgEx incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG_CONTEXT(log, Log_DEBUG, std::string("Received a SetExceededQuotaMsgEx from: ") + peer);

   bool retVal = true;
   FhgfsOpsErr errorCode = FhgfsOpsErr_SUCCESS;

   if(Program::getApp()->getConfig()->getQuotaEnableEnforcement() )
   {
      // update exceeded quota
      UIntList tmpIDList;
      parseIDList(&tmpIDList);
      Program::getApp()->getExceededQuotaStore()->updateExceededQuota(&tmpIDList,
         this->getQuotaDataType(), this->getExceededType() );
   }
   else
   {
      log.log(Log_ERR, "Unable to set exceeded quota IDs. Configuration problem detected. "
         "The management daemon on " + sock->getPeername() + " has quota enforcement enabled, "
         "but not this metadata daemon. Fix this configuration problem or quota enforcement will "
         "not work correctly.");

      errorCode = FhgfsOpsErr_INTERNAL;
   }

   SetExceededQuotaRespMsg respMsg(errorCode);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return retVal;
}
