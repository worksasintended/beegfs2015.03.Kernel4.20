#ifndef DIRINODE_H_
#define DIRINODE_H_

#include <common/storage/striping/StripePattern.h>
#include <common/storage/Metadata.h>
#include <common/storage/StorageDefinitions.h>
#include <common/storage/StorageErrors.h>
#include <common/threading/SafeMutexLock.h>
#include <common/storage/StatData.h>
#include <common/Common.h>
#include <components/metadatamirrorer/MetadataMirrorer.h>
#include "DirEntryStore.h"
#include "MetadataEx.h"
#include "InodeFileStore.h"


/* Note: Don't forget to update DiskMetaData::getSupportedDirInodeFeatureFlags() if you add new
 *       flags here. */

#define DIRINODE_FEATURE_HSM           1 // indicate that HSM info is set
#define DIRINODE_FEATURE_EARLY_SUBDIRS 2 // indicate proper alignment for statData
#define DIRINODE_FEATURE_MIRRORED      4 // indicate mirrored directory
#define DIRINODE_FEATURE_STATFLAGS     8 // StatData have a flags field

/**
 * Our inode object, but for directories only. Files are in class FileInode.
 */
class DirInode
{
   friend class MetaStore;
   friend class InodeDirStore;
   friend class DiskMetaData;

   public:
      DirInode(std::string id, int mode, unsigned userID, unsigned groupID,
         uint16_t ownerNodeID, StripePattern& stripePattern);

      /**
       * Constructur used to load inodes from disk
       * Note: Not all values are set, as we load those from disk.
       */
      DirInode(const std::string& id) : id(id), entries(id)
      {
         this->stripePattern = NULL;
         this->exclusive = false;
         this->isLoaded = false;
         this->featureFlags = 0;
         this->mirrorNodeID = 0;
         this->hsmCollocationID = HSM_COLLOCATIONID_DEFAULT;
      }

      ~DirInode()
      {
         LOG_DEBUG("Delete DirInode", Log_SPAM, std::string("Deleting inode: ") + this->id);

         SAFE_DELETE_NOSET(stripePattern);
      }

      
      static DirInode* createFromFile(std::string id);
      
      StripePattern* createFileStripePattern(UInt16List* preferredNodes,
         unsigned numtargets, unsigned chunksize);

      FhgfsOpsErr listIncremental(int64_t serverOffset,
         unsigned maxOutNames, StringList* outNames, int64_t* outNewServerOffset);
      FhgfsOpsErr listIncrementalEx(int64_t serverOffset,
         unsigned maxOutNames, bool filterDots, ListIncExOutArgs& outArgs);
      FhgfsOpsErr listIDFilesIncremental(int64_t serverOffset, uint64_t incrementalOffset,
         unsigned maxOutNames, ListIncExOutArgs& outArgs);

      bool exists(std::string entryName);

      FhgfsOpsErr makeDirEntry(DirEntry* entry);
      FhgfsOpsErr linkFileInodeToDir(std::string& inodePath, std::string &fileName);

      FhgfsOpsErr removeDir(std::string entryName, DirEntry** outDirEntry);
      FhgfsOpsErr unlinkDirEntry(std::string entryName, DirEntry* inEntry,
         unsigned unlinkTypeFlags);

      bool loadIfNotLoaded(void);

      FhgfsOpsErr refreshMetaInfo();

      // non-inlined getters & setters
      FhgfsOpsErr setOwnerNodeID(std::string entryName, uint16_t ownerNode);

      StripePattern* getStripePatternClone();
      bool setStripePattern(StripePattern& newPattern);

      FhgfsOpsErr getStatData(StatData& outStatData,
         uint16_t* outParentNodeID = NULL, std::string* outParentEntryID = NULL);
      void setStatData(StatData& statData);

      bool setAttrData(int validAttribs, SettableFileAttribs* attribs);
      FhgfsOpsErr setDirParentAndChangeTime(EntryInfo* entryInfo, uint16_t parentNodeID);

