#ifndef QUOTADATAREQUESTOR_H_
#define QUOTADATAREQUESTOR_H_


#include <common/Common.h>
#include <common/storage/quota/GetQuotaInfo.h>



class QuotaDataRequestor;                                               //forward declaration

typedef std::list<QuotaDataRequestor> QuotaDataRequestorList;
typedef QuotaDataRequestorList::iterator QuotaDataRequestorListIter;
typedef QuotaDataRequestorList::const_iterator QuotaDataRequestorListConstIter;


/**
 * collects the quota data from the storage servers
 */
class QuotaDataRequestor : public GetQuotaInfo
{
   public:
      QuotaDataRequestor(QuotaDataType type) : GetQuotaInfo()
      {
         this->cfg.cfgTargetSelection = GETQUOTACONFIG_ALL_TARGETS_ONE_REQUEST_PER_TARGET;
         this->cfg.cfgType = type;
         this->cfg.cfgUseAll = true;
      }

      QuotaDataRequestor(QuotaDataType type, unsigned id)
      : GetQuotaInfo()
      {
         this->cfg.cfgTargetSelection = GETQUOTACONFIG_ALL_TARGETS_ONE_REQUEST_PER_TARGET;
         this->cfg.cfgType = type;
         this->cfg.cfgID = id;
      }

      QuotaDataRequestor(QuotaDataType type, UIntList idList)
      : GetQuotaInfo()
      {
         this->cfg.cfgTargetSelection = GETQUOTACONFIG_ALL_TARGETS_ONE_REQUEST_PER_TARGET;
         this->cfg.cfgType = type;
         this->cfg.cfgUseList = true;
         this->cfg.cfgIDList = idList;
      }

      QuotaDataRequestor(QuotaDataType type, unsigned rangeStart, unsigned rangeEnd)
      : GetQuotaInfo()
      {
         this->cfg.cfgTargetSelection = GETQUOTACONFIG_ALL_TARGETS_ONE_REQUEST_PER_TARGET;
         this->cfg.cfgType = type;
         this->cfg.cfgUseRange = true;
         this->cfg.cfgIDRangeStart = rangeStart;
         this->cfg.cfgIDRangeEnd = rangeEnd;
      }

      bool requestQuota(QuotaDataMapForTarget* outQuotaData, RWLock* outQuotaResultsRWLock,
         bool* isStoreDirty);

      void setTargetNumID(uint16_t targetNumID)
      {
         this->cfg.cfgTargetSelection = GETQUOTACONFIG_SINGLE_TARGET;
         this->cfg.cfgTargetNumID = targetNumID;
      }

   private:
      void updateQuotaDataWithResponse(QuotaDataMapForTarget* inQuotaData,
         QuotaDataMapForTarget* outQuotaData, TargetMapper* mapper);
};

#endif /* QUOTADATAREQUESTOR_H_ */
