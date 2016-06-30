/*
 * Data of a FileInode stored on disk.
 */

#ifndef FILEINODESTOREDATA
#define FILEINODESTOREDATA

#include <common/storage/striping/StripePattern.h>
#include <common/storage/HsmFileMetaData.h>
#include <common/storage/Metadata.h>
#include <common/storage/StatData.h>

/* Note: Don't forget to update DiskMetaData::getSupportedFileInodeFeatureFlags() if you add new
 *       flags here. */

#define FILEINODE_FEATURE_MIRRORED            1 // indicate mirrored inodes
#define FILEINODE_FEATURE_HSM                 4

// NOTE: 8 is currently unused and was never used (set) at all in a release, so this be used next!

// note: original parent-id and uid are required for the chunk-path calculation
#define FILEINODE_FEATURE_HAS_ORIG_PARENTID  16 // parent-id was updated
#define FILEINODE_FEATURE_HAS_ORIG_UID       32 // uid was updated
#define FILEINODE_FEATURE_HAS_STATFLAGS      64 // stat-data have their own flags

enum FileInodeOrigFeature
{
   FileInodeOrigFeature_UNSET = -1,
   FileInodeOrigFeature_TRUE,
   FileInodeOrigFeature_FALSE
};

/* inode data inlined into a direntry, such as in DIRENTRY_STORAGE_FORMAT_VER3 */
class FileInodeStoreData
{
      // TODO: Can we reduce the number of friend classes?

   friend class FileInode;
   friend class DirEntry;
   friend class DirEntryStore;
   friend class GenericDebugMsgEx;
   friend class LookupIntentMsgEx; // just to avoid to copy two time statData
   friend class RecreateDentriesMsgEx;
   friend class RetrieveDirEntriesMsgEx;
   friend class MetaStore;
   friend class DiskMetaData;

   friend class AdjustChunkPermissionsMsgEx;

   friend class TestSerialization; // for testing

   public:

      FileInodeStoreData()
      {
         this->stripePattern              = NULL;
         this->inodeFeatureFlags          = 0;
         this->mirrorNodeID               = 0;
         this->origFeature                = FileInodeOrigFeature_UNSET;
      }

      FileInodeStoreData(std::string& entryID, StatData* statData, StripePattern* stripePattern,
         unsigned featureFlags, unsigned origParentUID, std::string& origParentEntryID,
         FileInodeOrigFeature origFeature) :
         entryID(entryID), origParentEntryID(origParentEntryID)
      {
         this->stripePattern              = stripePattern->clone();
         //this->entryID                  = entryID; // set in initializer list
         this->inodeStatData              = *statData;
         this->inodeFeatureFlags          = featureFlags;
         this->origParentUID              = origParentUID;
         this->mirrorNodeID               = 0;
         this->origFeature                = origFeature;

         if ((statData->getUserID() != origParentUID) &&
             (origFeature == FileInodeOrigFeature_TRUE) )
            this->inodeFeatureFlags |= FILEINODE_FEATURE_HAS_ORIG_UID;
      }

      unsigned serialize(char* buf) const;
      bool deserialize(const char* buf, size_t bufLen, unsigned* outLen);
      unsigned serialLen() const;

      friend bool fileInodeStoreDataEquals(const FileInodeStoreData& first,
         const FileInodeStoreData& second);

      /**
       * Used to set the values from those read from disk
       */
      FileInodeStoreData(std::string entryID, FileInodeStoreData* diskData) :
         entryID(entryID)
      {
         // this->entryID        = entryID; // set in initializer
         this->stripePattern = NULL;

         setFileInodeStoreData(diskData);
      }

      ~FileInodeStoreData()
      {
         SAFE_DELETE_NOSET(this->stripePattern);
      }

   private:
      unsigned inodeFeatureFlags; // feature flags for the inode itself, e.g. for mirroring

      StatData inodeStatData;
      std::string entryID; // filesystem-wide unique string
      StripePattern* stripePattern;

