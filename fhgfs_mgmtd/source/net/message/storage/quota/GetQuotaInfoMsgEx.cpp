#include <app/App.h>
#include <program/Program.h>
#include <common/net/message/storage/quota/GetQuotaInfoRespMsg.h>
#include <common/app/log/LogContext.h>
#include <common/storage/quota/GetQuotaConfig.h>
#include <common/toolkit/UnitTk.h>

#include <xfs/xfs.h>
#include <xfs/xqm.h>
#include <sys/mount.h>

#include "GetQuotaInfoMsgEx.h"


bool GetQuotaInfoMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock, char* respBuf,
   size_t bufLen, HighResolutionStats* stats)
{
   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG("GetQuotaInfoMsg incoming", Log_DEBUG, std::string("Received a GetQuotaInfoMsg from: ")
      + peer);

   App* app = Program::getApp();
   Config* cfg = app->getConfig();

   QuotaDataList outQuotaDataList;

   if(cfg->getQuotaEnableEnforcement() )
   {
      QuotaManager* quotaManager = app->getQuotaManager();

      if(getQueryType() == GetQuotaInfo_QUERY_TYPE_SINGLE_ID)
         requestQuotaForID(this->getIDRangeStart(), this->getType(), quotaManager,
            &outQuotaDataList);
      else
      if(getQueryType() == GetQuotaInfo_QUERY_TYPE_ID_RANGE)
         requestQuotaForRange(quotaManager, &outQuotaDataList);
      else
      if(getQueryType() == GetQuotaInfo_QUERY_TYPE_ID_LIST)
         requestQuotaForList(quotaManager, &outQuotaDataList);
   }

   // send response
   GetQuotaInfoRespMsg respMsg(&outQuotaDataList);
   respMsg.serialize(respBuf, bufLen);

   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   // do not report an error, also when no quota limits available
   return true;
}

/**
 * request the quota data for a single ID
 *
 * @param id the quota data of this ID are needed
 * @param type the type of the ID user or group
 * @param quotaManager the quota manager of the management daemon
 * @param outQuotaDataList the list with the updated quota data
 * @return true if quota data found for the ID
 */
bool GetQuotaInfoMsgEx::requestQuotaForID(unsigned id, QuotaDataType type,
   QuotaManager* quotaManager, QuotaDataList* outQuotaDataList)
{
   QuotaData data(id, type);
   bool retVal = quotaManager->getQuotaLimitsForID(data);

   if(!retVal)
      data.setIsValid(false);

   outQuotaDataList->push_back(data);

   return retVal;
}

/**
 * request the quota data for a range
 *
 * @param quotaManager the quota manager of the management daemon
 * @param outQuotaDataList the list with the updated quota data
 * @return true if quota data found for the ID
 */
bool GetQuotaInfoMsgEx::requestQuotaForRange(QuotaManager* quotaManager,
   QuotaDataList* outQuotaDataList)
{
   bool retVal = true;

   for(unsigned id = this->getIDRangeStart(); id <= this->getIDRangeEnd(); id++)
   {
      bool errorVal = requestQuotaForID(id, (QuotaDataType)this->getType(), quotaManager,
         outQuotaDataList);

      if(!errorVal)
         retVal = false;
   }

   return retVal;
}

/**
 * request the quota data for a list
 *
 * @param quotaManager the quota manager of the management daemon
 * @param outQuotaDataList the list with the updated quota data
 * @return true if quota data found for the ID
 */
bool GetQuotaInfoMsgEx::requestQuotaForList(QuotaManager* quotaManager,
   QuotaDataList* outQuotaDataList)
{
   bool retVal = true;

   UIntList idList;
   this->parseIDList(&idList);

   for(UIntListIter iter = idList.begin(); iter != idList.end(); iter++)
   {
      bool errorVal = requestQuotaForID(*iter, (QuotaDataType)this->getType(), quotaManager,
         outQuotaDataList);

      if(!errorVal)
         retVal = false;
   }

   return retVal;
}
