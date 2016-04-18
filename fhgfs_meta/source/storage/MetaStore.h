#ifndef METASTORE_H_
#define METASTORE_H_

#include <common/fsck/FsckDirInode.h>
#include <common/storage/striping/StripePattern.h>
#include <common/storage/Metadata.h>
#include <common/storage/StorageDefinitions.h>
#include <common/storage/StorageErrors.h>
#include <common/threading/SafeRWLock.h>
#include <common/threading/Atomics.h>
#include <common/storage/EntryInfo.h>
#include <common/Common.h>
#include <common/toolkit/FsckTk.h>
#include <storage/MkFileDetails.h>
#include "InodeFileStore.h"
#include "InodeDirStore.h"
#include "MetadataEx.h"
#include "DirEntry.h"


struct RmDirMapValues
{
      Mutex mutex;            // mutux for the condition
      AtomicInt64 refCounter; // refCounter for this oject, initialized automatically with zero
      bool rmDirSuccess;      // supposed to be set by the rmdir thread

      RmDirMapValues()
      {
         this->rmDirSuccess = false;
      }
};

typedef std::map<std::string, RmDirMapValues*> RmDirMap;
typedef RmDirMap::iterator RmDirMapIter;
typedef RmDirMap::value_type RmDirMapVal;

/*
 * This is the main class for all client side posix io operations regarding the meta server.
 * So client side net message will do io via this class.
 */
class MetaStore
{
   public:
      MetaStore() {};
      ~MetaStore() {};

      DirInode* referenceDir(const std::string dirID, const bool forceLoad);
      void releaseDir(std::string dirID);
      FileInode* referenceFile(EntryInfo* entryInfo);
      FileInode* referenceLoadedFile(std::string parentEntryID, std::string entryID);
      bool releaseFile(std::string parentEntryID, FileInode* file);
      bool referenceInode(std::string entryID, FileInode** outFileInode,
         DirInode** outDirInode);

      FhgfsOpsErr openFile(EntryInfo* entryInfo, unsigned accessFlags, FileInode** outFile);
      void closeFile(EntryInfo* entryInfo, FileInode* file, unsigned accessFlags,
         unsigned* outNumHardlinks, unsigned* outNumRefs);

      FhgfsOpsErr stat(EntryInfo* entryInfo, bool loadFromDisk, StatData& outStatData,
         uint16_t* outParentNodeID = NULL, std::string* parententryID = NULL);

      FhgfsOpsErr setAttr(EntryInfo* entryInfo, int validAttribs, SettableFileAttribs* attribs);
      FhgfsOpsErr setDirParent(EntryInfo* entryInfo, uint16_t parentNodeID);

      FhgfsOpsErr mkNewMetaFile(DirInode* dir, MkFileDetails* mkDetails,
         StripePattern* stripePattern, EntryInfo* outEntryInfo, FileInodeStoreData* outInodeData);

      FhgfsOpsErr makeDirInode(DirInode* dir);
      FhgfsOpsErr makeDirInode(DirInode* dir, const CharVector& defaultACLXAttr,
         const CharVector& accessACLXAttr);
      FhgfsOpsErr removeDirInode(std::string dirID);
      FhgfsOpsErr unlinkInode(EntryInfo* entryInfo, FileInode** outInode);
      FhgfsOpsErr fsckUnlinkFileInode(std::string entryID);
      FhgfsOpsErr unlinkFile(std::string dirID, std::string fileName, EntryInfo* outEntryInfo,
         FileInode** outFile);

      FhgfsOpsErr unlinkInodeLater(EntryInfo* entryInfo, bool wasInlined);

      FhgfsOpsErr renameInSameDir(DirInode* parentDir, std::string fromName,
         std::string toName, FileInode** outUnlinkInode);

      FhgfsOpsErr moveRemoteFileInsert(EntryInfo* fromFileInfo, std::string toParentID,
         std::string newEntryName, const char* buf, FileInode** outUnlinkedFile);

      FhgfsOpsErr moveRemoteFileBegin(DirInode* dir, EntryInfo* entryInfo, char* buf, size_t bufLen,
         size_t* outUsedBufLen);
      void moveRemoteFileComplete(DirInode* dir, std::string fileID);

      FhgfsOpsErr getAllInodesIncremental(unsigned hashDirNum, int64_t lastOffset,
          unsigned maxOutInodes, FsckDirInodeList* outDirInodes, FsckFileInodeList* outFileInodes,
          int64_t* outNewOffset);

      FhgfsOpsErr getAllEntryIDFilesIncremental(unsigned firstLevelhashDirNum,
         unsigned secondLevelhashDirNum, int64_t lastOffset, unsigned maxOutEntries,
         StringList* outEntryIDFiles, int64_t* outNewOffset);