      FileInodeOrigFeature origFeature; // indirectly determined via dentry-version
      unsigned origParentUID;
      std::string origParentEntryID;

      uint16_t mirrorNodeID; // mirror node (depends on feature flag)

      HsmFileMetaData hsmFileMetaData; // for HSM integration

      void getPathInfo(PathInfo* outPathInfo);


   public:

      StatData* getInodeStatData()
      {
         return &this->inodeStatData;
      }

      std::string getEntryID()
      {
         return this->entryID;
      }

   protected:

      /**
       * Set all fileInodeStoreData
       *
       * Note: Might update existing values and if these are allocated, such as stripePattern,
       *       these need to be freed first.
       */
      void setFileInodeStoreData(FileInodeStoreData* diskData)
      {
         SAFE_DELETE_NOSET(this->stripePattern);
         this->stripePattern     = diskData->getStripePattern()->clone();

         this->inodeStatData     = *(diskData->getInodeStatData() );
         this->inodeFeatureFlags = diskData->getInodeFeatureFlags();
         this->mirrorNodeID      = diskData->getMirrorNodeID();
         this->origFeature       = diskData->origFeature;
         this->origParentUID           = diskData->origParentUID;
         this->origParentEntryID = diskData->origParentEntryID;

         #ifdef BEEGFS_HSM_DEPRECATED
            HsmFileMetaData hsmFileMetaData = *(diskData->getHsmFileMetaData() );

            this->setHsmFileMetaData(hsmFileMetaData);
         #endif

      }

      void setInodeFeatureFlags(unsigned flags)
      {
         this->inodeFeatureFlags = flags;
      }

      void addInodeFeatureFlag(unsigned flag)
      {
         this->inodeFeatureFlags |= flag;
      }

      void removeInodeFeatureFlag(unsigned flag)
      {
         this->inodeFeatureFlags &= ~flag;
      }

      void setInodeStatData(StatData& statData)
      {
         this->inodeStatData = statData;
      }

      void setEntryID(std::string entryID)
      {
         this->entryID = entryID;
      }

      void setPattern(StripePattern* pattern)
      {
         this->stripePattern = pattern;
      }

      void setMirrorNodeID(uint16_t mirrorNodeID)
      {
         this->mirrorNodeID = mirrorNodeID;
      }

      void setOrigUID(unsigned origParentUID)
      {
         this->origParentUID = origParentUID;
      }

      /**
       * Set the origParentEntryID based on the parentDir. Feature flag will not be updated.
       * This is for inodes which are not de-inlined and not renamed between dirs.
       */
      void setDynamicOrigParentEntryID(std::string origParentEntryID)
      {
         /* Never overwrite existing data! Callers do not check if they need to set it only we do
          * that here. */
         if (!(this->inodeFeatureFlags & FILEINODE_FEATURE_HAS_ORIG_PARENTID) )
            this->origParentEntryID = origParentEntryID;
      }

      /**
       * Set the origParentEntryID from disk, no feature flag test and
       * feature flag will not be updated.
       * Note: Use this for disk-deserialization.
       */
      void setDiskOrigParentEntryID(std::string origParentEntryID)
      {
         this->origParentEntryID = origParentEntryID;
      }

      /**
       * Set the origParentEntryID. Feature flag will be updated, origInformation will be stored
       * to disk.
       * Note: Use this on file renames between dirs and de-inlining.
       */
      void setPersistentOrigParentEntryID(std::string origParentEntryID)
      {
         /* Never overwrite existing data! Callers do not check if they need to set it only we do
          * that here. */
         if ( (!(this->inodeFeatureFlags & FILEINODE_FEATURE_HAS_ORIG_PARENTID) ) &&
              (this->getOrigFeature() == FileInodeOrigFeature_TRUE) )
         {
            this->origParentEntryID = origParentEntryID;
            addInodeFeatureFlag(FILEINODE_FEATURE_HAS_ORIG_PARENTID);
         }
      }


