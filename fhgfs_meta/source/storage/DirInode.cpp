#include <net/msghelpers/MsgHelperXAttr.h>
#include <common/storage/striping/Raid0Pattern.h>
#include <common/storage/striping/Raid10Pattern.h>
#include <common/toolkit/TimeAbs.h>
#include <program/Program.h>
#include <storage/PosixACL.h>

#include <sys/types.h>
#include <attr/xattr.h>
#include <dirent.h>

#include "DiskMetaData.h"
#include "DirInode.h"

/**
 * Constructur used to create new directories.
 */
DirInode::DirInode(std::string id, int mode, unsigned userID, unsigned groupID,
   uint16_t ownerNodeID, StripePattern& stripePattern) :
   id(id), ownerNodeID(ownerNodeID), entries(id)
{
   //this->id               = id; // done in initializer list
   //this->ownerNodeID      = ownerNodeID; // done in initializer list
   this->stripePattern      = stripePattern.clone();
   this->parentNodeID       = 0; // 0 means undefined
   this->featureFlags       = DIRINODE_FEATURE_EARLY_SUBDIRS | DIRINODE_FEATURE_STATFLAGS;
   this->mirrorNodeID       = 0;
   this->exclusive          = false;
   int64_t currentTimeSecs  = TimeAbs().getTimeval()->tv_sec;
   this->numSubdirs         = 0;
   this->numFiles           = 0;

   SettableFileAttribs settableAttribs = {mode, userID, groupID, currentTimeSecs, currentTimeSecs};

   this->statData.setInitNewDirInode(&settableAttribs, currentTimeSecs);

   this->hsmCollocationID = HSM_COLLOCATIONID_DEFAULT;

   this->isLoaded = true;
}

/**
 * Create a stripe pattern based on the current pattern config and add targets to it.
 *
 * @param preferredTargets may be NULL
 */
StripePattern* DirInode::createFileStripePattern(UInt16List* preferredTargets,
   unsigned numtargets, unsigned chunksize)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   StripePattern* filePattern = createFileStripePatternUnlocked(
      preferredTargets, numtargets, chunksize);

   safeLock.unlock(); // U N L O C K

   return filePattern;
}

/**
 * @param numtargets 0 to use setting from directory (-1/~0 for all available targets).
 * @param chunksize 0 to use setting from directory, must be 2^n and >=64Ki otherwise.
 * @return NULL on error, new object on success.
 */
StripePattern* DirInode::createFileStripePatternUnlocked(UInt16List* preferredTargets,
   unsigned numtargets, unsigned chunksize)
{
   App* app = Program::getApp();
   Config* cfg = app->getConfig();
   TargetChooserType chooserType = cfg->getTuneTargetChooserNum();

   bool loadSuccess = loadIfNotLoadedUnlocked();
   if (!loadSuccess)
      return NULL;

   if(!chunksize)
      chunksize = stripePattern->getChunkSize(); // use default dir chunksize
   else
   if(unlikely( (chunksize < STRIPEPATTERN_MIN_CHUNKSIZE) ||
                !MathTk::isPowerOfTwo(chunksize) ) )
      return NULL; // invalid chunksize given

   StripePattern* filePattern;

   // choose some available storage targets

   unsigned desiredNumTargets = numtargets ? numtargets : stripePattern->getDefaultNumTargets();
   unsigned minNumRequiredTargets = stripePattern->getMinNumTargets();
   UInt16Vector stripeTargets;

   if (desiredNumTargets < minNumRequiredTargets)
      desiredNumTargets = minNumRequiredTargets;

   if(stripePattern->getPatternType() == STRIPEPATTERN_BuddyMirror)
   { // buddy mirror is special case, because no randmoninternode support and NodeCapacityPool
      NodeCapacityPools* capacityPools = app->getStorageBuddyCapacityPools();

      /* (note: chooserType random{inter,intra}node is not supported by buddy mirrors, because we
         can't do the internal node-based grouping for buddy mirrors, so we fallback to random in
         in this case) */

      if( (chooserType == TargetChooserType_RANDOMIZED) ||
          (chooserType == TargetChooserType_RANDOMINTERNODE) ||
          (chooserType == TargetChooserType_RANDOMINTRANODE) ||
          (preferredTargets && !preferredTargets->empty() ) )
      { // randomized chooser, the only chooser that currently supports preferredTargets
         capacityPools->chooseStorageTargets(
            desiredNumTargets, minNumRequiredTargets, preferredTargets, &stripeTargets);
      }
      else
      { // round robin or randomized round robin chooser
         capacityPools->chooseStorageTargetsRoundRobin(
            desiredNumTargets, &stripeTargets);

         if(chooserType == TargetChooserType_RANDOMROBIN)
            random_shuffle(stripeTargets.begin(), stripeTargets.end() ); // randomize result vector
      }
   }
   else
   {
      TargetCapacityPools* capacityPools = app->getStorageCapacityPools();

      if(chooserType == TargetChooserType_RANDOMIZED ||
         (preferredTargets && !preferredTargets->empty() ) )
      { // randomized chooser, the only chooser that currently supports preferredTargets
         capacityPools->chooseStorageTargets(
            desiredNumTargets, minNumRequiredTargets, preferredTargets, &stripeTargets);
      }
      else
      if(chooserType == TargetChooserType_RANDOMINTERNODE)
      { // select targets from different nodeIDs
         capacityPools->chooseTargetsInterdomain(
            desiredNumTargets, minNumRequiredTargets, &stripeTargets);
      }
      else
      if(chooserType == TargetChooserType_RANDOMINTRANODE)
      { // select targets from different nodeIDs
         capacityPools->chooseTargetsIntradomain(
            desiredNumTargets, minNumRequiredTargets, &stripeTargets);
      }
      else
      { // round robin or randomized round robin chooser
         capacityPools->chooseStorageTargetsRoundRobin(
            desiredNumTargets, &stripeTargets);

         if(chooserType == TargetChooserType_RANDOMROBIN)
            random_shuffle(stripeTargets.begin(), stripeTargets.end() ); // randomize result vector
      }
   }


   // check whether we got enough targets...

   if(unlikely(stripeTargets.size() < minNumRequiredTargets) )
      return NULL;

   // clone the dir pattern with new stripeTargets...

   if(cfg->getTuneRotateMirrorTargets() &&
      (stripePattern->getPatternType() == STRIPEPATTERN_Raid10) )
      filePattern = ( (Raid10Pattern*)stripePattern)->cloneRotated(stripeTargets);
   else
      filePattern = stripePattern->clone(stripeTargets);

   filePattern->setDefaultNumTargets(desiredNumTargets);
   filePattern->setChunkSize(chunksize);

   return filePattern;
}

StripePattern* DirInode::getStripePatternClone()
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

   StripePattern* pattern = stripePattern->clone();

   safeLock.unlock();

   return pattern;
}

/**
 * @param newPattern will be cloned
 */
