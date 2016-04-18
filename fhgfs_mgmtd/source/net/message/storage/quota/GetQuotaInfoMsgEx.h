#ifndef GETQUOTAINFOMSGEX_H_
#define GETQUOTAINFOMSGEX_H_


#include <common/net/message/storage/quota/GetQuotaInfoMsg.h>
#include <common/storage/quota/QuotaBlockDevice.h>
#include <common/storage/quota/QuotaData.h>
#include <common/toolkit/SynchronizedCounter.h>
#include <common/Common.h>
#include <components/quota/QuotaManager.h>



class GetQuotaInfoMsgEx : public GetQuotaInfoMsg
{
   public:
      GetQuotaInfoMsgEx() : GetQuotaInfoMsg()
      {
      }

      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);

   protected:

   private:
      bool requestQuotaForID(unsigned id, QuotaDataType type, QuotaManager* quotaManager,
         QuotaDataList* outQuotaDataList);
      bool requestQuotaForRange(QuotaManager* quotaManager, QuotaDataList* outQuotaDataList);
      bool requestQuotaForList(QuotaManager* quotaManager, QuotaDataList* outQuotaDataList);

      bool checkQuota(QuotaBlockDeviceMap* blockDevices, QuotaData* outData);

};

#endif /* GETQUOTAINFOMSGEX_H_ */
