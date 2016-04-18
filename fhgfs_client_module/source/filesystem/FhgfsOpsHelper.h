#ifndef FHGFSOPSHELPER_H_
#define FHGFSOPSHELPER_H_

#include <app/App.h>
#include <common/storage/StorageErrors.h>
#include <common/toolkit/MetadataTk.h>
#include <common/Common.h>
#include <components/InternodeSyncer.h>
#include <filesystem/FsDirInfo.h>
#include <filesystem/FsFileInfo.h>
#include <net/filesystem/FhgfsOpsRemoting.h>


#ifdef LOG_DEBUG_MESSAGES
   /**
    * Print debug messages. Used to trace functions which are frequently called and therefore
    * has to be exlicitly enabled at compilation time.
    * Has to be a macro due to usage of __VA_ARGS__.
    */
   #define FhgfsOpsHelper_logOpDebug(app, dentry, inode, logContext, msgStr, ...) \
      FhgfsOpsHelper_logOpMsg(Log_SPAM, app, dentry, inode, logContext, msgStr, ##__VA_ARGS__)
#else
   // no debug build, so those debug messages disabled at all
   #define FhgfsOpsHelper_logOpDebug(app, dentry, inode, logContext, msgStr, ...)
#endif // LOG_DEBUG_MESSAGES



extern void FhgfsOpsHelper_logOpMsg(int level, App* app, struct dentry* dentry, struct inode* inode,
   const char *logContext, const char *msgStr, ...);

extern int FhgfsOpsHelper_refreshDirInfoIncremental(App* app, const EntryInfo* entryInfo,
   FsDirInfo* dirInfo, fhgfs_bool forceUpdate);

extern FhgfsOpsErr FhgfsOpsHelper_flushCache(App* app, FhgfsInode* fhgfsInode,
   fhgfs_bool discardCacheOnError);
extern FhgfsOpsErr FhgfsOpsHelper_flushCacheNoWait(App* app, FhgfsInode* fhgfsInode,
   fhgfs_bool discardCacheOnError);
extern ssize_t FhgfsOpsHelper_writeCached(const char __user *buf, size_t size,
   loff_t offset, FhgfsInode* fhgfsInode, FsFileInfo* fileInfo, RemotingIOInfo* ioInfo);
extern ssize_t FhgfsOpsHelper_readCached(char __user *buf, size_t size,
   loff_t offset, FhgfsInode* fhgfsInode, FsFileInfo* fileInfo, RemotingIOInfo* ioInfo);

extern FhgfsOpsErr __FhgfsOpsHelper_flushCacheUnlocked(App* app, FhgfsInode* fhgfsInode,
   fhgfs_bool discardCacheOnError);
extern ssize_t __FhgfsOpsHelper_writeCacheFlushed(const char __user *buf, size_t size,
   loff_t offset, FhgfsInode* fhgfsInode, FsFileInfo* fileInfo, RemotingIOInfo* ioInfo);
extern ssize_t __FhgfsOpsHelper_readCacheFlushed(char __user *buf, size_t size,
   loff_t offset, FhgfsInode* fhgfsInode, FsFileInfo* fileInfo, RemotingIOInfo* ioInfo);
extern void __FhgfsOpsHelper_discardCache(App* app, FhgfsInode* fhgfsInode);

extern ssize_t FhgfsOpsHelper_writefileEx(FhgfsInode* fhgfsInode, const char __user *buf,
   size_t size, loff_t offset, RemotingIOInfo* ioInfo);
extern ssize_t FhgfsOpsHelper_appendfile(FhgfsInode* fhgfsInode, const char __user *buf,
   size_t size, RemotingIOInfo* ioInfo, loff_t* outNewOffset);

extern FhgfsOpsErr FhgfsOpsHelper_readOrClearUser(App* app, char __user *buf, size_t size,
   loff_t offset, FsFileInfo* fileInfo, RemotingIOInfo* ioInfo);

extern void FhgfsOpsHelper_getRelativeLinkStr(const char* pathFrom, const char* pathTo,
   char** pathRelativeTo);
extern int FhgfsOpsHelper_symlink(App* app, const EntryInfo* parentInfo, const char* to,
   struct CreateInfo *createInfo, EntryInfo* outEntryInfo);
extern int FhgfsOpsHelper_readlink(App* app, const EntryInfo* entryInfo, char __user* buf,
   int size);

extern ssize_t FhgfsOpsHelper_readStateless(App* app, const EntryInfo* entryInfo,
   char __user *buf, size_t size, loff_t offset);
extern ssize_t FhgfsOpsHelper_writeStateless(App* app, const EntryInfo* entryInfo,
   const char __user *buf, size_t size, loff_t offset, unsigned uid, unsigned gid);
extern ssize_t FhgfsOpsHelper_writeStatelessInode(FhgfsInode* fhgfsInode,
   const char __user *buf, size_t size, loff_t offset);


// inliners
static inline void FhgfsOpsHelper_logOp(int level, App* app, struct dentry* dentry,
   struct inode* inode, const char *logContext);
static inline FhgfsOpsErr FhgfsOpsHelper_closefileWithAsyncRetry(const EntryInfo* entryInfo,
   RemotingIOInfo* ioInfo);
static inline FhgfsOpsErr FhgfsOpsHelper_unlockEntryWithAsyncRetry(const EntryInfo* entryInfo,
   RemotingIOInfo* ioInfo, int64_t clientFD);
static inline FhgfsOpsErr FhgfsOpsHelper_unlockRangeWithAsyncRetry(const EntryInfo* entryInfo,
   RemotingIOInfo* ioInfo, int ownerPID);


/**
 * Wrapper function for FhgfsOpsHelper_logOpMsg(), just skips msgStr and sets it to NULL
 */
void FhgfsOpsHelper_logOp(int level, App* app, struct dentry* dentry, struct inode* inode,
   const char *logContext)
{
   Logger* log = App_getLogger(app);

   // check the log level here, as this function is inlined
   if (likely(level > Logger_getLogLevel(log) ) )
      return;

   FhgfsOpsHelper_logOpMsg(level, app, dentry, inode, logContext, NULL);
}

/**
 * Call close remoting and add close operation to the corresponding async retry queue if a
 * communucation error occurred.
 *
 * @param entryInfo will be copied
 * @param ioInfo will be copied
 * @return remoting result (independent of whether the operation was added for later retry)
 */
FhgfsOpsErr FhgfsOpsHelper_closefileWithAsyncRetry(const EntryInfo* entryInfo,
   RemotingIOInfo* ioInfo)
{
   FhgfsOpsErr closeRes = FhgfsOpsRemoting_closefile(entryInfo, ioInfo);

   if( (closeRes == FhgfsOpsErr_COMMUNICATION) || (closeRes == FhgfsOpsErr_INTERRUPTED) )
   { // comm error => add for later retry
      InternodeSyncer* syncer = App_getInternodeSyncer(ioInfo->app);
      InternodeSyncer_delayedCloseAdd(syncer, entryInfo, ioInfo);
   }

   return closeRes;
}

FhgfsOpsErr FhgfsOpsHelper_unlockEntryWithAsyncRetry(const EntryInfo* entryInfo,
   RemotingIOInfo* ioInfo, int64_t clientFD)
{
   FhgfsOpsErr unlockRes = FhgfsOpsRemoting_flockEntry(entryInfo, ioInfo, clientFD, 0,
      ENTRYLOCKTYPE_CANCEL);

   if( (unlockRes == FhgfsOpsErr_COMMUNICATION) || (unlockRes == FhgfsOpsErr_INTERRUPTED) )
   { // comm error => add for later retry

      InternodeSyncer* syncer = App_getInternodeSyncer(ioInfo->app);
      InternodeSyncer_delayedEntryUnlockAdd(syncer, entryInfo, ioInfo, clientFD);
   }

   return unlockRes;
}

FhgfsOpsErr FhgfsOpsHelper_unlockRangeWithAsyncRetry(const EntryInfo* entryInfo,
   RemotingIOInfo* ioInfo, int ownerPID)
{
   FhgfsOpsErr unlockRes = FhgfsOpsRemoting_flockRange(entryInfo, ioInfo, ownerPID,
      ENTRYLOCKTYPE_CANCEL, 0, ~0ULL);

   if( (unlockRes == FhgfsOpsErr_COMMUNICATION) || (unlockRes == FhgfsOpsErr_INTERRUPTED) )
   { // comm error => add for later retry
      InternodeSyncer* syncer = App_getInternodeSyncer(ioInfo->app);
      InternodeSyncer_delayedRangeUnlockAdd(syncer, entryInfo, ioInfo, ownerPID);
   }

   return unlockRes;
}


#endif /*FHGFSOPSHELPER_H_*/