      FhgfsOpsErr setAndStoreMirrorNodeIDChecked(uint16_t mirrorNodeID);

      FhgfsOpsErr listXAttr(const std::string& fileName, StringVector& outAttrList);
      FhgfsOpsErr getXAttr(const std::string& fileName, const std::string& xAttrName,
            CharVector& outValue, ssize_t& inOutSize);
      FhgfsOpsErr removeXAttr(const std::string& fileName, const std::string& xAttrName);
      FhgfsOpsErr setXAttr(const std::string& fileName, const std::string& xAttrName,
            const CharVector& xAttrValue, int flags);

   private:
      std::string id; // filesystem-wide unique string
      uint16_t ownerNodeID; // 0 means undefined
      StripePattern* stripePattern; // is the default for new files and subdirs
      
      std::string parentDirID; // must be reliable for NFS
      uint16_t parentNodeID;   // must be reliable for NFS

      uint16_t featureFlags;
      uint16_t mirrorNodeID; // node ID of dir mirror if DIRINODE_FEATURE_MIRRORED is set

      bool exclusive; // if set, we do not allow other references

      // StatData
      StatData statData;
      unsigned numSubdirs; // indirectly updated by subdir creation/removal
      unsigned numFiles; // indirectly updated by subfile creation/removal
      
      RWLock rwlock;

      DirEntryStore entries;
      
      /* if not set we have an object that has not read data from disk yet, the dir might not
         even exist on disk */
      bool isLoaded;
      Mutex loadLock; // protects the disk load
      
      InodeFileStore fileStore; /* We must not delete the DirInode as long as this
                                 * InodeFileStore still has entries. Therefore a dir reference
                                 * has to be taken for entry in this InodeFileStore */

      // for HSM integration
      HsmCollocationID hsmCollocationID;
      
      StripePattern* createFileStripePatternUnlocked(UInt16List* preferredNodes,
         unsigned numtargets, unsigned chunksize);

      FhgfsOpsErr storeInitialMetaData();
      FhgfsOpsErr storeInitialMetaData(const CharVector& defaultACLXAttr,
         const CharVector& accessACLXAttr);
      FhgfsOpsErr storeInitialMetaDataInode();
      bool storeUpdatedMetaDataBuf(char* buf, unsigned bufLen);
      bool storeUpdatedMetaDataBufAsXAttr(char* buf, unsigned bufLen);
      bool storeUpdatedMetaDataBufAsContents(char* buf, unsigned bufLen);
      bool storeUpdatedMetaDataBufAsContentsInPlace(char* buf, unsigned bufLen);
      bool storeUpdatedMetaDataUnlocked();

      FhgfsOpsErr renameDirEntry(std::string fromName, std::string toName,
         DirEntry* overWriteEntry);
      FhgfsOpsErr renameDirEntryUnlocked(std::string fromName, std::string toName,
         DirEntry* overWriteEntry);

      static bool removeStoredMetaData(std::string id);
      static bool removeStoredMetaDataDir(std::string& id);
      static bool removeStoredMetaDataFile(std::string& id);

      FhgfsOpsErr refreshMetaInfoUnlocked();


      FhgfsOpsErr makeDirEntryUnlocked(DirEntry* entry, bool deleteEntry);
      FhgfsOpsErr linkFileInodeToDirUnlocked(std::string& inodePath, std::string &fileName);

      FhgfsOpsErr removeDirUnlocked(std::string entryName, DirEntry** outDirEntry);
      FhgfsOpsErr unlinkDirEntryUnlocked(std::string entryName, DirEntry* inEntry,
         unsigned unlinkTypeFlags);
      
      FhgfsOpsErr refreshSubentryCountUnlocked();

      bool loadFromFile();
      bool loadFromFileXAttr();
      bool loadFromFileContents();
      bool loadIfNotLoadedUnlocked();

      FhgfsOpsErr getEntryData(std::string entryName, EntryInfo* outInfo,
         FileInodeStoreData* outInodeMetaData);

