#include <app/App.h>
#include <common/app/log/LogContext.h>
#include <common/net/message/storage/quota/GetQuotaInfoRespMsg.h>
#include <common/storage/quota/GetQuotaConfig.h>
#include <session/ZfsSession.h>
#include <storage/QuotaBlockDevice.h>
#include <program/Program.h>
#include <toolkit/QuotaTk.h>
#include "GetQuotaInfoMsgEx.h"



bool GetQuotaInfoMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock, char* respBuf,
   size_t bufLen, HighResolutionStats* stats)
{
   const char* logContext = "GetQuotaInfo (GetQuotaInfoMsg incoming)";

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG(logContext, Log_DEBUG, std::string("Received a GetQuotaInfoMsg from: ")
      + peer);

   App *app = Program::getApp();

   bool retVal = false;
   QuotaBlockDeviceMap quotaBlockDevices;
   QuotaDataList outQuotaDataList;
   ZfsSession session;

   switch(getTargetSelection() )
   {
      case GETQUOTACONFIG_ALL_TARGETS_ONE_REQUEST:
         app->getStorageTargets()->getQuotaBlockDevices(&quotaBlockDevices);
         break;
      case GETQUOTACONFIG_ALL_TARGETS_ONE_REQUEST_PER_TARGET:
         app->getStorageTargets()->getQuotaBlockDevice(&quotaBlockDevices, getTargetNumID() );
         break;
      case GETQUOTACONFIG_SINGLE_TARGET:
         app->getStorageTargets()->getQuotaBlockDevice(&quotaBlockDevices, getTargetNumID() );
         break;
   }

   if(quotaBlockDevices.empty() )
   {
      /* no quota data available but do not return an error during message processing, it's not
      the correct place for error handling in this case */
      LogContext(logContext).logErr("Error: no quota block devices.");
      retVal = true;
   }
   else
   {
      if(getQueryType() == GetQuotaInfo_QUERY_TYPE_SINGLE_ID)
         retVal = QuotaTk::requestQuotaForID(getIDRangeStart(), getType(), &quotaBlockDevices,
            &outQuotaDataList, &session);
      else
      if(getQueryType() == GetQuotaInfo_QUERY_TYPE_ID_RANGE)
         retVal = QuotaTk::requestQuotaForRange(&quotaBlockDevices, getIDRangeStart(),
            getIDRangeEnd(), getType(), &outQuotaDataList, &session);
      else
      if(getQueryType() == GetQuotaInfo_QUERY_TYPE_ID_LIST)
      {
         UIntList idList;
         parseIDList(&idList);
         retVal = QuotaTk::requestQuotaForList(&quotaBlockDevices, &idList, getType(),
            &outQuotaDataList, &session);
      }
   }


   // send response
   GetQuotaInfoRespMsg respMsg(&outQuotaDataList);
   respMsg.serialize(respBuf, bufLen);

   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return retVal;
}
