#ifndef FILEINODE_H_
#define FILEINODE_H_

#include <common/storage/striping/StripePattern.h>
#include <common/storage/StorageDefinitions.h>
#include <common/storage/StorageErrors.h>
#include <common/storage/PathInfo.h>
#include <common/storage/StatData.h>
#include <common/storage/HsmFileMetaData.h>
#include <common/storage/striping/ChunkFileInfo.h>
#include <common/threading/SafeMutexLock.h>
#include <common/threading/SafeRWLock.h>
#include <common/threading/Condition.h>
#include <common/threading/PThread.h>
#include <common/toolkit/TimeAbs.h>
#include <common/app/log/LogContext.h>
#include <common/Common.h>
#include <session/LockingNotifier.h>
#include "Locking.h"
#include "MetadataEx.h"
#include "DiskMetaData.h"
#include "DentryStoreData.h"
#include "FileInodeStoreData.h"


typedef std::vector<DynamicFileAttribs> DynamicFileAttribsVec;
typedef DynamicFileAttribsVec::iterator DynamicFileAttribsVecIter;
typedef DynamicFileAttribsVec::const_iterator DynamicFileAttribsVecCIter;

typedef std::vector<ChunkFileInfo> ChunkFileInfoVec;
typedef ChunkFileInfoVec::iterator ChunkFileInfoVecIter;
typedef ChunkFileInfoVec::const_iterator ChunkFileInfoVecCIter;


/**
 * Data are not really belonging to the inode, but we just need it to write inodes in the
 * dentry-format. We need to read those from disk first or obtain it from the dentry.
 */
struct DentryCompatData
{
      DentryCompatData()
      {
         this->entryType    = DirEntryType_INVALID;
         this->featureFlags = 0;
      }

      DirEntryType entryType;
      unsigned featureFlags;
};


/**
 * Our inode object, but for files only (so all file types except of directories).
 * Directories are in class DirInode.
 */
class FileInode
{
   friend class MetaStore;
   friend class InodeFileStore;
   friend class SessionFile; /* (needed for entry/range locking) */


   public:

      /**
       * Should be rarely used and if required, only with an immediate following
       * inode->deserialize(buf)
       */
      FileInode();

      /**
       * The preferred inode initializer.
       */
      FileInode(std::string entryID, FileInodeStoreData* inodeStoreData,
         DirEntryType entryType, unsigned dentryFeatureFlags);


      ~FileInode()
      {
         LOG_DEBUG("Delete FileInode", Log_SPAM, std::string("Deleting inode: ") + getEntryID());
      }

      void decNumSessionsAndStore(EntryInfo* entryInfo, unsigned accessFlags);
      static FileInode* createFromEntryInfo(EntryInfo* entryInfo);

      unsigned serializeMetaData(char* buf);

      bool flockAppend(EntryLockDetails& lockDetails);
      void flockAppendCancelAllWaiters();
      void flockAppendCancelByClientID(std::string clientID);
      std::string flockAppendGetAllAsStr();
      bool flockEntry(EntryLockDetails& lockDetails);
      void flockEntryCancelAllWaiters();
      void flockEntryCancelByClientID(std::string clientID);
      std::string flockEntryGetAllAsStr();
      bool flockRange(RangeLockDetails& lockDetails);
      void flockRangeCancelAllWaiters();
      void flockRangeCancelByClientID(std::string clientID);
      bool flockRangeGetConflictor(RangeLockDetails& lockDetails, RangeLockDetails* outConflictor);
      std::string flockRangeGetAllAsStr();

      bool deserializeMetaData(const char* buf);


   private:
      FileInodeStoreData inodeDiskData;

      ChunkFileInfoVec fileInfoVec; // stores individual data for each node (e.g. file length),
         // esp. the DYNAMIC ATTRIBS, that live only in memory and are not stored to disk (compared
         // to static attribs)

      pthread_t exclusiveTID; // ID of the thread, which has exclusiveTID access to the inode

      unsigned numSessionsRead; // open read-only
      unsigned numSessionsWrite; // open for writing or read-write


      bool isInlined; // boolean if the inode inlined into the Dentry or a separate file

