#ifndef FHGFSOPSREMOTING_H_
#define FHGFSOPSREMOTING_H_

#include <common/Common.h>
#include <filesystem/FsObjectInfo.h>
#include <filesystem/FsDirInfo.h>
#include <filesystem/FsFileInfo.h>
#include <common/storage/StorageDefinitions.h>
#include <common/toolkit/MetadataTk.h>
#include <common/storage/StorageErrors.h>
#include <net/filesystem/RemotingIOInfo.h>
#include <os/iov_iter.h>
#include <toolkit/FhgfsPage.h>
#include <toolkit/FhgfsChunkPageVec.h>

enum Fhgfs_RWType;
typedef enum Fhgfs_RWType Fhgfs_RWType;


struct StripePattern; // forward declaration
struct NetMessage; // forward declaration

extern fhgfs_bool FhgfsOpsRemoting_initMsgBufCache(void);
extern void FhgfsOpsRemoting_destroyMsgBufCache(void);

extern FhgfsOpsErr FhgfsOpsRemoting_listdirFromOffset(const EntryInfo* entryInfo,
   FsDirInfo* dirInfo, unsigned maxOutNames);
extern FhgfsOpsErr FhgfsOpsRemoting_statRoot(App* app, fhgfs_stat* outFhgfsStat);
static inline FhgfsOpsErr FhgfsOpsRemoting_statDirect(App* app, const EntryInfo* entryInfo,
   fhgfs_stat* outFhgfsStat);
extern FhgfsOpsErr FhgfsOpsRemoting_statDirect(App* app, const EntryInfo* entryInfo,
   fhgfs_stat* outFhgfsStat);
extern FhgfsOpsErr FhgfsOpsRemoting_statAndGetParentInfo(App* app, const EntryInfo* entryInfo,
   fhgfs_stat* outFhgfsStat, uint16_t* outParentNodeID, char** outParentEntryID);
extern FhgfsOpsErr FhgfsOpsRemoting_setAttr(App* app, const EntryInfo* entryInfo,
   SettableFileAttribs* fhgfsAttr, int validAttribs);
extern FhgfsOpsErr FhgfsOpsRemoting_mkdir(App* app, const EntryInfo* parentInfo,
   struct CreateInfo* createnfo, EntryInfo* outEntryInfo);
extern FhgfsOpsErr FhgfsOpsRemoting_rmdir(App* app, const EntryInfo* parentInfo,
   const char* entryName);
extern FhgfsOpsErr FhgfsOpsRemoting_mkfile(App* app, const EntryInfo* parentInfo,
   struct CreateInfo* createInfo, EntryInfo* outEntryInfo);
extern FhgfsOpsErr FhgfsOpsRemoting_mkfileWithStripeHints(App* app, const EntryInfo* parentInfo,
   struct CreateInfo* createInfo, unsigned numtargets, unsigned chunksize, EntryInfo* outEntryInfo);
extern FhgfsOpsErr FhgfsOpsRemoting_unlinkfile(App* app, const EntryInfo* parentInfo,
   const char* entryName);
extern FhgfsOpsErr FhgfsOpsRemoting_openfile(const EntryInfo* entryInfo, RemotingIOInfo* ioInfo);
extern FhgfsOpsErr FhgfsOpsRemoting_closefile(const EntryInfo* entryInfo,
   RemotingIOInfo* ioInfo);
extern FhgfsOpsErr FhgfsOpsRemoting_closefileEx(const EntryInfo* entryInfo,
   RemotingIOInfo* ioInfo, fhgfs_bool allowRetries);
extern FhgfsOpsErr FhgfsOpsRemoting_flockAppend(const EntryInfo* entryInfo,
   RemotingIOInfo* ioInfo, int64_t clientFD, int ownerPID, int lockTypeFlags);
extern FhgfsOpsErr FhgfsOpsRemoting_flockAppendEx(const EntryInfo* entryInfo,
   RemotingIOInfo* ioInfo, int64_t clientFD, int ownerPID, int lockTypeFlags,
   fhgfs_bool allowRetries);
extern FhgfsOpsErr FhgfsOpsRemoting_flockEntry(const EntryInfo* entryInfo,
   RemotingIOInfo* ioInfo, int64_t clientFD, int ownerPID, int lockTypeFlags);
extern FhgfsOpsErr FhgfsOpsRemoting_flockEntryEx(const EntryInfo* entryInfo,
   RemotingIOInfo* ioInfo, int64_t clientFD, int ownerPID, int lockTypeFlags,
   fhgfs_bool allowRetries);
extern FhgfsOpsErr FhgfsOpsRemoting_flockRange(const EntryInfo* entryInfo,
   RemotingIOInfo* ioInfo, int ownerPID, int lockTypeFlags, uint64_t start, uint64_t end);
