#ifndef MODESTORAGERESYNCSTATS_H_
#define MODESTORAGERESYNCSTATS_H_

#include <common/net/message/storage/mirroring/GetStorageResyncStatsMsg.h>
#include <common/net/message/storage/mirroring/GetStorageResyncStatsRespMsg.h>
#include <common/storage/StorageErrors.h>
#include <common/Common.h>
#include <modes/Mode.h>

class ModeStorageResyncStats : public Mode
{
   public:
      ModeStorageResyncStats()
      {
      }

      virtual int execute();

      static void printHelp();

   private:
      int getStats(uint16_t syncToTargetID, bool isBuddyGroupID);
      void printStats(BuddyResyncJobStats& jobStats);
      std::string jobStatusToStr(BuddyResyncJobStatus status);
};


#endif /* MODESTORAGERESYNCSTATS_H_ */
