#ifndef DISKMETADATA_H_
#define DISKMETADATA_H_

#include "FileInodeStoreData.h"

#define DIRENTRY_SERBUF_SIZE     (1024 * 4) /* make sure that this is always smaller or equal to
                                             * META_SERBUF_SIZE */

#define DISKMETADATA_TYPE_BUF_POS     0
#define DIRENTRY_TYPE_BUF_POS         4

// 8-bit
enum DiskMetaDataType
{
   DiskMetaDataType_FILEDENTRY = 1, // may have inlined inodes
   DiskMetaDataType_DIRDENTRY  = 2,
   DiskMetaDataType_FILEINODE  = 3, // currently in dentry-format
   DiskMetaDataType_DIRINODE   = 4,
};


// forward declarations;
class DirInode;
class FileInode;
class DirEntry;
class DentryStoreData;
class FileInodeStoreData;

/**
 * Generic class for on-disk storage.
 *
 * Note: The class is inherited from DirEntry::. But is an object in FileInode:: methods.
 */
class DiskMetaData
{
   public:

      // used for DirEntry derived class
      DiskMetaData(DentryStoreData* dentryData, FileInodeStoreData* inodeData)
      {
         this->dentryDiskData = dentryData;
         this->inodeData   = inodeData;
      };


      unsigned serializeFileInode(char* buf);
      unsigned serializeDentry(char* buf);
      bool deserializeFileInode(const char* buf);
      bool deserializeDentry(const char* buf);
      static DirEntryType deserializeInode(const char* buf, FileInode** outFileInode,
         DirInode** outDirInode);
      static unsigned serializeDirInode(char* buf, DirInode* inode);
      static bool deserializeDirInode(const char* buf, DirInode* outInode);

   protected:
      DentryStoreData* dentryDiskData; // Not owned by this object!
      FileInodeStoreData* inodeData;  // Not owned by this object!

   private:
      unsigned serializeInDentryFormat(char* buf, DiskMetaDataType metaDataType);
      unsigned serializeDentryV3(char* buf);
      unsigned serializeDentryV4(char* buf);
      unsigned serializeDentryV5(char* buf);
      bool deserializeDentryV3(const char* buf, size_t bufLen, unsigned* outLen);
      bool deserializeDentryV4(const char* buf, size_t bufLen, unsigned* outLen);
      bool deserializeDentryV5(const char* buf, size_t bufLen, unsigned* outLen);

      static unsigned getSupportedDentryFeatureFlags();
      static unsigned getSupportedDentryV4FileInodeFeatureFlags();
      static unsigned getSupportedDentryV5FileInodeFeatureFlags();
      static unsigned getSupportedDirInodeFeatureFlags();

      static bool checkFeatureFlagsCompat(unsigned usedFeatureFlags,
         unsigned supportedFeatureFlags);

   // inliners

   public:

   private:



};

#endif // DISKMETADATA_H_
