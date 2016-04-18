#ifndef DIRINODE_H_
#define DIRINODE_H_

#include <common/fsck/FsckDirInode.h>
#include <database/EntryID.h>

namespace db {

struct DirInode {
   EntryID id;                /* 12 */
   EntryID parentDirID;       /* 24 */

   uint32_t parentNodeID;       /* 28 */
   uint32_t ownerNodeID;        /* 32 */
   uint32_t saveNodeID;         /* 36 */

   uint32_t stripePatternType:16;
   uint32_t readable:1;         /* 40 */

   uint64_t size;               /* 48 */
   uint64_t numHardlinks;       /* 56 */

   typedef EntryID KeyType;

   EntryID pkey() const { return id; }

   operator FsckDirInode() const
   {
      UInt16Vector noTargets;
      return FsckDirInode(id.str(), parentDirID.str(), parentNodeID,
         ownerNodeID, size, numHardlinks, noTargets,
         FsckStripePatternType(stripePatternType), saveNodeID, readable);
   }
};

}

#endif
