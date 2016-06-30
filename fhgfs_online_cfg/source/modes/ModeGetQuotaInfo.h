#ifndef MODEGETQUOTAINFO_H_
#define MODEGETQUOTAINFO_H_


#include <common/storage/quota/GetQuotaInfo.h>
#include <common/storage/quota/QuotaData.h>
#include <common/storage/quota/QuotaDefaultLimits.h>

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

      bool requestDefaultLimits(Node* mgmtNode, QuotaDefaultLimits& defaultLimits);

      void printQuota(QuotaDataMap* usedQuota, QuotaDataMap* quotaLimits,
         QuotaDefaultLimits& defaultLimits);
      bool printQuotaForID(QuotaDataMap* usedQuota, QuotaDataMap* quotaLimits, unsigned id,
         std::string& defaultSizeLimitValue, std::string& defaultSizeLimitUnit,
         uint64_t defaultInodeLimit);
      bool printQuotaForRange(QuotaDataMap* usedQuota, QuotaDataMap* quotaLimits,
         std::string& defaultSizeLimitValue, std::string& defaultSizeLimitUnit,
         uint64_t defaultInodeLimit);
      bool printQuotaForList(QuotaDataMap* usedQuota, QuotaDataMap* quotaLimits,
         std::string& defaultSizeLimitValue, std::string& defaultSizeLimitUnit,
         uint64_t defaultInodeLimit);
      void printDefaultQuotaLimits(QuotaDefaultLimits& quotaLimits);
};

#endif /* MODEGETQUOTAINFO_H_ */