      // append lock queues (exclusive only, entire file locking)
      EntryLockDetails exclAppendLock; // current exclusiveTID lock
      EntryLockDetailsList waitersExclAppendLock; // queue (append new to end, pop from top)
      StringSet waitersLockIDsAppendLock; // currently enqueued lockIDs (for fast duplicate check)

      // flock() queues (entire file locking, i.e. no ranges)
      EntryLockDetails exclFLock; // current exclusiveTID lock
      EntryLockDetailsSet sharedFLocks; // current shared locks (key is lock, value is dummy)
      EntryLockDetailsList waitersExclFLock; // queue (append new to end, pop from top)
      EntryLockDetailsList waitersSharedFLock; // queue (append new to end, pop from top)
      StringSet waitersLockIDsFLock; // currently enqueued lockIDs (for fast duplicate check)

      // fcntl() flock queues (range-based)
      RangeLockExclSet exclRangeFLocks; // current exclusiveTID locks
      RangeLockSharedSet sharedRangeFLocks;  // current shared locks (key is lock, value is dummy)
      RangeLockDetailsList waitersExclRangeFLock; // queue (append new to end, pop from top)
      RangeLockDetailsList waitersSharedRangeFLock; // queue (append new to end, pop from top)
      StringSet waitersLockIDsRangeFLock; // currently enqueued lockIDs (for fast duplicate check)

      RWLock rwlock; // default inode lock

      DentryCompatData dentryCompatData;

      RWLock setGetReferenceParentLock; // just to set/get the referenceParentID
      std::string referenceParentID;
      AtomicSSizeT numParentRefs; /* counter how often parentDir is referenced, 0 if we
                                   * are part of MetaStores FileStore */


      static FileInode* createFromInlinedInode(EntryInfo* entryInfo);
      static FileInode* createFromInodeFile(EntryInfo* entryInfo);

      void initFileInfoVec();
      void updateDynamicAttribs(void);

      bool setAttrData(EntryInfo* entryInfo, int validAttribs, SettableFileAttribs* attribs);
      bool incDecNumHardLinks(EntryInfo * entryInfo, int value);

      bool storeUpdatedMetaDataBuf(char* buf, unsigned bufLen);
      bool storeUpdatedMetaDataBufAsXAttr(char* buf, unsigned bufLen, std::string metaFileName);
      bool storeUpdatedMetaDataBufAsContents(char* buf, unsigned bufLen, std::string metaFileName);
      bool storeUpdatedMetaDataBufAsContentsInPlace(char* buf, unsigned bufLen,
         std::string metaFileName);
      bool storeUpdatedInodeUnlocked(EntryInfo* entryInfo,
         StripePattern* updatedStripePattern = NULL);
      FhgfsOpsErr storeUpdatedInlinedInodeUnlocked(EntryInfo* entryInfo,
         StripePattern* updatedStripePattern = NULL);

      static bool removeStoredMetaData(std::string id);

      bool loadFromInodeFile(EntryInfo* entryInfo);
      bool loadFromFileXAttr(const std::string& id);
      bool loadFromFileContents(const std::string& id);

      bool flockEntryUnlocked(EntryLockDetails& lockDetails, EntryLockQueuesContainer* lockQs);
      bool flockEntryCancelByHandle(EntryLockDetails& lockDetails,
         EntryLockQueuesContainer* lockQs);
      void flockEntryGenericCancelAllWaiters(EntryLockQueuesContainer* lockQs);
      void flockEntryGenericCancelByClientID(std::string clientID,
         EntryLockQueuesContainer* lockQs);
      std::string flockEntryGenericGetAllAsStr(EntryLockQueuesContainer* lockQs);

      bool flockEntryCheckConflicts(EntryLockDetails& lockDetails, EntryLockQueuesContainer* lockQs,
         EntryLockDetails* outConflictor);
      bool flockEntryIsGranted(EntryLockDetails& lockDetails, EntryLockQueuesContainer* lockQs);
      bool flockEntryUnlock(EntryLockDetails& lockDetails, EntryLockQueuesContainer* lockQs);
      void flockEntryShared(EntryLockDetails& lockDetails, EntryLockQueuesContainer* lockQs);
      void flockEntryExclusive(EntryLockDetails& lockDetails, EntryLockQueuesContainer* lockQs);
      void flockEntryTryNextWaiters(EntryLockQueuesContainer* lockQs);

