/***
 * OBSOLETE
 * OBSOLETE
 * OBSOLETE
 */

/*#ifndef WRITELOCALFILEWORKV2_H_
#define WRITELOCALFILEWORKV2_H_

#include <common/Common.h>
#include <common/net/sock/Socket.h>
#include <common/components/worker/RetryableWork.h>
#include <common/toolkit/SynchronizedCounter.h>
#include <common/toolkit/vector/StrCpyVec.h>
#include <common/toolkit/vector/IntCpyVec.h>
#include <common/toolkit/vector/Int64CpyVec.h>

struct WriteLocalFileWorkV2;
typedef struct WriteLocalFileWorkV2 WriteLocalFileWorkV2;

static inline void WriteLocalFileWorkV2_init(WriteLocalFileWorkV2* this, App* app, const char* buf,
   loff_t offset, size_t size, const char* fileHandleID, unsigned accessFlags, uint16_t targetID,
   PathInfo* pathInfo, int64_t* nodeResult, SynchronizedCounter* counter,
   fhgfs_bool firstWriteDoneForTarget);
static inline WriteLocalFileWorkV2* WriteLocalFileWorkV2_construct(App* app, const char* buf,
   loff_t offset, size_t size, const char* fileHandleID, unsigned accessFlags, uint16_t targetID,
   PathInfo* pathInfo, int64_t* nodeResult, SynchronizedCounter* counter,
   fhgfs_bool firstWriteDoneForTarget);
static inline void WriteLocalFileWorkV2_uninit(Work* this);
static inline void WriteLocalFileWorkV2_destruct(Work* this);

extern int64_t __WriteLocalFileWorkV2_communicate(WriteLocalFileWorkV2* this, Node* node,
   char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen);
extern fhgfs_bool __WriteLocalFileWorkV2_prepareRetry(WriteLocalFileWorkV2* this);

// virtual functions
extern void WriteLocalFileWorkV2_process(Work* this, char* bufIn, unsigned bufInLen,
   char* bufOut, unsigned bufOutLen);

// getters & setters
static inline void WriteLocalFileWorkV2_setMirrorToTargetID(WriteLocalFileWorkV2* this,
   uint16_t mirrorToTargetID);
static inline void WriteLocalFileWorkV2_setUserdataForQuota(WriteLocalFileWorkV2* this,
   unsigned userID, unsigned groupID);


struct WriteLocalFileWorkV2
{
   RetryableWork retryableWork;

   App* app;

   const char* buf; // the data that shall be transferred
   loff_t offset; // target-node local offset
   size_t size; // size of the buf

   const char* fileHandleID;
   uint16_t targetID; // target ID
   uint16_t mirrorToTargetID;  // the data in this msg shall be forwarded to this mirror
   PathInfo* pathInfo; // not owned by this object
   unsigned accessFlags;

   int64_t* nodeResult;
   SynchronizedCounter* counter;

   fhgfs_bool firstWriteDoneForTarget;  true if the first chunk was written to the storage target,
                                          it's needed for the session check

   unsigned userID;
   unsigned groupID;
};


void WriteLocalFileWorkV2_init(WriteLocalFileWorkV2* this, App* app, const char* buf, loff_t offset,
      size_t size, const char* fileHandleID, unsigned accessFlags, uint16_t targetID,
      PathInfo* pathInfo, int64_t* nodeResult, SynchronizedCounter* counter,
      fhgfs_bool firstWriteDoneForTarget)
{
   RetryableWork_init( (RetryableWork*)this);

   this->app = app;

   this->buf = buf;
   this->offset = offset;
   this->size = size;

   this->fileHandleID = fileHandleID;
   this->targetID = targetID;
   this->mirrorToTargetID = 0;
   this->pathInfo = pathInfo;
   this->accessFlags = accessFlags;

   this->nodeResult = nodeResult;
   this->counter = counter;

   this->firstWriteDoneForTarget = firstWriteDoneForTarget;

   this->userID = 0;
   this->groupID = 0;

   // assign virtual functions
   ( (Work*)this)->uninit = WriteLocalFileWorkV2_uninit;

   ( (Work*)this)->process = WriteLocalFileWorkV2_process;
}

struct WriteLocalFileWorkV2* WriteLocalFileWorkV2_construct(App* app, const char* buf,
   loff_t offset, size_t size, const char* fileHandleID, unsigned accessFlags, uint16_t targetID,
   PathInfo* pathInfo, int64_t* nodeResult, SynchronizedCounter* counter,
   fhgfs_bool firstWriteDoneForTarget)
{
   struct WriteLocalFileWorkV2* this = (WriteLocalFileWorkV2*)os_kmalloc(sizeof(*this) );

   WriteLocalFileWorkV2_init(this, app, buf, offset, size, fileHandleID, accessFlags, targetID,
      pathInfo, nodeResult, counter, firstWriteDoneForTarget);

   return this;
}

void WriteLocalFileWorkV2_uninit(Work* this)
{
   RetryableWork_uninit(this);
}

void WriteLocalFileWorkV2_destruct(Work* this)
{
   WriteLocalFileWorkV2_uninit(this);

   os_kfree(this);
}

void WriteLocalFileWorkV2_setMirrorToTargetID(WriteLocalFileWorkV2* this, uint16_t mirrorToTargetID)
{
   this->mirrorToTargetID = mirrorToTargetID;
}

void WriteLocalFileWorkV2_setUserdataForQuota(WriteLocalFileWorkV2* this, unsigned userID,
   unsigned groupID)
{
   this->userID = userID;
   this->groupID = groupID;
}

#endif  WRITELOCALFILEWORKV2_H_ */
