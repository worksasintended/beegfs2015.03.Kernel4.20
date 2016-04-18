#ifndef MSGHELPERTRUNC_H_
#define MSGHELPERTRUNC_H_

#include <common/Common.h>
#include <storage/MetaStore.h>


class MsgHelperTrunc
{
   public:
      static FhgfsOpsErr truncFile(EntryInfo* entryInfo, int64_t filesize,
         bool useQuota, unsigned msgUserID);
      static FhgfsOpsErr truncChunkFile(FileInode* file, EntryInfo* entryInfo, int64_t filesize,
         bool useQuota, unsigned msgUserID);

      static bool isTruncChunkRequired(EntryInfo* entryInfo, int64_t filesize);
      
   private:
      MsgHelperTrunc() {}
      
      static FhgfsOpsErr truncChunkFileSequential(FileInode* file, EntryInfo* entryInfo,
         int64_t filesize, bool useQuota, unsigned msgUserID);
      static FhgfsOpsErr truncChunkFileParallel(FileInode* file, EntryInfo* entryInfo,
         int64_t filesize, bool useQuota, unsigned msgUserID);
      static int64_t getNodeLocalOffset(int64_t pos, int64_t chunkSize, size_t numNodes,
         size_t stripeNodeIndex);
      static int64_t getNodeLocalTruncPos(int64_t pos, StripePattern& pattern,
         size_t stripeNodeIndex);
      
      
   public:
      // inliners
};

#endif /* MSGHELPERTRUNC_H_ */
