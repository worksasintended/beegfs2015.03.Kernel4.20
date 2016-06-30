#ifndef COMPONENTS_QUOTA_QUOTADEFAULTLIMITS_H_
#define COMPONENTS_QUOTA_QUOTADEFAULTLIMITS_H_


#include <stdlib.h>



class QuotaDefaultLimits
{
   public:
      QuotaDefaultLimits() : defaultUserQuotaInodes(0), defaultUserQuotaSize(0),
         defaultGroupQuotaInodes(0), defaultGroupQuotaSize(0) {};
      QuotaDefaultLimits(size_t userInodesLimit, size_t userSizeLimit, size_t groupInodesLimit,
         size_t groupSizeLimit) : defaultUserQuotaInodes(userInodesLimit),
         defaultUserQuotaSize(userSizeLimit), defaultGroupQuotaInodes(groupInodesLimit),
         defaultGroupQuotaSize(groupSizeLimit) {};
      virtual ~QuotaDefaultLimits() {};


   private:
      uint64_t defaultUserQuotaInodes;
      uint64_t defaultUserQuotaSize;
      uint64_t defaultGroupQuotaInodes;
      uint64_t defaultGroupQuotaSize;


   public:
      unsigned serialLen();
      size_t serialize(char* buf);
      bool deserialize(const char* buf, size_t bufLen, unsigned* outLen);

      uint64_t getDefaultUserQuotaInodes()
      {
         return defaultUserQuotaInodes;
      }

      uint64_t getDefaultUserQuotaSize()
      {
         return defaultUserQuotaSize;
      }

      uint64_t getDefaultGroupQuotaInodes()
      {
         return defaultGroupQuotaInodes;
      }

      uint64_t getDefaultGroupQuotaSize()
      {
         return defaultGroupQuotaSize;
      }

      void updateUserLimits(uint64_t inodeLimit, uint64_t sizeLimit)
      {
         defaultUserQuotaInodes = inodeLimit;
         defaultUserQuotaSize = sizeLimit;
      }

      void updateGroupLimits(uint64_t inodeLimit, uint64_t sizeLimit)
      {
         defaultGroupQuotaInodes = inodeLimit;
         defaultGroupQuotaSize = sizeLimit;
      }

      void updateLimits(QuotaDefaultLimits& limits)
      {
         defaultUserQuotaInodes = limits.getDefaultUserQuotaInodes();
         defaultUserQuotaSize = limits.getDefaultUserQuotaSize();
         defaultGroupQuotaInodes = limits.getDefaultGroupQuotaInodes();
         defaultGroupQuotaSize = limits.getDefaultGroupQuotaSize();
      }
};

#endif /* COMPONENTS_QUOTA_QUOTADEFAULTLIMITS_H_ */