      unsigned getInodeFeatureFlags() const
      {
         return this->inodeFeatureFlags;
      }

      StripePattern* getPattern()
      {
         return this->stripePattern;
      }

      StripePattern* getStripePattern()
      {
         return this->stripePattern;
      }


      void setOrigFeature(FileInodeOrigFeature value)
      {
         this->origFeature = value;
      }

      FileInodeOrigFeature getOrigFeature() const
      {
         return this->origFeature;
      }

      unsigned getOrigUID() const
      {
         return this->origParentUID;
      }

      std::string getOrigParentEntryID() const
      {
         return this->origParentEntryID;
      }

      void incDecNumHardlinks(int value)
      {
         this->inodeStatData.incDecNumHardLinks(value);
      }

      void setNumHardlinks(unsigned numHardlinks)
      {
         this->inodeStatData.setNumHardLinks(numHardlinks);
      }

      unsigned getNumHardlinks() const
      {
         return this->inodeStatData.getNumHardlinks();
      }

      uint16_t getMirrorNodeID() const
      {
         return this->mirrorNodeID;
      }

      /**
       * Return the pattern and set the internal pattern to NULL to make sure it does not get
       * deleted on object destruction.
       */
      StripePattern* getStripePatternAndSetToNull()
      {
         StripePattern* pattern = this->stripePattern;
         this->stripePattern = NULL;

         return pattern;
      }

      // for HSM integration
      void setHsmFileMetaData(HsmFileMetaData& hsmFileMetaData)
      {
         this->hsmFileMetaData = hsmFileMetaData;

         // (un-)set feature flag
         if (this->hsmFileMetaData.hasDefaultValues())
            this->removeInodeFeatureFlag(FILEINODE_FEATURE_HSM);
         else
            this->addInodeFeatureFlag(FILEINODE_FEATURE_HSM);
      }

      void setHsmOfflineChunkCount(uint16_t offlineChunkCount)
      {
         this->hsmFileMetaData.setOfflineChunkCount(offlineChunkCount);

         // (un-)set feature flag
         if (this->hsmFileMetaData.hasDefaultValues())
            this->removeInodeFeatureFlag(FILEINODE_FEATURE_HSM);
         else
            this->addInodeFeatureFlag(FILEINODE_FEATURE_HSM);
      }

      void setHsmCollocationID(HsmCollocationID hsmCollocationID)
      {
         this->hsmFileMetaData.setCollocationID(hsmCollocationID);

         // (un-)set feature flag
         if (this->hsmFileMetaData.hasDefaultValues())
            this->removeInodeFeatureFlag(FILEINODE_FEATURE_HSM);
         else
            this->addInodeFeatureFlag(FILEINODE_FEATURE_HSM);
      }

      HsmCollocationID getHsmCollocationID() const
      {
         return this->hsmFileMetaData.getCollocationID();
      }

      HsmFileMetaData* getHsmFileMetaData()
      {
         return &(this->hsmFileMetaData);
      }

      uint16_t getHsmOfflineFileChunkCount()
      {
         return this->hsmFileMetaData.getOfflineChunkCount();
      }

      void incHsmOfflineFileChunkCount()
      {
         this->hsmFileMetaData.incOfflineChunkCount();

         // set feature flag
         this->addInodeFeatureFlag(FILEINODE_FEATURE_HSM);
      }

      void decHsmOfflineFileChunkCount()
      {
         this->hsmFileMetaData.decOfflineChunkCount();

         // (un-)set feature flag
         if (this->hsmFileMetaData.hasDefaultValues())
            this->removeInodeFeatureFlag(FILEINODE_FEATURE_HSM);
         else
            this->addInodeFeatureFlag(FILEINODE_FEATURE_HSM);
      }

      void setAttribChangeTimeSecs(int64_t attribChangeTimeSecs)
      {
         this->inodeStatData.setAttribChangeTimeSecs(attribChangeTimeSecs);
      }

};


#endif /* FILEINODESTOREDATA */
