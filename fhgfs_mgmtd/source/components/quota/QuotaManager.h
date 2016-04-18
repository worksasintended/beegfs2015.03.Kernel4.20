#ifndef QUOTAMANAGER_H_
#define QUOTAMANAGER_H_

#include <common/app/log/LogContext.h>
#include <common/storage/quota/GetQuotaInfo.h>
#include <common/storage/quota/ExceededQuotaStore.h>
#include <common/threading/RWLock.h>
#include <common/Common.h>
#include <components/quota/QuotaDataRequestor.h>
#include <components/quota/QuotaStoreLimits.h>


/**
 * the quota manager which coordinates all the quota enforcement communication, it starts the
 * slave processes to get the quota data, calculate the exceeded quota IDs and push the exceeded IDs
 * to the servers
 */
class QuotaManager : public PThread
{
   public:
      QuotaManager();
      ~QuotaManager();

      void run();
      bool updateQuotaLimits(QuotaDataList* list);
      void removeTargetFromQuotaDataStores(uint16_t targetNumID);
      bool getQuotaLimitsForID(QuotaData& inOutQuotaData);
      std::string usedQuotaUserToString(bool mergeTargets);
      std::string usedQuotaGroupToString(bool mergeTargets);

   protected:


   private:
      LogContext log;

      RWLock usedQuotaUserRWLock; // syncs access to the usedQuotaUser
      RWLock usedQuotaGroupRWLock; // syncs access to the usedQuotaGroup

      QuotaDataMapForTarget *usedQuotaUser;
      QuotaDataMapForTarget *usedQuotaGroup;

      bool usedQuotaUserStoreDirty;
      bool usedQuotaGroupStoreDirty;

      QuotaStoreLimits* quotaUserLimits;
      QuotaStoreLimits* quotaGroupLimits;

      ExceededQuotaStore* exceededQuotaStore;

      void requestLoop();

      bool updateUsedQuota();
      bool collectQuotaDataFromServer();
      void calculateExceededQuota();
      bool isQuotaExceededForIDunlocked(unsigned id, QuotaLimitType limitType,
         QuotaDataMapForTarget* usedQuota, QuotaDataMap* limits);
      bool pushExceededQuotaIDs();
      int getMaxMessageCountForPushExceededQuota(UIntList* listToSend);

      bool saveQuotaData();
      bool loadQuotaData();

   public:
      // getters & setters

      ExceededQuotaStore* getExceededQuotaStore()
      {
         return this->exceededQuotaStore;
      }
};

#endif /* QUOTAMANAGER_H_ */
