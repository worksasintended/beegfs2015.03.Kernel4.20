/*
 * Dentry information stored on disk
 */

#ifndef DENTRYSTOREDATA_H_
#define DENTRYSTOREDATA_H_


#include <common/storage/StorageDefinitions.h>

/* Note: Don't forget to update DiskMetaData::getSupportedDentryFeatureFlags() if you add new
 *       flags here. */

// Feature flags, 16 bit
#define DENTRY_FEATURE_INODE_INLINE        1 // inode inlined into a dentry
#define DENTRY_FEATURE_IS_FILEINODE        2 // file-inode
#define DENTRY_FEATURE_MIRRORED            4 // feature flag to indicate mirrored inodes


class DentryStoreData
{
   friend class DirEntry;
   friend class DiskMetaData;
   friend class FileInode;

   public:

      DentryStoreData()
      {
         this->entryType          = DirEntryType_INVALID;
         this->ownerNodeID        = 0,
         this->dentryFeatureFlags = 0;

      }

      DentryStoreData(std::string entryID, DirEntryType entryType, uint16_t ownerNodeID,
         unsigned dentryFeatureFlags) : entryID(entryID)
      {
         this->entryType          = entryType;
         this->ownerNodeID        = ownerNodeID;
         this->dentryFeatureFlags = (uint16_t) dentryFeatureFlags;
      }

      std::string entryID;    // a filesystem-wide identifier for this dir
      DirEntryType entryType;
      uint16_t ownerNodeID;   // 0 means undefined
      uint16_t dentryFeatureFlags;
      uint16_t mirrorNodeID; /* Mirror node of the file/dir inode (depends on feature flag)
                              * Dentry mirroring is done automatically based on the parent */

   protected:

      // getters / setters

      void setEntryID(std::string entryID)
      {
         this->entryID = entryID;
      }

      std::string getEntryID()
      {
         return this->entryID;
      }

      void setDirEntryType(DirEntryType entryType)
      {
         this->entryType = entryType;
      }

      DirEntryType getDirEntryType() const
      {
         return this->entryType;
      }

      void setOwnerNodeID(uint16_t ownerNodeID)
      {
         this->ownerNodeID = ownerNodeID;
      }

      uint16_t getOwnerNodeID() const
      {
         return this->ownerNodeID;
      }

      void setMirrorNodeID(uint16_t mirrorNodeID)
      {
         this->dentryFeatureFlags |= DENTRY_FEATURE_MIRRORED;
         this->mirrorNodeID = mirrorNodeID;
      }

      uint16_t getMirrorNodeID() const
      {
         return this->mirrorNodeID;
      }

      void setDentryFeatureFlags(uint16_t dentryFeatureFlags)
      {
         this->dentryFeatureFlags = dentryFeatureFlags;
      }

      uint16_t getDentryFeatureFlags() const
      {
         return this->dentryFeatureFlags;
      }

};


#endif /* DENTRYSTOREDATA_H_ */