extern FhgfsOpsErr FhgfsOpsRemoting_flockRangeEx(const EntryInfo* entryInfo,
   RemotingIOInfo* ioInfo, int ownerPID, int lockTypeFlags, uint64_t start, uint64_t end,
   fhgfs_bool allowRetries);
extern ssize_t FhgfsOpsRemoting_writefile(const char __user *buf, size_t size, loff_t offset,
   RemotingIOInfo* ioInfo);
extern ssize_t FhgfsOpsRemoting_writefileVec(const struct iov_iter* iter, loff_t offset,
   RemotingIOInfo* ioInfo);
extern ssize_t FhgfsOpsRemoting_rwChunkPageVec(FhgfsChunkPageVec *pageVec, RemotingIOInfo* ioInfo,
   Fhgfs_RWType rwType);
extern ssize_t FhgfsOpsRemoting_readfile(char __user *buf, size_t size, loff_t offset,
   RemotingIOInfo* ioInfo, FhgfsInode* fhgfsInode);
extern ssize_t FhgfsOpsRemoting_readfileVec(const struct iov_iter* iter, loff_t offset,
   RemotingIOInfo* ioInfo, FhgfsInode* fhgfsInode);
extern FhgfsOpsErr FhgfsOpsRemoting_rename(App* app, const char* oldName, unsigned oldLen,
   DirEntryType entryType, EntryInfo* fromDirInfo, const char* newName, unsigned newLen,
   EntryInfo* toDirInfo);
extern FhgfsOpsErr FhgfsOpsRemoting_truncfile(App* app, const EntryInfo* entryInfo, loff_t size);
extern FhgfsOpsErr FhgfsOpsRemoting_fsyncfile(RemotingIOInfo* ioInfo, fhgfs_bool forceRemoteFlush,
   fhgfs_bool checkSession, fhgfs_bool doSyncOnClose);
extern FhgfsOpsErr FhgfsOpsRemoting_statStoragePath(App* app, bool ignoreErrors,
   int64_t* outSizeTotal, int64_t* outSizeFree);
extern FhgfsOpsErr FhgfsOpsRemoting_listXAttr(App* app, const EntryInfo* entryInfo, char* value,
      size_t size, ssize_t* outSize);
extern FhgfsOpsErr FhgfsOpsRemoting_getXAttr(App* app, const EntryInfo* entryInfo, const char* name,
      void* value, size_t size, ssize_t* outSize);
extern FhgfsOpsErr FhgfsOpsRemoting_removeXAttr(App* app, const EntryInfo* entryInfo,
      const char* name);
extern FhgfsOpsErr FhgfsOpsRemoting_setXAttr(App* app, const EntryInfo* entryInfo, const char* name,
      const char* value, const size_t size, int flags);


extern FhgfsOpsErr FhgfsOpsRemoting_lookupIntent(App* app,
   const LookupIntentInfoIn* inInfo, LookupIntentInfoOut* outInfo);

extern FhgfsOpsErr FhgfsOpsRemoting_hardlink(App* app, const char* fromName, unsigned fromLen,
   EntryInfo* fromInfo, EntryInfo* fromDirInfo, const char* toName, unsigned toLen,
   EntryInfo* toDirInfo);

extern FhgfsOpsErr FhgfsOpsRemoting_refreshEntry(App* app, const EntryInfo* entryInfo);

FhgfsOpsErr __FhgfsOpsRemoting_flockGenericEx(struct NetMessage* requestMsg, unsigned respMsgType,
   uint16_t ownerNodeID, RemotingIOInfo* ioInfo, int lockTypeFlags, char* lockAckID,
   fhgfs_bool allowRetries);

#ifdef LOG_DEBUG_MESSAGES
extern void __FhgfsOpsRemoting_logDebugIOCall(const char* logContext, size_t size, loff_t offset,
   RemotingIOInfo* ioInfo, const char* rwTypeStr);
#else
#define __FhgfsOpsRemoting_logDebugIOCall(logContext, size, offset, ioInfo, rwTypeStr)
#endif // LOG_DEBUG_MESSAGES


enum Fhgfs_RWType
{
   BEEGFS_RWTYPE_READ = 0,  // read request
   BEEGFS_RWTYPE_WRITE      // write request
};


/**
 * Stat a file or directory using EntryInfo.
 */
FhgfsOpsErr FhgfsOpsRemoting_statDirect(App* app, const EntryInfo* entryInfo,
   fhgfs_stat* outFhgfsStat)
{
   return FhgfsOpsRemoting_statAndGetParentInfo(app, entryInfo, outFhgfsStat, NULL, NULL);
}


#endif /*FHGFSOPSREMOTING_H_*/