FhgfsOpsErr DirInode::setStripePattern(StripePattern& newPattern, uint32_t actorUID)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE);

   bool loadSuccess = loadIfNotLoadedUnlocked();
   if (!loadSuccess)
   {
      safeLock.unlock();
      return FhgfsOpsErr_INTERNAL;
   }

   if (actorUID != 0 && statData.getUserID() != actorUID)
   {
      safeLock.unlock();
      return FhgfsOpsErr_PERM;
   }

   StripePattern* oldPattern;

   if (actorUID != 0)
   {
      oldPattern = stripePattern->clone();
      stripePattern->setDefaultNumTargets(newPattern.getDefaultNumTargets());
      stripePattern->setChunkSize(newPattern.getChunkSize());
   }
   else
   {
      oldPattern = this->stripePattern;
      this->stripePattern = newPattern.clone();
   }

   if(!storeUpdatedMetaDataUnlocked() )
   { // failed to update metadata => restore old value
      delete(this->stripePattern); // delete newPattern-clone
      this->stripePattern = oldPattern;

      safeLock.unlock();
      return FhgfsOpsErr_INTERNAL;
   }
   else
   { // sucess
      delete(oldPattern);
   }

   safeLock.unlock();

   return FhgfsOpsErr_SUCCESS;
}

/**
 * Note: See listIncrementalEx for comments; this is just a wrapper for it that doen't retrieve the
 * direntry types.
 */
FhgfsOpsErr DirInode::listIncremental(int64_t serverOffset,
   unsigned maxOutNames, StringList* outNames, int64_t* outNewServerOffset)
{
   ListIncExOutArgs outArgs(outNames, NULL, NULL, NULL, outNewServerOffset);

   return listIncrementalEx(serverOffset, maxOutNames, true, outArgs);
}

/**
 * Note: Returns only a limited number of entries.
 *
 * Note: serverOffset is an internal value and should not be assumed to be just 0, 1, 2, 3, ...;
 * so make sure you use either 0 (at the beginning) or something that has been returned by this
 * method as outNewOffset (similar to telldir/seekdir).
 *
 * Note: You have reached the end of the directory when success is returned and
 * "outNames.size() != maxOutNames".
 *
 * @param serverOffset zero-based offset
 * @param maxOutNames the maximum number of entries that will be added to the outNames
 * @param filterDots true to not return "." and ".." entries.
 * @param outArgs outNewOffset is only valid if return value indicates success.
 */
FhgfsOpsErr DirInode::listIncrementalEx(int64_t serverOffset,
   unsigned maxOutNames, bool filterDots, ListIncExOutArgs& outArgs)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   FhgfsOpsErr listRes = entries.listIncrementalEx(serverOffset, maxOutNames, filterDots, outArgs);

   safeLock.unlock(); // U N L O C K

   return listRes;
}

/**
 * Note: Returns only a limited number of dentry-by-ID file names
 *
 * Note: serverOffset is an internal value and should not be assumed to be just 0, 1, 2, 3, ...;
 * so make sure you use either 0 (at the beginning) or something that has been returned by this
 * method as outNewOffset.
 *
 * Note: this function was written for fsck
 *
 * @param serverOffset zero-based offset
 * @param incrementalOffset zero-based offset; only used if serverOffset is -1
 * @param maxOutNames the maximum number of entries that will be added to the outNames
 * @param outArgs outNewOffset is only valid if return value indicates success, outEntryTypes is
 * not used (NULL), outNames is required.
 */
FhgfsOpsErr DirInode::listIDFilesIncremental(int64_t serverOffset, uint64_t incrementalOffset,
   unsigned maxOutNames, ListIncExOutArgs& outArgs)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   FhgfsOpsErr listRes = entries.listIDFilesIncremental(serverOffset, incrementalOffset,
      maxOutNames, outArgs);

   safeLock.unlock(); // U N L O C K

   return listRes;
}

/**
 * Check whether an entry with the given name exists in this directory.
 */
bool DirInode::exists(std::string entryName)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   bool retVal = entries.exists(entryName);

   safeLock.unlock(); // U N L O C K

   return retVal;
}

/**
 * @param outInodeMetaData might be NULL
 * @return DirEntryType_INVALID if entry does not exist
 */
FhgfsOpsErr DirInode::getEntryData(std::string entryName, EntryInfo* outInfo,
   FileInodeStoreData* outInodeMetaData)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_READ); // L O C K

   FhgfsOpsErr retVal = entries.getEntryData(entryName, outInfo, outInodeMetaData);

   if (retVal == FhgfsOpsErr_SUCCESS && DirEntryType_ISREGULARFILE(outInfo->getEntryType() ) )
   {
      bool inStore = fileStore.isInStore(outInfo->getEntryID() );
      if (inStore)
      {  // hint for the caller not to rely on outInodeMetaData
         retVal = FhgfsOpsErr_DYNAMICATTRIBSOUTDATED;
      }
   }

   safeLock.unlock(); // U N L O C K

   return retVal;
}


/**
 * Create new directory entry in the directory given by DirInode
 *
 * Note: Do not use or delete entry after calling this method
 */
FhgfsOpsErr DirInode::makeDirEntry(DirEntry* entry)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE); // L O C K

   // we always delete the entry from this method
   FhgfsOpsErr mkRes = makeDirEntryUnlocked(entry, true);

   safeLock.unlock(); // U N L O C K

   return mkRes;
}

/**
 * @param deleteEntry  shall we delete entry or does the caller still need it?
 */
FhgfsOpsErr DirInode::makeDirEntryUnlocked(DirEntry* entry, bool deleteEntry)
{
   FhgfsOpsErr mkRes = FhgfsOpsErr_INTERNAL;

   DirEntryType entryType = entry->getEntryType();
   if (unlikely( (!DirEntryType_ISFILE(entryType) && (!DirEntryType_ISDIR(entryType) ) ) ) )
      goto out;

   // load DirInode on demand if required, we need it now
   if (loadIfNotLoadedUnlocked() == false)
   {
      mkRes = FhgfsOpsErr_PATHNOTEXISTS;
      goto out;
   }

   mkRes = this->entries.makeEntry(entry);
   if(mkRes == FhgfsOpsErr_SUCCESS)
   { // entry successfully created

      if (DirEntryType_ISDIR(entryType) )
      {
         // update our own dirInode
         increaseNumSubDirsAndStoreOnDisk();

         // add entry to mirror queue (dirs just have a name and no dentry-by-id link)
         if(getFeatureFlags() & DIRINODE_FEATURE_MIRRORED)
            MetadataMirrorer::addNewDentryStatic(getID(), entry->getName(), "", false,
               getMirrorNodeID() );
      }
      else
      {
         // update our own dirInode
         increaseNumFilesAndStoreOnDisk();

         // add entry to mirror queue (files have name and dentry-by-id hardlinked)
         if(getFeatureFlags() & DIRINODE_FEATURE_MIRRORED)
            MetadataMirrorer::addNewDentryStatic(getID(), entry->getName(), entry->getID(), true,
               getMirrorNodeID() );
      }

   }

out:
   if (deleteEntry)
      delete entry;

   return mkRes;
}

FhgfsOpsErr DirInode::linkFilesInDirUnlocked(std::string fromName, FileInode* fromInode,
   std::string toName)
{
   FhgfsOpsErr linkRes = entries.linkEntryInDir(fromName, toName);

   if(linkRes == FhgfsOpsErr_SUCCESS)
   {
      // ignore the return code, time stamps are not that important
      increaseNumFilesAndStoreOnDisk();

      // add entry to mirror queue
      if(getFeatureFlags() & DIRINODE_FEATURE_MIRRORED)
         MetadataMirrorer::hardlinkInSameDirStatic(getID(), fromName, fromInode, toName,
            getMirrorNodeID() );
   }

   return linkRes;
}


/**
 * Link an existing inode (stored in dir-entry format) to our directory. So add a new dirEntry.
 *
 * Used for example for linking an (unlinked/deleted) file-inode to the disposal dir.
 */