      bool flockRangeUnlocked(RangeLockDetails& lockDetails);
      bool flockRangeCancelByHandle(RangeLockDetails& lockDetails);

      bool flockRangeCheckConflicts(RangeLockDetails& lockDetails, RangeLockDetails* outConflictor);
      bool flockRangeCheckConflictsEx(RangeLockDetails& lockDetails, int maxExclWaitersCheckNum,
         RangeLockDetails* outConflictor);
      bool flockRangeIsGranted(RangeLockDetails& lockDetails);
      bool flockRangeUnlock(RangeLockDetails& lockDetails);
      void flockRangeShared(RangeLockDetails& lockDetails);
      void flockRangeExclusive(RangeLockDetails& lockDetails);
      void flockRangeTryNextWaiters();


   public:
      // inliners

      /**
       * Save meta-data to disk
       */
      bool updateInodeOnDisk(EntryInfo* entryInfo, StripePattern* updatedStripePattern = NULL)
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE); // L O C K

         bool retVal = storeUpdatedInodeUnlocked(entryInfo, updatedStripePattern);

         safeLock.unlock(); // U N L O C K

         return retVal;
      }

      /**
       * Unlink the stored file-inode file on disk.
       */
      static bool unlinkStoredInodeUnlocked(std::string id)
      {
         return removeStoredMetaData(id);
      }


      // getters & setters
      std::string getEntryID()
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         std::string entryID = getEntryIDUnlocked();

         safeLock.unlock();

         return entryID;
      }

      std::string getEntryIDUnlocked()
      {
         return this->inodeDiskData.getEntryID();
      }


      /**
       * Note: Does not (immediately) update the persistent metadata.
       * Note: Be careful with this!
       */
      void setIDUnpersistent(std::string newID)
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE);

         this->inodeDiskData.setEntryID(newID);

         safeLock.unlock();
      }

      StripePattern* getStripePatternUnlocked()
      {
         return this->inodeDiskData.getStripePattern();
      }

      StripePattern* getStripePattern()
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         StripePattern* pattern = this->getStripePatternUnlocked();

         safeLock.unlock();

         return pattern;
      }

      /**
       * Return the stripe pattern and set our value to NULL.
       *
       * Note: Usually this inode should not be further used anymore.
       */
      StripePattern* getStripePatternAndSetToNull()
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE);

         StripePattern* pattern = this->inodeDiskData.getStripePatternAndSetToNull();

         safeLock.unlock();

         return pattern;
      }

      void setFeatureFlags(unsigned flags)
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE);

         this->inodeDiskData.setInodeFeatureFlags(flags);

         safeLock.unlock();
      }

      void addFeatureFlag(unsigned flag)
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE);

         addFeatureFlagUnlocked(flag);

         safeLock.unlock();
      }

      unsigned getFeatureFlags()
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         unsigned flags = this->inodeDiskData.getInodeFeatureFlags();

         safeLock.unlock();

         return flags;
      }

      unsigned getUserID()
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         unsigned retVal = this->inodeDiskData.getInodeStatData()->getUserID();

         safeLock.unlock();

         return retVal;
      }

      unsigned getGroupID()
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         unsigned retVal = this->inodeDiskData.getInodeStatData()->getGroupID();

         safeLock.unlock();

         return retVal;
      }

      int getMode()
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         int retVal = this->inodeDiskData.getInodeStatData()->getMode();

         safeLock.unlock();

         return retVal;
      }

      uint16_t getMirrorNodeID()
      {
         return this->inodeDiskData.getMirrorNodeID();
      }


      /**
       * Note: We do not set our class values here yet, because this method is called frequently and
       *       we do not need our class values here yet. So class values are only set on demand.
       *       this->updateDynamicAttribs() will set/update our class values·
       *
       * @param erroneousVec if errors occurred during vector retrieval from storage nodes
       *       (this parameter was used to set dynAttribsUptodate to false, but we do not have
       *        this value anymore since the switch to (post-)DB metadata format)
       * @return false if error during save occurred (but we don't save dynAttribs currently)
       *
       */
      bool setDynAttribs(DynamicFileAttribsVec& newAttribsVec)
      {
         /* note: we cannot afford to make a disk write each time this is updated, so
            we only update metadata on close currrently */

         bool retVal = true;
         bool anyAttribUpdated = false;

         SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE);

         if(unlikely(newAttribsVec.size() != fileInfoVec.size() ) )
         { // should never happen
            PThread::getCurrentThreadApp()->getLogger()->logErr("Apply dynamic file attribs",
               "Vector sizes do not match - This should never happen!");

            retVal = false;
            goto unlock_and_exit;
         }


         for(size_t i=0; i < newAttribsVec.size(); i++)
         {
            if(fileInfoVec[i].updateDynAttribs(newAttribsVec[i] ) )
               anyAttribUpdated = true;
         }

         IGNORE_UNUSED_VARIABLE(anyAttribUpdated);

      unlock_and_exit:
         safeLock.unlock();

         return retVal;
      }

      /**
       * Note: Use this only if the file is not loaded already (because otherwise the dyn attribs
       * will be outdated)
       */
      static FhgfsOpsErr getStatData(EntryInfo* entryInfo, StatData& outStatData)
      {

         // TODO: Add code for "loadFromEntryInfo(), to avoid new/delete?
         FileInode* inode = createFromEntryInfo(entryInfo);

         if(!inode)
            return FhgfsOpsErr_PATHNOTEXISTS;

         FhgfsOpsErr retVal = inode->getStatData(outStatData);

         delete inode;

         return retVal;
      }

      /**
       * Unlocked method to get statdata
       *
       * Note: Needs write-lock
       */
      FhgfsOpsErr getStatDataUnlocked(StatData& outStatData)
      {
         FhgfsOpsErr statRes = FhgfsOpsErr_SUCCESS;

         this->updateDynamicAttribs();

         outStatData = *(this->inodeDiskData.getInodeStatData() );

         /* note: we don't check numSessionsRead below (for dyn attribs outated statRes), because
            that would be too much overhead; side-effect is that we don't update atime for read-only
            files while they are open. */

         if(numSessionsWrite)
            statRes = FhgfsOpsErr_DYNAMICATTRIBSOUTDATED;

         return statRes;
      }


      /**
       * Return statData
       *
       * Note: Also updates dynamic attribs, so needs a write lock
       */
      FhgfsOpsErr getStatData(StatData& outStatData)
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE);

         FhgfsOpsErr statRes = getStatDataUnlocked(outStatData);

         safeLock.unlock();

         return statRes;
      }

      void setStatData(StatData& statData)
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE);

         this->inodeDiskData.setInodeStatData(statData);

         safeLock.unlock();
      }

      /**
       * Note: Does not take a lock and is a very special use case. For example on moving a file
       *       to a remote server, when an inode object is created from a buffer and we want to
       *       have values from this inode object.
       */
      StatData* getNotUpdatedStatDataUseCarefully()
      {
         return this->inodeDiskData.getInodeStatData();
      }

      void setNumHardlinksUnpersistent(unsigned numHardlinks)
      {
         /* note: this is the version for the quick on-close unlink check, so file is referenced by
          *       client sessions and we don't need to update persistent metadata
          */

         SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE);

         this->inodeDiskData.setNumHardlinks(numHardlinks);

         safeLock.unlock();
      }

      unsigned getNumHardlinks()
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         unsigned retVal = this->inodeDiskData.getNumHardlinks();

         safeLock.unlock();

         return retVal;
      }

      /**
       * Get the thread ID of the thread allow to work with this inode, so the ID, which has
       * set the exclusiveTID value.
       */
      pthread_t getExclusiveThreadID()
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         pthread_t retVal = this->exclusiveTID;

         safeLock.unlock();

         return retVal;
      }

      void setExclusiveTID(pthread_t posixTID)
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE);

         this->exclusiveTID = posixTID;

         safeLock.unlock();
      }

      /**
       * @return num read-sessions + num write-sessions
       */
      unsigned getNumSessionsAll()
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         unsigned num = this->numSessionsRead + this->numSessionsWrite;

         safeLock.unlock();

         return num;
      }

      unsigned getNumSessionsRead()
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         unsigned num = this->numSessionsRead;

         safeLock.unlock();

         return num;
      }

      unsigned getNumSessionsWrite()
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         unsigned num = this->numSessionsWrite;

         safeLock.unlock();

         return num;
      }

      /**
       * Increase number of sessions for read or write (=> file open).
       *
       * @param accessFlags OPENFILE_ACCESS_... flags
       */
      void incNumSessions(unsigned accessFlags)
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE);

         if(accessFlags & OPENFILE_ACCESS_READ)
            this->numSessionsRead++;
         else
            this->numSessionsWrite++; // (includes read+write)

         safeLock.unlock();
      }

      FileInode* clone()
      {
         char* buf = (char*)malloc(META_SERBUF_SIZE);
         serializeMetaData(buf);

         FileInode *clone = new FileInode();
         bool cloneRes = clone->deserializeMetaData(buf);
         if (unlikely(cloneRes == false) )
         {
            delete clone;
            clone = NULL;
         }
         else
         {
            /* Ihe inode serializer might will not set origParentEntryID and origParentUID if the file
             * was not moved or the inode was not de-inlined, but the cloned inode needs these
             * values */
            clone->inodeDiskData.origParentEntryID = this->inodeDiskData.origParentEntryID;
            clone->inodeDiskData.origParentUID           = this->inodeDiskData.origParentUID;
         }

         free(buf);

         return clone;
      }

      /**
       * TODO: must handle hardlinked inodes, so from different dirs
       */
      void incParentRef(std::string& referenceParentID)
      {
         this->numParentRefs.increase();

         SafeRWLock rwLock(&this->setGetReferenceParentLock, SafeRWLock_WRITE);
         this->referenceParentID = referenceParentID;
         rwLock.unlock();
      }

      void decParentRef()
      {
         this->numParentRefs.decrease();
      }

      ssize_t getNumParentRefs()
      {
         return this->numParentRefs.read();
      }
      
      /*
       * TODO: What are we going to return if the inode is hardlinked and referenced from
       *       multiple dirs? */
      std::string getReferenceParentID()
      {
         SafeRWLock rwLock(&this->setGetReferenceParentLock, SafeRWLock_READ);

         std::string id = this->referenceParentID;

         rwLock.unlock();

         return id;
      }

      /**
       * Update inline information.
       *
       * @param value   If set to false pathInfo flags and dentryCompat data will be updated as
       *                well.
       */
      void setIsInlined(bool value)
      {
         SafeRWLock safeLock(&this->rwlock, SafeRWLock_WRITE);

         setIsInlinedUnlocked(value);

         safeLock.unlock();
      }

      /**
       * See setIsInlined().
       */
      void setIsInlinedUnlocked(bool value)
      {
         this->isInlined = value;

         if (value == false)
         {
            if (this->inodeDiskData.getOrigFeature() == FileInodeOrigFeature_TRUE)
            {
               /* If the inode gets de-inlined origParentEntryID cannot be set from parentDir
                * anymore. */
               addFeatureFlagUnlocked(FILEINODE_FEATURE_HAS_ORIG_PARENTID);
            }

            // dentryCompatData also need the update
            this->dentryCompatData.featureFlags &= ~(DENTRY_FEATURE_INODE_INLINE);


         }
         else
            this->dentryCompatData.featureFlags |= DENTRY_FEATURE_INODE_INLINE;
      }

      bool getIsInlined()
      {
         SafeRWLock rwLock(&this->rwlock, SafeRWLock_READ);

         bool retVal = this->isInlined;

         rwLock.unlock();

         return retVal;
      }

      bool hsmRelease(EntryInfo* entryInfo, uint8_t urgency)
      {
         // first step for now : always allow release
         incHsmOfflineFileChunkCount(entryInfo);

         return true;
      }

      void hsmRecalled(EntryInfo* entryInfo)
      {
         decHsmOfflineFileChunkCount(entryInfo);
      }

      void setHsmFileMetaDataUnpersistent(HsmFileMetaData& hsmFileMetaData)
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE);

         this->inodeDiskData.setHsmFileMetaData(hsmFileMetaData);

         safeLock.unlock();
      }

      HsmCollocationID getHsmCollocationID()
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         HsmCollocationID retVal = this->inodeDiskData.getHsmCollocationID();

         safeLock.unlock();

         return retVal;
      }

      void setAndStoreHsmCollocationID(EntryInfo* entryInfo, HsmCollocationID hsmCollocationID)
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE);

         this->inodeDiskData.setHsmCollocationID(hsmCollocationID);

         storeUpdatedInodeUnlocked(entryInfo);

         safeLock.unlock();
      }

      void setHsmCollocationID(HsmCollocationID hsmCollocationID)
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE);

         this->inodeDiskData.setHsmCollocationID(hsmCollocationID);

         safeLock.unlock();
      }

      uint16_t getHsmOfflineFileChunkCount()
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);
         uint16_t retVal = this->inodeDiskData.getHsmOfflineFileChunkCount();
         safeLock.unlock();

         return retVal;
      }

      void setHsmOfflineChunkCountUnpersistent(uint16_t offlineChunkCount)
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE);

         this->inodeDiskData.setHsmOfflineChunkCount(offlineChunkCount);

         safeLock.unlock();
      }

      /**
       * Increase number of offline file chunks
       */
      void incHsmOfflineFileChunkCount(EntryInfo* entryInfo)
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE);

         unsigned numTargets = (unsigned)(getStripePatternUnlocked()->getStripeTargetIDs()->size());

         incHsmOfflineFileChunkCountUnlockedUnpersistent(numTargets);

         storeUpdatedInodeUnlocked(entryInfo);

         safeLock.unlock();
      }

      /**
       * Increase number of offline file chunks
       * Unlocked, so make sure to lock in calling function
       *
       * @param numTargets the number of targets this file is striped across
       */
      void incHsmOfflineFileChunkCountUnlockedUnpersistent(unsigned numTargets)
      {
         if(unlikely(numTargets == this->inodeDiskData.getHsmOfflineFileChunkCount() ) )
         {
            LogContext log("File::offlineChunkCount");
            log.logErr(
               std::string("Warning: offlineChunkCount is already equal to the number of stripe "
                  "targets. " + std::string("File: ") + this->getEntryIDUnlocked()) );
         }
         else
         {
            this->inodeDiskData.incHsmOfflineFileChunkCount();
         }
      }

      /**
       * Decrease number of offline file chunks
       */
      void decHsmOfflineFileChunkCount(EntryInfo* entryInfo)
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE);

         decHsmOfflineFileChunkCountUnlockedUnpersistent();

         storeUpdatedInodeUnlocked(entryInfo);

         safeLock.unlock();
      }

      /**
       * Decrease number of offline file chunks
       * Unlocked, so make sure to lock in calling function
       */
      void decHsmOfflineFileChunkCountUnlockedUnpersistent()
      {
         if(unlikely(!this->inodeDiskData.getHsmOfflineFileChunkCount() ) )
         {
            LogContext log("File::offlineChunkCount");
            log.logErr(
               std::string("Warning: offlineChunkCount is already zero. " +
               std::string("File: ") + this->getEntryIDUnlocked()) );
         }
         else
            this->inodeDiskData.decHsmOfflineFileChunkCount();
      }

      void getPathInfo(PathInfo* outPathInfo)
      {
         SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

         this->inodeDiskData.getPathInfo(outPathInfo);

         safeLock.unlock();
      }


   protected:

      void setOrigParentID(std::string origParentEntryId)
      {
         this->inodeDiskData.setDynamicOrigParentEntryID(origParentEntryId);
      }

      void setPersistentOrigParentID(std::string origParentEntryId)
      {
         this->inodeDiskData.setPersistentOrigParentEntryID(origParentEntryId);
      }

   private:

      /**
       * Increase link counter by one
       */
      void incDecNumHardlinksUnpersistentUnlocked(int value)
      {
         this->inodeDiskData.incDecNumHardlinks(value);
      }

      FileInodeStoreData* getInodeDiskData()
      {
         return &this->inodeDiskData;
      }

      void addFeatureFlagUnlocked(unsigned flag)
      {
         this->inodeDiskData.addInodeFeatureFlag(flag);
      }

};

#endif /*FILEINODE_H_*/
