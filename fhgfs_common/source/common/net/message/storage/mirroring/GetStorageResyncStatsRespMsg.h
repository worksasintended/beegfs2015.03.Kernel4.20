#ifndef GETSTORAGERESYNCSTATSRESPMSG_H_
#define GETSTORAGERESYNCSTATSRESPMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/mirroring/BuddyResyncJobStats.h>

class GetStorageResyncStatsRespMsg : public NetMessage
{
   public:
      /*
       * @param jobStats not owned by this object!
       */
      GetStorageResyncStatsRespMsg(BuddyResyncJobStats* jobStats) :
         NetMessage(NETMSGTYPE_GetStorageResyncStatsResp)
      {
         this->jobStatsPtr = jobStats;
      }

      /**
       * For deserialization only!
       */
      GetStorageResyncStatsRespMsg() : NetMessage(NETMSGTYPE_GetStorageResyncStatsResp)
      {}

   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         unsigned retVal = NETMSG_HEADER_LENGTH +
            jobStatsPtr->serialLen(); // jobStatsPtr

         return retVal;
      }

   private:
      // for serialization
      BuddyResyncJobStats* jobStatsPtr;

      // for deserialization
      BuddyResyncJobStats jobStats;

   public:
      // getters & setters
      BuddyResyncJobStats* getJobStats()
      {
         return &jobStats;
      }

      void getJobStats(BuddyResyncJobStats& outStats)
      {
         outStats.update(jobStats);
      }
};

#endif /*GETSTORAGERESYNCSTATSRESPMSG_H_*/
