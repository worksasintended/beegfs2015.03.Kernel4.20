#ifndef MSGHELPERSTAT_H_
#define MSGHELPERSTAT_H_

#include <common/Common.h>
#include <storage/MetaStore.h>
#include <storage/MetadataEx.h>


class MsgHelperStat
{
   public:
      static FhgfsOpsErr stat(EntryInfo* entryInfo, bool loadFromDisk, unsigned msgUserID,
         StatData& outStatData, uint16_t* outParentNodeId = NULL,
         std::string* outParentEntryID = NULL);
      static FhgfsOpsErr refreshDynAttribs(EntryInfo* entryInfo, bool makePersistent,
         unsigned msgUserID);


   private:
      MsgHelperStat() {}

      static FhgfsOpsErr refreshDynAttribsSequential(FileInode* file, std::string entryID,
         unsigned msgUserID);
      static FhgfsOpsErr refreshDynAttribsParallel(FileInode* file, std::string entryID,
         unsigned msgUserID);


   public:
      // inliners
};

#endif /* MSGHELPERSTAT_H_ */