      bool setAttrDataUnlocked(int validAttribs, SettableFileAttribs* attribs);
      FhgfsOpsErr setDirParentAndChangeTimeUnlocked(EntryInfo* entryInfo, uint16_t parentNodeID);

   protected:
      FhgfsOpsErr linkFilesInDirUnlocked(std::string fromName, FileInode* fromInode,
         std::string toName);


   public:
      // inliners

      
      /**
       * Note: Must be called before any of the mutator methods (otherwise they will fail)
       */
      FhgfsOpsErr storePersistentMetaData()
      {
         return storeInitialMetaData();
      }

      FhgfsOpsErr storePersistentMetaData(const CharVector& defaultACLXAttr,
         const CharVector& accessACLXAttr)
      {
         return storeInitialMetaData(defaultACLXAttr, accessACLXAttr);
      }
      
      /**
       * Unlink the dir-inode file on disk.
       */
      static bool unlinkStoredInode(std::string id)
      {
         return removeStoredMetaData(id);
      }

      /**
       * Note: Intended to be used by fsck only.
       */
      FhgfsOpsErr storeAsReplacementFile(std::string id)
      {
         // note: creates new dir metadata file for non-existing or invalid one => no locking needed

         removeStoredMetaDataFile(id);
         return storeInitialMetaDataInode();
      }
      
     /**
       * Return create a DirEntry for the given file name
       *
       * note: this->rwlock already needs to be locked
       */
      DirEntry* dirEntryCreateFromFileUnlocked(std::string entryName)
      {
         return this->entries.dirEntryCreateFromFile(entryName);
      }
      

      /**
       * Get a dentry
       * note: the difference to getDirEntryInfo/getFileEntryInfo is that this works independent
       * of the entry-type
       */
      bool getDentry(std::string entryName, DirEntry& outEntry)
      {
         bool retVal;

         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         retVal = getDentryUnlocked(entryName, outEntry);

         safeLock.unlock();

         return retVal;
      }

      /**
       * Get a dentry
       * note: the difference to getDirEntryInfo/getFileEntryInfo is that this works independent
       * of the entry-type
       */
      bool getDentryUnlocked(std::string entryName, DirEntry& outEntry)
      {
         return this->entries.getDentry(entryName, outEntry);
      }

      /**
       * Get the dentry (dir-entry) of a directory
       */
      bool getDirDentry(std::string dirName, DirEntry& outEntry)
      {
         bool retVal;
         
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         retVal = entries.getDirDentry(dirName, outEntry);

         safeLock.unlock();
         
         return retVal;
      }

      /*
       * Get the dentry (dir-entry) of a file
       */
      bool getFileDentry(std::string fileName, DirEntry& outEntry)
      {
         bool retVal;
         
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         retVal = entries.getFileDentry(fileName, outEntry);

         safeLock.unlock();
         
         return retVal;
      }
      
      /**
       * Get the EntryInfo
       * note: the difference to getDirEntryInfo/getFileEntryInfo is that this works independent
       * of the entry-type
       */
      bool getEntryInfo(std::string entryName, EntryInfo& outEntryInfo)
      {

         bool retVal;

         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         retVal = this->entries.getEntryInfo(entryName, outEntryInfo);

         safeLock.unlock();

         return retVal;
      }

      /**
       * Get the EntryInfo of a directory
       */
      bool getDirEntryInfo(std::string dirName, EntryInfo& outEntryInfo)
      {
         bool retVal;

         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         retVal = entries.getDirEntryInfo(dirName, outEntryInfo);

         safeLock.unlock();

         return retVal;
      }

      /*
       * Get the dir-entry of a file
       */
      bool getFileEntryInfo(std::string fileName, EntryInfo& outEntryInfo)
      {
         bool retVal;

         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         retVal = entries.getFileEntryInfo(fileName, outEntryInfo);

         safeLock.unlock();

         return retVal;
      }

      std::string getID()
      {
         return id;
      }

      uint16_t getOwnerNodeID()
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         uint16_t owner = ownerNodeID;

         safeLock.unlock();
         
