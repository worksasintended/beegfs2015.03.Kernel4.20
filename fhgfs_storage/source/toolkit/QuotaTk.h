#ifndef TOOLKIT_QUOTATK_H_
#define TOOLKIT_QUOTATK_H_


#include <common/storage/quota/QuotaBlockDevice.h>
#include <common/storage/quota/QuotaData.h>
#include <common/Common.h>


class QuotaTk
{
   public:
      static bool requestQuotaForID(unsigned id, QuotaDataType type, QuotaBlockDeviceMap* blockDevices,
         QuotaDataList* outQuotaDataList);
      static bool requestQuotaForRange(QuotaBlockDeviceMap* blockDevices, unsigned rangeStart,
         unsigned rangeEnd, QuotaDataType type, QuotaDataList* outQuotaDataList);
      static bool requestQuotaForList(QuotaBlockDeviceMap* blockDevices, UIntList* idList,
         QuotaDataType type, QuotaDataList* outQuotaDataList);

      static bool checkQuota(QuotaBlockDeviceMap* blockDevices, QuotaData* outData);

   private:
      QuotaTk() {};

};

#endif /* TOOLKIT_QUOTATK_H_ */
