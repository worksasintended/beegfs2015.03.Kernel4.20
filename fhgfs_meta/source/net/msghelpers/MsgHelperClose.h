#ifndef MSGHELPERCLOSE_H_
#define MSGHELPERCLOSE_H_

#include <common/Common.h>
#include <storage/MetaStore.h>


class MsgHelperClose
{
   public:
      static FhgfsOpsErr closeFile(std::string sessionID, std::string fileHandleID,
         EntryInfo* entryInfo, int maxUsedNodeIndex, unsigned msgUserID,
         bool* outUnlinkDisposalFile);
      static FhgfsOpsErr closeSessionFile(std::string sessionID, std::string fileHandleID,
         EntryInfo* entryInfo, unsigned* outAccessFlags, FileInode** outCloseFile);
      static FhgfsOpsErr closeChunkFile(std::string sessionID, std::string fileHandleID,
         int maxUsedNodeIndex, FileInode* file, EntryInfo *entryInfo, unsigned msgUserID);
      static FhgfsOpsErr unlinkDisposableFile(std::string fileID, unsigned msgUserID);


   private:
      MsgHelperClose() {}

      static FhgfsOpsErr closeChunkFileSequential(std::string sessionID, std::string fileHandleID,
         int maxUsedNodeIndex, FileInode* file, EntryInfo *entryInfo, unsigned msgUserID);
      static FhgfsOpsErr closeChunkFileParallel(std::string sessionID, std::string fileHandleID,
         int maxUsedNodeIndex, FileInode* file, EntryInfo *entryInfo, unsigned msgUserID);


   public:
      // inliners
};

#endif /* MSGHELPERCLOSE_H_ */
