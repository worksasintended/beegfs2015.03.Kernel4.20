#ifndef MODEGETQUOTAINFO_H_
#define MODEGETQUOTAINFO_H_


#include <common/storage/quota/GetQuotaInfo.h>
#include <common/storage/quota/QuotaData.h>

#include "Mode.h"



class ModeGetQuotaInfo : public Mode, GetQuotaInfo
{
   public:
      ModeGetQuotaInfo() : Mode(), GetQuotaInfo()
      {
      }

      virtual int execute();
      static void printHelp();

   protected:

   private:
      int checkConfig(StringMap* cfg);
      void printQuota(QuotaDataMap* usedQuota, QuotaDataMap* quotaLimits);
      bool printQuotaForID(QuotaDataMap* usedQuota, QuotaDataMap* quotaLimits, unsigned id);
      bool printQuotaForRange(QuotaDataMap* usedQuota, QuotaDataMap* quotaLimits);
      bool printQuotaForList(QuotaDataMap* usedQuota, QuotaDataMap* quotaLimits);
};

#endif /* MODEGETQUOTAINFO_H_ */