FhgfsOpsErr DirInode::linkFileInodeToDir(std::string& inodePath, std::string &fileName)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE); // L O C K

   // we always delete the entry from this method
   FhgfsOpsErr retVal = linkFileInodeToDirUnlocked(inodePath, fileName);

   safeLock.unlock(); // U N L O C K

   return retVal;
}

FhgfsOpsErr DirInode::linkFileInodeToDirUnlocked(std::string& inodePath, std::string &fileName)
{
   bool loadRes = loadIfNotLoadedUnlocked();
   if (!loadRes)
      return FhgfsOpsErr_PATHNOTEXISTS;

   FhgfsOpsErr retVal = this->entries.linkInodeToDir(inodePath, fileName);

   if (retVal == FhgfsOpsErr_SUCCESS)
      increaseNumFilesAndStoreOnDisk();

   return retVal;
}

FhgfsOpsErr DirInode::removeDir(std::string entryName, DirEntry** outDirEntry)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE); // L O C K

   FhgfsOpsErr rmRes = removeDirUnlocked(entryName, outDirEntry);

   safeLock.unlock(); // U N L O C K

   return rmRes;
}

/**
 * @param outDirEntry is object of the removed dirEntry, maybe NULL of the caller does not need it
 */
FhgfsOpsErr DirInode::removeDirUnlocked(std::string entryName, DirEntry** outDirEntry)
{
   // load DirInode on demand if required, we need it now
   bool loadSuccess = loadIfNotLoadedUnlocked();
   if (!loadSuccess)
      return FhgfsOpsErr_PATHNOTEXISTS;

   FhgfsOpsErr rmRes = entries.removeDir(entryName, outDirEntry);

   if(rmRes == FhgfsOpsErr_SUCCESS)
   {
      if (unlikely (!decreaseNumSubDirsAndStoreOnDisk() ) )
         rmRes = FhgfsOpsErr_INTERNAL;

      // add entry to mirror queue
      if(getFeatureFlags() & DIRINODE_FEATURE_MIRRORED)
         MetadataMirrorer::removeDentryStatic(getID(), entryName, "", getMirrorNodeID() );
   }

   return rmRes;
}

/**
 *
 * @param unlinkEntryName If true do not try to unlink the dentry-name entry, entryName even
 *    might not be set.
 * @param outFile will be set to the unlinked file and the object must then be deleted by the caller
 * (can be NULL if the caller is not interested in the file)
 */
FhgfsOpsErr DirInode::renameDirEntry(std::string fromName, std::string toName,
   DirEntry* overWriteEntry)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE); // L O C K

   FhgfsOpsErr renameRes = renameDirEntryUnlocked(fromName, toName, overWriteEntry);

   safeLock.unlock(); // U N L O C K

   return renameRes;
}


/**
 * Rename an entry in the same directory.
 */
FhgfsOpsErr DirInode::renameDirEntryUnlocked(std::string fromName, std::string toName,
   DirEntry* overWriteEntry)
{
   const char* logContext = "DirInode rename entry";

   FhgfsOpsErr renameRes = entries.renameEntry(fromName, toName);

   if(renameRes == FhgfsOpsErr_SUCCESS)
   {
      if (overWriteEntry)
      {
         if (DirEntryType_ISDIR(overWriteEntry->getEntryType() )  )
            this->numSubdirs--;
         else
            this->numFiles--;
      }

      // ignore the return code, time stamps are not that important
      updateTimeStampsAndStoreToDisk(logContext);

      // add entry to mirror queue
      if(getFeatureFlags() & DIRINODE_FEATURE_MIRRORED)
         MetadataMirrorer::renameDentryInSameDirStatic(getID(), fromName, toName,
            getMirrorNodeID() );
   }

   return renameRes;
}

/**
 * @param inEntry should be provided if available for performance, but may be NULL if
 *    unlinkTypeFlags == DirEntry_UNLINK_FILENAME_ONLY or unlinkTypeFlags == DirEntry_UNLINK_ID_AND_FILENAME
 * @param unlinkEntryName If false do not try to unlink the dentry-name entry, entryName even
 *    might not be set (but inEntry must be set in this case).
 * @param outFile will be set to the unlinked file and the object must then be deleted by the caller
 * (can be NULL if the caller is not interested in the file)
 */
FhgfsOpsErr DirInode::unlinkDirEntry(std::string entryName, DirEntry* entry,
   unsigned unlinkTypeFlags)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE); // L O C K

   FhgfsOpsErr unlinkRes = unlinkDirEntryUnlocked(entryName, entry, unlinkTypeFlags);

   safeLock.unlock(); // U N L O C K

   return unlinkRes;
}

/**
 * @inEntry should be provided if available for performance reasons, but may be NULL (in which case
 * unlinkTypeFlags must be != DirEntry_UNLINK_ID_ONLY)
 */
FhgfsOpsErr DirInode::unlinkDirEntryUnlocked(std::string entryName, DirEntry* inEntry,
   unsigned unlinkTypeFlags)
{
   const char* logContext = "DirInode unlink entry";

   // load DirInode on demand if required, we need it now
   bool loadSuccess = loadIfNotLoadedUnlocked();
   if (!loadSuccess)
      return FhgfsOpsErr_PATHNOTEXISTS;

   DirEntry* entry;
   DirEntry loadedEntry(entryName);

   if (!inEntry)
   {
      if (unlikely(!(unlinkTypeFlags & DirEntry_UNLINK_FILENAME) ) )
      {
         LogContext(logContext).logErr("Bug: inEntry missing!");
         LogContext(logContext).logBacktrace();
         return FhgfsOpsErr_INTERNAL;
      }

      bool getRes = getDentryUnlocked(entryName, loadedEntry);
      if (!getRes)
         return FhgfsOpsErr_PATHNOTEXISTS;

         entry = &loadedEntry;
   }
   else
      entry = inEntry;

   FhgfsOpsErr unlinkRes = this->entries.unlinkDirEntry(entryName, entry, unlinkTypeFlags);

   if (unlinkRes != FhgfsOpsErr_SUCCESS)
      goto out;

   if (unlinkTypeFlags & DirEntry_UNLINK_FILENAME)
   {
      bool decRes;

      if (DirEntryType_ISDIR(entry->getEntryType() ) )
         decRes = decreaseNumSubDirsAndStoreOnDisk();
      else
         decRes = decreaseNumFilesAndStoreOnDisk();

      if (unlikely(!decRes) )
      {
         LogContext(logContext).logErr("Unlink succeeded, but failed to store updated DirInode"
            " DirID: " + this->getID() );

         // except of this warning we ignore this error, as the unlink otherwise succeeded
      }
   }

   // add entry to mirror queue
   if(getFeatureFlags() & DIRINODE_FEATURE_MIRRORED)
   {
      std::string remoteUnlinkName;
      std::string remoteEntryID;

      if (unlinkTypeFlags & DirEntry_UNLINK_FILENAME)
         remoteUnlinkName = entryName;
      else
      {
         // is "" by default
      }

      if (unlinkTypeFlags & DirEntry_UNLINK_ID)
         remoteEntryID = entry->getEntryID();
      {
         // is "" by default
      }

      MetadataMirrorer::removeDentryStatic(getID(), remoteUnlinkName, remoteEntryID,
         getMirrorNodeID() );
   }

out:
   return unlinkRes;
}

FhgfsOpsErr DirInode::refreshMetaInfo()
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE); // L O C K

   FhgfsOpsErr retVal = refreshMetaInfoUnlocked();

   safeLock.unlock(); // U N L O C K

   return retVal;
}