      void getReferenceStats(size_t* numReferencedDirs, size_t* numReferencedFiles);
      void getCacheStats(size_t* numCachedDirs);

      bool cacheSweepAsync();

      FhgfsOpsErr insertDisposableFile(FileInode* inode);

      FhgfsOpsErr getEntryData(DirInode *dirInode, std::string entryName,
         EntryInfo* outInfo, FileInodeStoreData* outInodeMetaData);

      FhgfsOpsErr linkInSameDir(EntryInfo* parentInfo, EntryInfo* fromFileInfo,
         std::string fromName, std::string toName);

      FhgfsOpsErr getDirXAttr(DirInode* dir, const std::string& xAttrName,
         CharVector& outValue);

   private:
      InodeDirStore dirStore;

      /* We need to avoid to use that one, as it is a global store, with possible lots of entries.
       * So access to the map is slow and inserting entries blocks the entire MetaStore */
      InodeFileStore fileStore;

      RWLock rwlock; /* note: this is mostly not used as a read/write-lock but rather a shared/excl
         lock (because we're not really modifying anyting directly) - especially relevant for the
         mutliple dirStore locking dual-move methods */

      FhgfsOpsErr isFileUnlinkable(DirInode* subDir, EntryInfo* entryInfo);

      FhgfsOpsErr mkMetaFileUnlocked(DirInode* dir, std::string entryName,
         DirEntryType entryType, FileInode* inode);
      FhgfsOpsErr mkNewMetaFileUnlocked(DirInode* dir, MkFileDetails* mkDetails,
         StripePattern* stripePattern, EntryInfo* outEntryInfo,
         FileInodeStoreData* outInodeData);

      FhgfsOpsErr unlinkInodeUnlocked(EntryInfo* entryInfo, DirInode* subDir, FileInode** outInode);
      FhgfsOpsErr unlinkInodeLaterUnlocked(EntryInfo* entryInfo, bool wasInlined);

      FhgfsOpsErr unlinkFileUnlocked(DirInode* subdir, std::string fileName,
         FileInode** outInode, EntryInfo* outEntryInfo, bool& outWasInlined);

      FhgfsOpsErr unlinkDirEntryWithInlinedInodeUnlocked(std::string fileName, DirInode* subdir,
         DirEntry* dirEntry, unsigned unlinkTypeFlags, FileInode** outInode);
      FhgfsOpsErr unlinkDentryAndInodeUnlocked(std::string fileName, DirInode* subdir,
         DirEntry* dirEntry, unsigned unlinkTypeFlags, FileInode** outInode);

      FhgfsOpsErr unlinkOverwrittenEntry(DirInode* parentDir, DirEntry* overWrittenEntry,
         FileInode** outInode);
      FhgfsOpsErr unlinkOverwrittenEntryUnlocked(DirInode* parentDir, DirEntry* overWrittenEntry,
         FileInode** outInode);


      // int readDirs(std::string pathStr, StringVector *outDirs);
      // int readEntries(std::string pathStr, MetaDataEntryVec *outEntries);

      DirInode* referenceDirUnlocked(std::string dirID, bool forceLoad);
      DirInode* referenceAndReadLockDirUnlocked(std::string dirID, bool forceload = false);
      DirInode* referenceAndWriteLockDirUnlocked(std::string dirID);
      void releaseDirUnlocked(std::string dirID);
      void unlockDirUnlocked(DirInode* dir);
      void unlockAndReleaseDirUnlocked(std::string dirID, DirInode* dir);
      FileInode* referenceFileUnlocked(EntryInfo* entryInfo);
      FileInode* referenceFileUnlocked(DirInode* subDir, EntryInfo* entryInfo);
      FileInode* referenceLoadedFileUnlocked(std::string parentEntryID, std::string entryID);
      FileInode* referenceLoadedFileUnlocked(DirInode* subDir, std::string entryID);
      bool releaseFileUnlocked(std::string parentEntryID, FileInode* inode);
      bool releaseFileUnlocked(DirInode* subDir, FileInode* inode);

      bool moveReferenceToMetaFileStoreUnlocked(std::string parentEntryID,
         std::string entryID);

      FhgfsOpsErr performRenameEntryInSameDir(DirInode* dir, std::string fromName,
         std::string toName, DirEntry** outOverwrittenEntry);
      FhgfsOpsErr checkRenameOverwrite(EntryInfo* fromEntry, EntryInfo* overWriteEntry,
         bool& outIsSameInode);

      FhgfsOpsErr setAttrUnlocked(EntryInfo* entryInfo, int validAttribs,
         SettableFileAttribs* attribs);

   public:
      // getters & setters

      // inliners

};


#endif /* METASTORE_H_ */
