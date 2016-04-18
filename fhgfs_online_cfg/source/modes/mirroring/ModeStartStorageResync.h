#ifndef MODESTARTSTORAGERESYNC_H_
#define MODESTARTSTORAGERESYNC_H_

#include <common/storage/StorageErrors.h>
#include <common/Common.h>
#include <modes/Mode.h>

class ModeStartStorageResync : public Mode
{
   public:
      ModeStartStorageResync()
      {
      }

      virtual int execute();

      static void printHelp();


   private:
      int requestResync(uint16_t syncToTargetID, bool isBuddyGroupID, int64_t timestamp,
         bool restartResync);
      FhgfsOpsErr setResyncState(uint16_t targetID, uint16_t nodeID);
      FhgfsOpsErr overrideLastBuddyComm(uint16_t targetID, uint16_t nodeID, int64_t timestamp,
         bool restartResync);
};


#endif /* MODESTARTSTORAGERESYNC_H_ */