/**
 * Re-computes number of subentries.
 *
 * Note: Intended to be called only when you have reason to believe that the stored metadata is
 * not correct (eg after an upgrade that introduced new metadata information).
 */
FhgfsOpsErr DirInode::refreshMetaInfoUnlocked()
{

   // load DirInode on demand if required, we need it now
   bool loadSuccess = loadIfNotLoadedUnlocked();
   if (!loadSuccess)
      return FhgfsOpsErr_PATHNOTEXISTS;

   FhgfsOpsErr retVal = refreshSubentryCountUnlocked();
   if(retVal == FhgfsOpsErr_SUCCESS)
   {
      if(!storeUpdatedMetaDataUnlocked() )
         retVal = FhgfsOpsErr_INTERNAL;
   }

   return retVal;
}

/**
 * Reads all stored entry links (via entries::listInc() ) to re-compute number of subentries.
 *
 * Note: Intended to be called only when you have reason to believe that the stored metadata is
 * not correct (eg after an upgrade that introduced new metadata information).
 * Note: Does not update persistent metadata.
 * Note: Unlocked, so hold the write lock when calling this.
 */
FhgfsOpsErr DirInode::refreshSubentryCountUnlocked()
{
   const char* logContext = "Directory (refresh entry count)";

   FhgfsOpsErr retVal = FhgfsOpsErr_SUCCESS;
   unsigned currentNumSubdirs = 0;
   unsigned currentNumFiles = 0;

   unsigned maxOutNames = 128;
   StringList names;
   UInt8List entryTypes;
   int64_t serverOffset = 0;
   size_t numOutEntries; // to avoid inefficient calls to list::size.

   // query contents
   ListIncExOutArgs outArgs(&names, &entryTypes, NULL, NULL, &serverOffset);

   do
   {
      names.clear();
      entryTypes.clear();

      FhgfsOpsErr listRes = entries.listIncrementalEx(serverOffset, maxOutNames, true, outArgs);
      if(listRes != FhgfsOpsErr_SUCCESS)
      {
         LogContext(logContext).logErr(
            std::string("Unable to fetch dir contents for dirID: ") + id);
         retVal = listRes;
         break;
      }

      // walk over returned entries

      StringListConstIter namesIter = names.begin();
      UInt8ListConstIter typesIter = entryTypes.begin();
      numOutEntries = 0;

      for( /* using namesIter, typesIter, numOutEntries */;
          (namesIter != names.end() ) && (typesIter != entryTypes.end() );
          namesIter++, typesIter++, numOutEntries++)
      {
         if(DirEntryType_ISDIR(*typesIter) )
            currentNumSubdirs++;
         else
         if(DirEntryType_ISFILE(*typesIter) )
            currentNumFiles++;
         else
         { // invalid entry => log error, but continue
            LogContext(logContext).logErr(std::string("Unable to read entry type for name: '") +
               *namesIter + "' "  "(dirID: " + id + ")");
         }
      }

   } while(numOutEntries == maxOutNames);

   if(retVal == FhgfsOpsErr_SUCCESS)
   { // update in-memory counters (but we leave persistent updates to the caller)
      this->numSubdirs = currentNumSubdirs;
      this->numFiles = currentNumFiles;
   }

   return retVal;
}


/*
 * Note: Current object state is used for the serialization.
 */
FhgfsOpsErr DirInode::storeInitialMetaData()
{
   FhgfsOpsErr dirRes = DirEntryStore::mkDentryStoreDir(this->id);
   if(dirRes != FhgfsOpsErr_SUCCESS)
      return dirRes;

   FhgfsOpsErr fileRes = storeInitialMetaDataInode();
   if(unlikely(fileRes != FhgfsOpsErr_SUCCESS) )
   {
      if (unlikely(fileRes == FhgfsOpsErr_EXISTS) )
      {
         // there must have been some kind of race as dirRes was successful
         fileRes = FhgfsOpsErr_SUCCESS;
      }
      else
         DirEntryStore::rmDirEntryStoreDir(this->id); // remove dir
   }

   // add entry to mirror queue
   if( (fileRes == FhgfsOpsErr_SUCCESS) && (getFeatureFlags() & DIRINODE_FEATURE_MIRRORED) )
      MetadataMirrorer::addNewInodeStatic(this->id, this->mirrorNodeID);

   return fileRes;
}

FhgfsOpsErr DirInode::storeInitialMetaData(const CharVector& defaultACLXAttr,
         const CharVector& accessACLXAttr)
{
   FhgfsOpsErr res = storeInitialMetaData();

   if (res != FhgfsOpsErr_SUCCESS)
      return res;

   if (!defaultACLXAttr.empty() )
      res = setXAttr(NULL, MsgHelperXAttr::CURRENT_DIR_FILENAME, PosixACL::defaultACLXAttrName,
         defaultACLXAttr, 0);

   if (res != FhgfsOpsErr_SUCCESS)
      return res;

   if (!accessACLXAttr.empty() )
      res = setXAttr(NULL, MsgHelperXAttr::CURRENT_DIR_FILENAME, PosixACL::accessACLXAttrName,
         accessACLXAttr, 0);

   return res;
}

/*
 * Creates the initial metadata inode for this directory.
 *
 * Note: Current object state is used for the serialization.
 */
FhgfsOpsErr DirInode::storeInitialMetaDataInode()
{
   const char* logContext = "Directory (store initial metadata file)";

   App* app = Program::getApp();
   std::string metaFilename = MetaStorageTk::getMetaInodePath(
      app->getInodesPath()->getPathAsStrConst(), id);

   FhgfsOpsErr retVal = FhgfsOpsErr_SUCCESS;
   bool useXAttrs = app->getConfig()->getStoreUseExtendedAttribs();

   char buf[META_SERBUF_SIZE];
   unsigned bufLen;

   // create file

   int openFlags = O_CREAT|O_EXCL|O_WRONLY;
   mode_t openMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

   int fd = open(metaFilename.c_str(), openFlags, openMode);
   if(fd == -1)
   { // error
      if(errno == EEXIST)
         retVal = FhgfsOpsErr_EXISTS;
      else
      {
         LogContext(logContext).logErr("Unable to create dir metadata inode " + metaFilename +
            ". " + "SysErr: " + System::getErrString() );
         retVal = FhgfsOpsErr_INTERNAL;
      }

      goto error_donothing;
   }

   // alloc buf and serialize

   bufLen = DiskMetaData::serializeDirInode(buf, this);

   // write data to file

   if(useXAttrs)
   { // extended attribute
      int setRes = fsetxattr(fd, META_XATTR_NAME, buf, bufLen, 0);

      if(unlikely(setRes == -1) )
      { // error
         if(errno == ENOTSUP)
            LogContext(logContext).logErr("Unable to store directory xattr metadata: " +
               metaFilename + ". " +
               "Did you enable extended attributes (user_xattr) on the underlying file system?");
         else
            LogContext(logContext).logErr("Unable to store directory xattr metadata: " +
               metaFilename + ". " + "SysErr: " + System::getErrString() );

         retVal = FhgfsOpsErr_INTERNAL;

         goto error_closefile;
      }
   }
   else
   { // normal file content
      ssize_t writeRes = write(fd, buf, bufLen);

      if(writeRes != (ssize_t)bufLen)
      {
         LogContext(logContext).logErr("Unable to store directory metadata: " + metaFilename +
            ". " + "SysErr: " + System::getErrString() );
         retVal = FhgfsOpsErr_INTERNAL;

         goto error_closefile;
      }
   }

   close(fd);

   LOG_DEBUG(logContext, Log_DEBUG, "Directory metadata inode stored: " + metaFilename);

   return retVal;


   // error compensation
error_closefile:
   close(fd);
   unlink(metaFilename.c_str() );

error_donothing:

   return retVal;
}

