#ifndef QUOTASTORELIMITS_H_
#define QUOTASTORELIMITS_H_

#include <common/threading/RWLock.h>
#include <common/threading/SafeRWLock.h>
#include <common/storage/quota/QuotaData.h>
#include <common/Common.h>



class QuotaStoreLimits
{
   public:
      QuotaStoreLimits(QuotaDataType type, std::string logContext, std::string storePath) :
         quotaType(type), logContext(logContext), storePath(storePath), storeDirty(false)
      {
      }

      bool loadFromFile();
      bool saveToFile();

      bool addOrUpdateLimit(QuotaData quotaData);
      bool addOrUpdateLimits(QuotaDataList* quotaData);

      bool getQuotaLimit(QuotaData& quotaDataInOut);

      void getAllQuotaLimits(QuotaDataMap* outMap);


   protected:


   private:
      QuotaDataType quotaType; // type of allowed QuotaData for the map, used only for debug

      std::string logContext; // the LogContext string, required for multiple use in the same daemon

      RWLock limitsRWLock; // syncs access to the limits

      std::string storePath; // not thread-safe!
      bool storeDirty; // true if saved limits file needs to be updated

      QuotaDataMap limits; // the individual quota limits


      bool addOrUpdateLimitUnlocked(QuotaData quotaData);


   public:
      // getters & setters

      std::string getStorePath()
      {
         return this->storePath;
      }

      bool isStoreDirty()
      {
         SafeRWLock rwLock(&this->limitsRWLock, SafeRWLock_READ); // R E A D L O C K
         bool retVal = this->storeDirty;
         rwLock.unlock(); // U N L O C K

         return retVal;
      }

      /**
       * Get number of stored quota limits.
       */
      size_t getSize()
      {
         SafeRWLock rwLock(&this->limitsRWLock, SafeRWLock_READ); // R E A D L O C K

         size_t size = this->limits.size();

         rwLock.unlock(); // U N L O C K

         return size;
      }
};

#endif /* QUOTASTORELIMITS_H_ */