         return owner;
      }
      
      bool setOwnerNodeID(uint16_t newOwner)
      {
         bool success = true;
         
         SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE);
         
         bool loadSuccess = loadIfNotLoadedUnlocked();
         if (!loadSuccess)
         {
            safeLock.unlock();
            return false;
         }

         uint16_t oldOwner = this->ownerNodeID;
         this->ownerNodeID = newOwner;
         
         if(!storeUpdatedMetaDataUnlocked() )
         { // failed to update metadata => restore old value
            this->ownerNodeID = oldOwner;
            
            success = false;
         }

         safeLock.unlock();
         
         return success;
      }
      
      void getParentInfo(std::string* outParentDirID, uint16_t* outParentNodeID)
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         *outParentDirID = this->parentDirID;
         *outParentNodeID = this->parentNodeID;

         safeLock.unlock();
      }

      /**
       * Note: Initial means for newly created objects (=> unlocked, unpersistent)
       */
      void setParentInfoInitial(std::string parentDirID, uint16_t parentNodeID)
      {
         this->parentDirID = parentDirID;
         this->parentNodeID = parentNodeID;
      }

      void setFeatureFlags(unsigned flags)
      {
         this->featureFlags = flags;
      }

      void addFeatureFlag(unsigned flag)
      {
         this->featureFlags |= flag;
      }

      void removeFeatureFlag(unsigned flag)
      {
         this->featureFlags &= ~flag;
      }

      unsigned getFeatureFlags() const
      {
         return this->featureFlags;
      }

      uint16_t getMirrorNodeID() const
      {
         return this->mirrorNodeID;
      }

      /**
       * Note: This is unlocked and unpersistent. Caller must either hold exclusive lock or must
       * ensure that no concurrent access happens (e.g. for newly created dir objects).
       */
      void setMirrorNodeIDUnlocked(uint16_t mirrorNodeID)
      {
         this->featureFlags |= DIRINODE_FEATURE_MIRRORED;
         this->mirrorNodeID = mirrorNodeID;
      }

      bool getExclusive()
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         bool retVal = this->exclusive;

         safeLock.unlock();
         
         return retVal;
      }
      
      void setExclusive(bool exclusive)
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE);

         this->exclusive = exclusive;

         safeLock.unlock();
      }
      
      unsigned getUserID()
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         unsigned retVal = this->statData.getUserID();

         safeLock.unlock();
         
         return retVal;
      }
      
      unsigned getUserIDUnlocked()
      {
         return this->statData.getUserID();
      }

      unsigned getGroupID()
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         unsigned retVal = this->statData.getGroupID();

         safeLock.unlock();
         
         return retVal;
      }
      
      int getMode()
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         int retVal = this->statData.getMode();

         safeLock.unlock();
         
         return retVal;
      }
      
      size_t getNumSubEntries()
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);
         
         size_t retVal = numSubdirs + numFiles;

         safeLock.unlock();
         
         return retVal;
      }

      
      static FhgfsOpsErr getStatData(std::string dirID, StatData& outStatData,
         uint16_t* outParentNodeID, std::string* outParentEntryID)
      {
         DirInode dir(dirID);
         if(!dir.loadFromFile() )
            return FhgfsOpsErr_PATHNOTEXISTS;

         return dir.getStatData(outStatData, outParentNodeID, outParentEntryID);
      }

      StripePattern* getStripePattern() const
      {
         return stripePattern;
      }
      
      /**
       * Move a dentry that has in inlined inode to the inode-hash directories
       * (probably an unlink() of an opened file).
       *
       * @param unlinkFileName Also unlink the fileName or only the ID file.
       */
      bool unlinkBusyFileUnlocked(std::string fileName, DirEntry* dentry, unsigned unlinkTypeFlags)
      {
         const char* logContext = "unlink busy file";

         FhgfsOpsErr unlinkRes = this->entries.removeBusyFile(fileName, dentry, unlinkTypeFlags);
         if (unlinkRes == FhgfsOpsErr_SUCCESS)
         {
            if( (unlinkTypeFlags & DirEntry_UNLINK_FILENAME) &&
               unlikely(!decreaseNumFilesAndStoreOnDisk() ) )
               unlinkRes = FhgfsOpsErr_INTERNAL;

            // add entry to mirror queue
            if(getFeatureFlags() & DIRINODE_FEATURE_MIRRORED)
            {
               std::string mirrorUnlinkName;

               if (unlinkTypeFlags & DirEntry_UNLINK_FILENAME)
                  mirrorUnlinkName = fileName;
               else
               {
                  // mirrorUnlinkName initialized with ""
               }

               MetadataMirrorer::removeDentryStatic(getID(), mirrorUnlinkName,
                  dentry->getEntryID(), getMirrorNodeID() );
            }
         }
         else
         {
            std::string msg = "Failed to remove file dentry. "
               "DirInode: " + this->id + " "
               "entryName: " + fileName + " "
               "entryID: " + dentry->getID() + " "
               "Error: " + FhgfsOpsErrTk::toErrString(unlinkRes);

            LogContext(logContext).logErr(msg);
         }


         return unlinkRes;
      }

      void setHsmCollocationIDUnlocked(HsmCollocationID hsmCollocationID)
      {
         this->hsmCollocationID = hsmCollocationID;

         // (un-)set feature flag
         if (this->hsmCollocationID != HSM_COLLOCATIONID_DEFAULT)
            this->addFeatureFlag(DIRINODE_FEATURE_HSM);
         else
            this->removeFeatureFlag(DIRINODE_FEATURE_HSM);
      }

      // for HSM integration
      void setHsmCollocationID(HsmCollocationID hsmCollocationID)
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE);

         setHsmCollocationIDUnlocked(hsmCollocationID);

         safeLock.unlock();
      }

      void setAndStoreHsmCollocationID(HsmCollocationID hsmCollocationID)
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE);

         setHsmCollocationIDUnlocked(hsmCollocationID);

         storeUpdatedMetaDataUnlocked();
         safeLock.unlock();
      }

      HsmCollocationID getHsmCollocationID()
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         HsmCollocationID retVal = getHsmCollocationIDUnlocked();

         safeLock.unlock();

         return retVal;
      }

      HsmCollocationID getHsmCollocationIDUnlocked()
      {
         return this->hsmCollocationID;
      }


   private:

      bool updateTimeStampsAndStoreToDisk(const char* logContext)
      {
         int64_t nowSecs = TimeAbs().getTimeval()->tv_sec;;
         this->statData.setAttribChangeTimeSecs(nowSecs);
         this->statData.setModificationTimeSecs(nowSecs);

         if(unlikely(!storeUpdatedMetaDataUnlocked() ) )
         {
            LogContext(logContext).logErr(std::string("Failed to update dir-info on disk: "
               "Dir-ID: ") + this->getID() + std::string(". SysErr: ") + System::getErrString() );

            return false;
         }

         return true;
      }


      bool increaseNumSubDirsAndStoreOnDisk(void)
      {
         const char* logContext = "DirInfo update: increase number of SubDirs";

         numSubdirs++;

         return updateTimeStampsAndStoreToDisk(logContext);
      }

      bool increaseNumFilesAndStoreOnDisk(void)
      {
         const char* logContext = "DirInfo update: increase number of Files";

         numFiles++;

         return updateTimeStampsAndStoreToDisk(logContext);
      }

      bool decreaseNumFilesAndStoreOnDisk(void)
      {
         const char* logContext = "DirInfo update: decrease number of Files";

         if (numFiles) // make sure it does not get sub-zero
            numFiles--;

         return updateTimeStampsAndStoreToDisk(logContext);
      }

      bool decreaseNumSubDirsAndStoreOnDisk(void)
      {
         const char* logContext = "DirInfo update: decrease number of SubDirs";

         if (numSubdirs) // make sure it does not get sub-zero
            numSubdirs--;

         return updateTimeStampsAndStoreToDisk(logContext);
      }
};

#endif /*DIRINODE_H_*/