/**
 * Note: Wrapper/chooser for storeUpdatedMetaDataBufAsXAttr/Contents.
 *
 * @param buf the serialized object state that is to be stored
 */
bool DirInode::storeUpdatedMetaDataBuf(char* buf, unsigned bufLen)
{
   bool useXAttrs = Program::getApp()->getConfig()->getStoreUseExtendedAttribs();

   if(useXAttrs)
      return storeUpdatedMetaDataBufAsXAttr(buf, bufLen);

   return storeUpdatedMetaDataBufAsContents(buf, bufLen);
}

/**
 * Note: Don't call this directly, use the wrapper storeUpdatedMetaDataBuf().
 *
 * @param buf the serialized object state that is to be stored
 */
bool DirInode::storeUpdatedMetaDataBufAsXAttr(char* buf, unsigned bufLen)
{
   const char* logContext = "Directory (store updated xattr metadata)";

   App* app = Program::getApp();
   std::string metaFilename = MetaStorageTk::getMetaInodePath(
      app->getInodesPath()->getPathAsStrConst(), id);

   // write data to file

   int setRes = setxattr(metaFilename.c_str(), META_XATTR_NAME, buf, bufLen, 0);

   if(unlikely(setRes == -1) )
   { // error
      LogContext(logContext).logErr("Unable to write directory metadata update: " +
         metaFilename + ". " + "SysErr: " + System::getErrString() );

      return false;
   }

   LOG_DEBUG(logContext, 4, "Directory metadata update stored: " + id);

   return true;
}

/**
 * Stores the update to a sparate file first and then renames it.
 *
 * Note: Don't call this directly, use the wrapper storeUpdatedMetaDataBuf().
 *
 * @param buf the serialized object state that is to be stored
 */
bool DirInode::storeUpdatedMetaDataBufAsContents(char* buf, unsigned bufLen)
{
   const char* logContext = "Directory (store updated metadata)";

   App* app = Program::getApp();
   std::string metaFilename = MetaStorageTk::getMetaInodePath(
      app->getInodesPath()->getPathAsStrConst(), id);
   std::string metaUpdateFilename(metaFilename + META_UPDATE_EXT_STR);

   ssize_t writeRes;
   int renameRes;

   // open file (create it, but not O_EXCL because a former update could have failed)
   int openFlags = O_CREAT|O_TRUNC|O_WRONLY;

   int fd = open(metaUpdateFilename.c_str(), openFlags, 0644);
   if(fd == -1)
   { // error
      if(errno == ENOSPC)
      { // no free space => try again with update in-place
         LogContext(logContext).log(Log_DEBUG, "No space left to create update file. Trying update "
            "in-place: " + metaUpdateFilename + ". " + "SysErr: " + System::getErrString() );

         return storeUpdatedMetaDataBufAsContentsInPlace(buf, bufLen);
      }

      LogContext(logContext).logErr("Unable to create directory metadata update file: " +
         metaUpdateFilename + ". " + "SysErr: " + System::getErrString() );

      return false;
   }

   // metafile created => store meta data
   writeRes = write(fd, buf, bufLen);
   if(writeRes != (ssize_t)bufLen)
   {
      if( (writeRes >= 0) || (errno == ENOSPC) )
      { // no free space => try again with update in-place
         LogContext(logContext).log(Log_DEBUG, "No space left to write update file. Trying update "
            "in-place: " + metaUpdateFilename + ". " + "SysErr: " + System::getErrString() );

         close(fd);
         unlink(metaUpdateFilename.c_str() );

         return storeUpdatedMetaDataBufAsContentsInPlace(buf, bufLen);
      }

      LogContext(logContext).logErr("Unable to store directory metadata update: " + metaFilename +
         ". " + "SysErr: " + System::getErrString() );

      goto error_closefile;
   }

   close(fd);

   renameRes = rename(metaUpdateFilename.c_str(), metaFilename.c_str() );
   if(renameRes == -1)
   {
      LogContext(logContext).logErr("Unable to replace old directory metadata file: " +
         metaFilename + ". " + "SysErr: " + System::getErrString() );

      goto error_unlink;
   }

   LOG_DEBUG(logContext, 4, "Directory metadata update stored: " + id);

   return true;


   // error compensation
error_closefile:
   close(fd);

error_unlink:
   unlink(metaUpdateFilename.c_str() );

   return false;
}

/**
 * Stores the update directly to the current metadata file (instead of creating a separate file
 * first and renaming it).
 *
 * Note: Don't call this directly, it is automatically called by storeUpdatedMetaDataBufAsContents()
 * when necessary.
 *
 * @param buf the serialized object state that is to be stored
 */
bool DirInode::storeUpdatedMetaDataBufAsContentsInPlace(char* buf, unsigned bufLen)
{
   const char* logContext = "Directory (store updated metadata in-place)";

   App* app = Program::getApp();
   std::string metaFilename = MetaStorageTk::getMetaInodePath(
      app->getInodesPath()->getPathAsStrConst(), id);

   int fallocRes;
   ssize_t writeRes;
   int truncRes;

   // open file (create it, but not O_EXCL because a former update could have failed)
   int openFlags = O_CREAT|O_WRONLY;

   int fd = open(metaFilename.c_str(), openFlags, 0644);
   if(fd == -1)
   { // error
      LogContext(logContext).logErr("Unable to open directory metadata file: " +
         metaFilename + ". " + "SysErr: " + System::getErrString() );

      return false;
   }

   // make sure we have enough room to write our update
   fallocRes = posix_fallocate(fd, 0, bufLen); // (note: posix_fallocate does not set errno)
   if(fallocRes == EBADF)
   { // special case for XFS bug
      struct stat statBuf;
      int statRes = fstat(fd, &statBuf);

      if (statRes == -1)
      {
         LogContext(logContext).log(Log_WARNING, "Unexpected error: fstat() failed with SysErr: "
            + System::getErrString(errno));
         goto error_closefile;
      }

      if (statBuf.st_size < bufLen)
      {
         LogContext(logContext).log(Log_WARNING, "File space allocation ("
            + StringTk::intToStr(bufLen)  + ") for metadata update failed: " + metaFilename + ". " +
            "SysErr: " + System::getErrString(fallocRes) + " "
            "statRes: " + StringTk::intToStr(statRes) + " "
            "oldSize: " + StringTk::intToStr(statBuf.st_size));
         goto error_closefile;
      }
      else
      { // // XFS bug! We only return an error if statBuf.st_size < bufLen. Ingore fallocRes then
         LOG_DEBUG(logContext, Log_SPAM, "Ignoring kernel file system bug: "
            "posix_fallocate() failed for len < filesize");
      }
   }
   else
   if (fallocRes != 0)
   { // default error handling if posix_fallocate() failed
      LogContext(logContext).log(Log_WARNING, "File space allocation ("
         + StringTk::intToStr(bufLen)  + ") for metadata update failed: " +  metaFilename + ". " +
         "SysErr: " + System::getErrString(fallocRes));
      goto error_closefile;
   }

   // metafile created => store meta data
   writeRes = write(fd, buf, bufLen);
   if(writeRes != (ssize_t)bufLen)
   {
      LogContext(logContext).logErr("Unable to store directory metadata update: " + metaFilename +
         ". " + "SysErr: " + System::getErrString() );

      goto error_closefile;
   }

   // truncate in case the update lead to a smaller file size
   truncRes = ftruncate(fd, bufLen);
   if(truncRes == -1)
   { // ignore trunc errors
      LogContext(logContext).log(Log_WARNING, "Unable to truncate metadata file (strange, but "
         "proceeding anyways): " + metaFilename + ". " + "SysErr: " + System::getErrString() );
   }


   close(fd);

   LOG_DEBUG(logContext, 4, "Directory metadata in-place update stored: " + id);

   return true;


   // error compensation
error_closefile:
   close(fd);

   return false;
}


