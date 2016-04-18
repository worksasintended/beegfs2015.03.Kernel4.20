#include <app/App.h>
#include <app/log/Logger.h>
#include <common/net/message/NetMessage.h>
#include <common/storage/Metadata.h>
#include <common/storage/StorageDefinitions.h>
#include <common/storage/StorageErrors.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/VersionTk.h>
#include <nodes/NodeStoreEx.h>
#include <toolkit/NoAllocBufferStore.h>

#include "MetadataTk.h"


/**
 * Used to initialize struct CreateInfo. This function always should be used, to make sure,
 * values are not forgotten.
 *
 * Note: preferred meta/storage targets are automatically set to app's preferred meta/storage
 *       targets; keep that in mind when you're cleaning up.
 */
void CreateInfo_init(App* app, struct inode* parentDirInode, const char* entryName,
   int mode, fhgfs_bool isExclusiveCreate, CreateInfo* outCreateInfo)
{
   outCreateInfo->userID = FhgfsCommon_getCurrentUserID();

   // groupID and mode logic taken from inode_init_owner()
   if (parentDirInode && (parentDirInode->i_mode & S_ISGID) )
   {
      outCreateInfo->groupID = i_gid_read(parentDirInode);
      if (S_ISDIR(mode))
         mode |= S_ISGID;
   }
   else
      outCreateInfo->groupID = FhgfsCommon_getCurrentGroupID();

   outCreateInfo->entryName = entryName;
   outCreateInfo->mode      = mode;
   outCreateInfo->isExclusiveCreate = isExclusiveCreate;

   outCreateInfo->preferredStorageTargets = App_getPreferredStorageTargets(app);
   outCreateInfo->preferredMetaTargets    = App_getPreferredMetaNodes(app);
}


/**
 * @param outEntryInfo contained values will be kalloced (on success) and need to be kfreed with
 * FhgfsInode_freeEntryMinInfoVals() later.
 */
fhgfs_bool MetadataTk_getRootEntryInfoCopy(App* app, EntryInfo* outEntryInfo)
{
   NodeStoreEx* nodes = App_getMetaNodes(app);

   uint16_t outOwnerNodeID = NodeStoreEx_getRootNodeNumID(nodes);
   const char* outParentEntryID = StringTk_strDup("");
   const char* outEntryID = StringTk_strDup(META_ROOTDIR_ID_STR);
   const char* outDirName = StringTk_strDup("");
   DirEntryType outEntryType = (DirEntryType) DirEntryType_DIRECTORY;
   int outStatFlags = 0;

   /* Even if outOwnerNodeID == 0 we still init outEntryInfo and malloc as FhGFS
    * policy says that kfree(NULL) is not allowed (the kernel allows it). */

   EntryInfo_init(outEntryInfo, outOwnerNodeID, outParentEntryID, outEntryID, outDirName,
      outEntryType, outStatFlags);

   if (!outOwnerNodeID)
      return fhgfs_false;

   return fhgfs_true;
}