bool DirInode::storeUpdatedMetaDataUnlocked()
{
   std::string logContext = "Write Meta Inode";

   if (unlikely(!this->isLoaded) )
   {
      LogContext(logContext).logErr("Bug: Inode data not loaded yet.");
      LogContext(logContext).logBacktrace();

      return false;
   }

   #ifdef DEBUG_MUTEX_LOCKING
      if (unlikely(!this->rwlock.isRWLocked() ) )
      {
         LogContext(logContext).logErr("Bug: Inode not rw-locked.");
         LogContext(logContext).logBacktrace();

         return false;
      }
   #endif

   char buf[META_SERBUF_SIZE];

   unsigned bufLen = DiskMetaData::serializeDirInode(buf, this);

   bool saveRes = storeUpdatedMetaDataBuf(buf, bufLen);


   // add entry to mirror queue
   if( (getFeatureFlags() & DIRINODE_FEATURE_MIRRORED) && saveRes)
      MetadataMirrorer::addNewInodeStatic(this->id, this->mirrorNodeID);

   return saveRes;
}

/**
 * Note: Assumes that the caller already verified that the directory is empty
 */
bool DirInode::removeStoredMetaData(std::string id)
{
   bool dirRes = DirEntryStore::rmDirEntryStoreDir(id);
   if(!dirRes)
      return dirRes;

   return removeStoredMetaDataFile(id);
}


/**
 * Note: Assumes that the caller already verified that the directory is empty
 */
bool DirInode::removeStoredMetaDataFile(std::string& id)
{
   const char* logContext = "Directory (remove stored metadata)";

   App* app = Program::getApp();
   std::string metaFilename = MetaStorageTk::getMetaInodePath(
      app->getInodesPath()->getPathAsStrConst(), id);

   // delete metadata file

   int unlinkRes = unlink(metaFilename.c_str() );
   if(unlinkRes == -1)
   { // error
      LogContext(logContext).logErr("Unable to delete directory metadata file: " + metaFilename +
         ". " + "SysErr: " + System::getErrString() );

      if (errno != ENOENT)
         return false;
   }

   LOG_DEBUG(logContext, 4, "Directory metadata deleted: " + metaFilename);

   return true;
}

bool DirInode::loadIfNotLoaded(void)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE);

   bool loadRes = loadIfNotLoadedUnlocked();

   safeLock.unlock();

   return loadRes;
}


/**
 * Load the DirInode from disk if it was notalready loaded before.
 *
 * @return true if loading not required or loading successfully, false if loading from disk failed.
 */
bool DirInode::loadIfNotLoadedUnlocked()
{
   if (!this->isLoaded)
   { // So the object was already created before without loading the inode from disk, do that now.
      bool loadSuccess = loadFromFile();
      if (!loadSuccess)
      {
         const char* logContext = "Loading DirInode on demand";
         std::string msg = "Loading DirInode failed dir-ID: ";
         LOG_DEBUG_CONTEXT(LogContext(logContext), Log_DEBUG, msg + this->id);
         IGNORE_UNUSED_VARIABLE(logContext);

         return false;
      }
   }

   return true;
}

/**
 * Note: Wrapper/chooser for loadFromFileXAttr/Contents.
 */
bool DirInode::loadFromFile()
{
   bool useXAttrs = Program::getApp()->getConfig()->getStoreUseExtendedAttribs();

   bool loadRes;
   if(useXAttrs)
      loadRes = loadFromFileXAttr();
   else
      loadRes = loadFromFileContents();

   if (loadRes)
      this->isLoaded = true;

   return loadRes;
}


/**
 * Note: Don't call this directly, use the wrapper loadFromFile().
 */
bool DirInode::loadFromFileXAttr()
{
   const char* logContext = "Directory (load from xattr file)";

   App* app = Program::getApp();
   std::string inodePath = MetaStorageTk::getMetaInodePath(
      app->getInodesPath()->getPathAsStrConst(), this->id);

   bool retVal = false;

   char buf[META_SERBUF_SIZE];

   ssize_t getRes = getxattr(inodePath.c_str(), META_XATTR_NAME, buf, META_SERBUF_SIZE);
   if(getRes > 0)
   { // we got something => deserialize it
      bool deserRes = DiskMetaData::deserializeDirInode(buf, this);
      if(unlikely(!deserRes) )
      { // deserialization failed
         LogContext(logContext).logErr("Unable to deserialize metadata in file: " + inodePath);
         goto error_exit;
      }

      retVal = true;
   }
   else
   if( (getRes == -1) && (errno == ENOENT) )
   { // file not exists
      LOG_DEBUG(logContext, Log_DEBUG, "Metadata file not exists: " +
         inodePath + ". " + "SysErr: " + System::getErrString() );
   }
   else
   { // unhandled error
      LogContext(logContext).logErr("Unable to open/read xattr metadata file: " +
         inodePath + ". " + "SysErr: " + System::getErrString() );
   }


error_exit:

   return retVal;
}

/**
 * Note: Don't call this directly, use the wrapper loadFromFile().
 */
bool DirInode::loadFromFileContents()
{
   const char* logContext = "Directory (load from file)";

   App* app = Program::getApp();
   std::string inodePath = MetaStorageTk::getMetaInodePath(
      app->getInodesPath()->getPathAsStrConst(), this->id);

   bool retVal = false;

   int openFlags = O_NOATIME | O_RDONLY;

   int fd = open(inodePath.c_str(), openFlags, 0);
   if(fd == -1)
   { // open failed
      if(errno != ENOENT)
         LogContext(logContext).logErr("Unable to open metadata file: " + inodePath +
            ". " + "SysErr: " + System::getErrString() );

      return false;
   }

   char buf[META_SERBUF_SIZE];
   int readRes = read(fd, buf, META_SERBUF_SIZE);
   if(readRes <= 0)
   { // reading failed
      LogContext(logContext).logErr("Unable to read metadata file: " + inodePath + ". " +
         "SysErr: " + System::getErrString() );
   }
   else
   {
      bool deserRes = DiskMetaData::deserializeDirInode(buf, this);
      if(!deserRes)
      { // deserialization failed
         LogContext(logContext).logErr("Unable to deserialize metadata in file: " + inodePath);
      }
      else
      { // success
         retVal = true;
      }
   }

   close(fd);

   return retVal;
}


DirInode* DirInode::createFromFile(std::string id)
{
   DirInode* newDir = new DirInode(id);

   bool loadRes = newDir->loadFromFile();
   if(!loadRes)
   {
      delete(newDir);
      return NULL;
   }

   newDir->entries.setParentID(id);

   return newDir;
}

/**
 * Get directory stat information
 *
 * @param outParentNodeID may NULL if the caller is not interested
 */
FhgfsOpsErr DirInode::getStatData(StatData& outStatData,
   uint16_t* outParentNodeID, std::string* outParentEntryID)
{
   // note: keep in mind that correct nlink count (2+numSubdirs) is absolutely required to not break
      // the "find" command, which uses optimizations based on nlink (see "find -noleaf" for
      // details). The other choice would be to set nlink=1, which find would reduce to -1
      // as is substracts 2 for "." and "..". -1 = max(unsigned) and makes find to check all
      // possible subdirs. But as we have the correct nlink anyway, we can provide it to find
      // and allow it to use non-posix optimizations.

   const char* logContext = "DirInode::getStatData";

   SafeRWLock safeLock(&rwlock, SafeRWLock_READ);

   this->statData.setNumHardLinks(2 + numSubdirs); // +2 by definition (for "." and "<name>")

   this->statData.setFileSize(numSubdirs + numFiles); // just because we got those values anyway

   outStatData = this->statData;

   if (outParentNodeID)
   {
      *outParentNodeID = this->parentNodeID;

      #ifdef BEEGFS_DEBUG
         if (!outParentEntryID)
         {
            LogContext(logContext).logErr("Bug: outParentEntryID is unexpectedly NULL");
            LogContext(logContext).logBacktrace();
         }
      #endif
      IGNORE_UNUSED_VARIABLE(logContext);

      *outParentEntryID = this->parentDirID;
   }

   safeLock.unlock();

   return FhgfsOpsErr_SUCCESS;
}

/*
 * only used for testing at the moment
 */
void DirInode::setStatData(StatData& statData)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE); // L O C K

   this->statData = statData;

   this->numSubdirs = statData.getNumHardlinks() - 2;
   this->numFiles = statData.getFileSize() - this->numSubdirs;

   storeUpdatedMetaDataUnlocked();

   safeLock.unlock(); // U N L O C K
}

/**
 * @param validAttribs SETATTR_CHANGE_...-Flags, might be 0 if only attribChangeTimeSecs
 *    shall be updated
 * @attribs  new attributes, but might be NULL if validAttribs == 0
 */
bool DirInode::setAttrData(int validAttribs, SettableFileAttribs* attribs)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE); // L O C K

   bool retVal = setAttrDataUnlocked(validAttribs, attribs);

   safeLock.unlock(); // U N L O C K

   return retVal;
}

/**
 * See setAttrData() for documentation.
 */
bool DirInode::setAttrDataUnlocked(int validAttribs, SettableFileAttribs* attribs)
{
   bool success = true;


   // load DirInode on demand if required, we need it now
   bool loadSuccess = loadIfNotLoadedUnlocked();
   if (!loadSuccess)
      return FhgfsOpsErr_PATHNOTEXISTS;

   // save old attribs
   SettableFileAttribs oldAttribs;
   oldAttribs = *(this->statData.getSettableFileAttribs() );

    this->statData.setAttribChangeTimeSecs(TimeAbs().getTimeval()->tv_sec);

   if(validAttribs)
   { // apply new attribs wrt flags...

      if(validAttribs & SETATTR_CHANGE_MODE)
         this->statData.setMode(attribs->mode);

      if(validAttribs & SETATTR_CHANGE_MODIFICATIONTIME)
      {
         /* only static value update required for storeUpdatedMetaDataUnlocked() */
         this->statData.setModificationTimeSecs(attribs->modificationTimeSecs);
      }

      if(validAttribs & SETATTR_CHANGE_LASTACCESSTIME)
      {
         /* only static value update required for storeUpdatedMetaDataUnlocked() */
         this->statData.setLastAccessTimeSecs(attribs->lastAccessTimeSecs);
      }

      if(validAttribs & SETATTR_CHANGE_USERID)
         this->statData.setUserID(attribs->userID);

      if(validAttribs & SETATTR_CHANGE_GROUPID)
         this->statData.setGroupID(attribs->groupID);
   }

   bool storeRes = storeUpdatedMetaDataUnlocked();
   if(!storeRes)
   { // failed to update metadata on disk => restore old values
      this->statData.setSettableFileAttribs(oldAttribs);

      success = false;
   }

   return success;
}

/**
 * Set/Update parent information (from the given entryInfo)
 */
FhgfsOpsErr DirInode::setDirParentAndChangeTime(EntryInfo* entryInfo, uint16_t parentNodeID)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE); // L O C K

   FhgfsOpsErr retVal = setDirParentAndChangeTimeUnlocked(entryInfo, parentNodeID);

   safeLock.unlock(); // U N L O C K

   return retVal;
}

/**
 * See setParentEntryID() for documentation.
 */
FhgfsOpsErr DirInode::setDirParentAndChangeTimeUnlocked(EntryInfo* entryInfo, uint16_t parentNodeID)
{
   const char* logContext = "DirInode::setDirParentAndChangeTimeUnlocked";

   // load DirInode on demand if required, we need it now
   bool loadSuccess = loadIfNotLoadedUnlocked();
   if (!loadSuccess)
      return FhgfsOpsErr_PATHNOTEXISTS;

#ifdef BEEGFS_DEBUG
   LogContext(logContext).log(Log_DEBUG, "DirInode" + this->getID()   + " "
      "newParentDirID: "  + entryInfo->getParentEntryID()             + " "
      "newParentNodeID: " + StringTk::uintToStr(parentNodeID)         + ".");
#endif
   IGNORE_UNUSED_VARIABLE(logContext);

   this->parentDirID = entryInfo->getParentEntryID();
   this->parentNodeID = parentNodeID;

   // updates change time stamps and saves to disk
   bool attrRes = setAttrDataUnlocked(0, NULL);

   if (unlikely(!attrRes) )
      return FhgfsOpsErr_INTERNAL;

   return FhgfsOpsErr_SUCCESS;
}

/**
 * The "checked" suffix means this only sets the mirrorNodeID if it was unset before.
 *
 * @return FhgfsOpsErr_INVAL if mirror was already set
 */
FhgfsOpsErr DirInode::setAndStoreMirrorNodeIDChecked(uint16_t mirrorNodeID)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE); // L O C K

   FhgfsOpsErr retVal = FhgfsOpsErr_SUCCESS;
   bool loadSuccess;
   bool storeRes;

   loadSuccess = loadIfNotLoadedUnlocked();
   if(!loadSuccess)
   {
      retVal = FhgfsOpsErr_PATHNOTEXISTS;
      goto unlock_and_exit;
   }

   // make sure mirrorNodeID is not set yet
   if(this->mirrorNodeID)
   { // mirror already set
      retVal = FhgfsOpsErr_INVAL;
      goto unlock_and_exit;
   }

   // set new mirror
   setMirrorNodeIDUnlocked(mirrorNodeID);

   // store changes on disk
   storeRes = storeUpdatedMetaDataUnlocked();
   if(!storeRes)
   { // failed to update metadata on disk => restore old values
      this->featureFlags &= ~DIRINODE_FEATURE_MIRRORED;
      this->mirrorNodeID = 0;

      retVal = FhgfsOpsErr_SAVEERROR;
      goto unlock_and_exit;
   }


unlock_and_exit:
   safeLock.unlock(); // U N L O C K

   return retVal;
}

FhgfsOpsErr DirInode::listXAttr(const std::string& fileName, StringVector& outAttrList)
{
   const char* logContext = "DirInode listXAttr";
   std::string dentryPath = entries.getDirEntryPath() + "/" + fileName;
   FhgfsOpsErr res;

   ssize_t size = listxattr(dentryPath.c_str(), NULL, 0); // get size of raw list

   if(size < 0)
   {
      std::string errStr(strerror(errno) );
      LogContext(logContext).logErr("listxattr failed on dentry: " + dentryPath +
            " error: " + errStr);
      return FhgfsOpsErr_INTERNAL;
   }

   char* xAttrRawList = static_cast<char*>(malloc(size) );

   if(!xAttrRawList)
      return FhgfsOpsErr_INTERNAL;

   size = listxattr(dentryPath.c_str(), xAttrRawList, size); // get actual raw list

   if(size < 0)
   {
      int err = errno;

      switch(err)
      {
         case ERANGE:
         case E2BIG:
            res = FhgfsOpsErr_RANGE;
            break;

         case ENOENT:
            res = FhgfsOpsErr_PATHNOTEXISTS;
            break;

         default: // don't forward other errors to the client, but log them on the server
            LogContext(logContext).logErr("failed on dentry: " + dentryPath +
                  " - " + std::string(strerror(err) ) + " on metadata directory.");
            res = FhgfsOpsErr_INTERNAL;
            break;
      }
   }
   else
   {
      StringTk::explode(std::string(xAttrRawList, size), '\0', &outAttrList);

      res = FhgfsOpsErr_SUCCESS;
   }

   free(xAttrRawList);
   return res;
}

/**
 * Get an extended attribute by name.
 * @param outValue If size = 0, unchanged.
 *                 otherwise, outValue is resized to @size, and filled with the contents of the
 *                 xattr.
 * @param inOutSize If inOutSize == 0, then it will be set to the size needed for the buffer,
                    otherwise, it specifies what size the buffer should be, and the buffer will be
                    resized to that size (excess data will be discarded if size is too small).
                    In failure case, will be set to negative error code.
 * @returns FhgfsOpsErr_RANGE if size is smaller than xattr, FhgfsOpsErr_NODATA if there is no
 *          xattr by that name, FhgfsOpsErr_INTERNAL on any other errors, FhfgfsOpsErr_SUCCESS
 *          on success.
 */
FhgfsOpsErr DirInode::getXAttr(const std::string& fileName, const std::string& xAttrName,
      CharVector& outValue, ssize_t& inOutSize)
{
   const char* logContext = "DirInode getXAttr";
   std::string dentryPath = entries.getDirEntryPath() + "/" + fileName;
   int err;

   outValue.resize(inOutSize);
   inOutSize = getxattr(dentryPath.c_str(), xAttrName.c_str(), &outValue.front(), inOutSize);

   if(inOutSize < 0)
   {
      err = errno;
      outValue.clear();

      switch(err)
      {
         case ENOATTR:
            return FhgfsOpsErr_NODATA;

         case ERANGE:
         case E2BIG:
            return FhgfsOpsErr_RANGE;

         case ENOENT:
            return FhgfsOpsErr_PATHNOTEXISTS;

         default: // don't forward other errors to the client, but log them on the server
            LogContext(logContext).logErr("failed on dentry: " + dentryPath +
                  " - " + std::string(strerror(err) ) + " on metadata directory.");
            return FhgfsOpsErr_INTERNAL;
      }
   }
   else
   {
      outValue.resize(inOutSize);
      // onOutSize is already set to the correct size, so we just need to return success here.
      return FhgfsOpsErr_SUCCESS;
   }
}

FhgfsOpsErr DirInode::removeXAttr(EntryInfo* entryInfo, const std::string& fileName,
   const std::string& xAttrName)
{
   const char* logContext = "DirInode removeXAttr";
   std::string dentryPath = entries.getDirEntryPath() + "/" + fileName;

   int res = removexattr(dentryPath.c_str(), xAttrName.c_str() );

   if (res == 0 && fileName == MsgHelperXAttr::CURRENT_DIR_FILENAME)
   {
      rwlock.writeLock();
      updateTimeStampsAndStoreToDisk(__func__);
      rwlock.unlock();
   }

   if (res == 0 && fileName != MsgHelperXAttr::CURRENT_DIR_FILENAME)
   {
      MetaStore* metaStore = Program::getApp()->getMetaStore();

      FileInode* inode = metaStore->referenceFile(entryInfo);
      if (inode)
      {
         inode->updateInodeChangeTime(entryInfo);
         metaStore->releaseFile(getID(), inode);
      }
   }

   if(res != 0)
   {
      int err = errno;

      switch(err)
      {
         case ENOATTR:
            return FhgfsOpsErr_NODATA;

         case ERANGE:
         case E2BIG:
            return FhgfsOpsErr_RANGE;

         case ENOENT:
            return FhgfsOpsErr_PATHNOTEXISTS;

         default: // don't forward other errors to the client, but log them on the server
            LogContext(logContext).logErr("failed on dentry: " + dentryPath +
                  " - " + std::string(strerror(err) ) + " on metadata directory.");
            return FhgfsOpsErr_INTERNAL;
      }
   }
   else
      return FhgfsOpsErr_SUCCESS;
}

FhgfsOpsErr DirInode::setXAttr(EntryInfo* entryInfo, const std::string& fileName,
   const std::string& xAttrName, const CharVector& xAttrValue, int flags, bool updateTimestamps)
{
   const char* logContext = "DirInode setXAttr";
   std::string dentryPath = entries.getDirEntryPath() + "/" + fileName;

   int res = setxattr(dentryPath.c_str(), xAttrName.c_str(), &xAttrValue.front(),
         xAttrValue.size(), flags);

   if (res == 0 && fileName == MsgHelperXAttr::CURRENT_DIR_FILENAME && updateTimestamps)
   {
      rwlock.writeLock();
      updateTimeStampsAndStoreToDisk(__func__);
      rwlock.unlock();
   }

   if (res == 0 && fileName != MsgHelperXAttr::CURRENT_DIR_FILENAME && updateTimestamps)
   {
      MetaStore* metaStore = Program::getApp()->getMetaStore();

      FileInode* inode = metaStore->referenceFile(entryInfo);
      if (inode)
      {
         inode->updateInodeChangeTime(entryInfo);
         metaStore->releaseFile(getID(), inode);
      }
   }

   if(res != 0)
   {
      int err = errno;

      switch(err)
      {
         case EEXIST:
            return FhgfsOpsErr_EXISTS;

         case ENOATTR:
            return FhgfsOpsErr_NODATA;

         case ERANGE:
         case E2BIG:
            return FhgfsOpsErr_RANGE;

         case ENOENT:
            return FhgfsOpsErr_PATHNOTEXISTS;

         case ENOSPC:
            LogContext(logContext).logErr("failed on dentry: " + dentryPath +
                  " (No space left on device on metadata server)");
            return FhgfsOpsErr_NOSPACE;

         default: // don't forward other errors to the client, but log them on the server
            LogContext(logContext).logErr("failed on dentry: " + dentryPath +
                  " - " + std::string(strerror(err) ) + " on metadata directory.");
            return FhgfsOpsErr_INTERNAL;
      }
   }
   else
      return FhgfsOpsErr_SUCCESS;
}


FhgfsOpsErr DirInode::setOwnerNodeID(std::string entryName, uint16_t ownerNode)
{
   SafeRWLock safeLock(&rwlock, SafeRWLock_WRITE); // L O C K

   bool loadSuccess = loadIfNotLoadedUnlocked();
   if (!loadSuccess)
   {
      safeLock.unlock();
      return FhgfsOpsErr_PATHNOTEXISTS;
   }

   FhgfsOpsErr retVal = entries.setOwnerNodeID(entryName, ownerNode);

   safeLock.unlock(); // U N L O C K

   return retVal;
}
