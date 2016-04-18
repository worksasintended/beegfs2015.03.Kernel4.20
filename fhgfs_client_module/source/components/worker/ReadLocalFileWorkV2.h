#ifndef READLOCALFILEWORKV2_H_
#define READLOCALFILEWORKV2_H_

#include <common/Common.h>
#include <common/net/sock/Socket.h>
#include <common/components/worker/RetryableWork.h>
#include <common/toolkit/SynchronizedCounter.h>
#include <common/toolkit/vector/StrCpyVec.h>
#include <common/toolkit/vector/IntCpyVec.h>
#include <common/toolkit/vector/Int64CpyVec.h>

struct ReadLocalFileWorkV2;
typedef struct ReadLocalFileWorkV2 ReadLocalFileWorkV2;

static inline void ReadLocalFileWorkV2_init(ReadLocalFileWorkV2* this, App* app, char* buf,
   loff_t offset, size_t size, const char* fileHandleID, unsigned accessFlags, uint16_t targetID,
   PathInfo* pathInfo, int64_t* nodeResult, SynchronizedCounter* counter,
   fhgfs_bool firstWriteDoneForTarget);
static inline ReadLocalFileWorkV2* ReadLocalFileWorkV2_construct(App* app, char* buf, loff_t offset,
   size_t size, const char* fileHandleID, unsigned accessFlags, uint16_t targetID,
   PathInfo* pathInfo, int64_t* nodeResult, SynchronizedCounter* counter,
   fhgfs_bool firstWriteDoneForTarget);
static inline void ReadLocalFileWorkV2_uninit(Work* this);
static inline void ReadLocalFileWorkV2_destruct(Work* this);

extern int64_t __ReadLocalFileWorkV2_communicate(ReadLocalFileWorkV2* this, Node* node,
   char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen);
extern fhgfs_bool __ReadLocalFileWorkV2_prepareRetry(ReadLocalFileWorkV2* this);

// virtual functions
extern void ReadLocalFileWorkV2_process(Work* this, char* bufIn, unsigned bufInLen,
   char* bufOut, unsigned bufOutLen);


struct ReadLocalFileWorkV2
{
   RetryableWork retryableWork;

   App* app;

   char* buf; // the data that shall be transferred
   loff_t offset; // target-node local offset
   size_t size; // size of the buf

   const char* fileHandleID;
   uint16_t targetID; // target ID
   PathInfo* pathInfo; // Note: We are not the owner of this object
   unsigned accessFlags;

   int64_t* nodeResult;
   SynchronizedCounter* counter;

   fhgfs_bool firstWriteDoneForTarget; /* true if the first chunk was written to the storage target,
                                          it's needed for the session check*/
};

/**
 * Note: pathInfo is not owned by this object.
 */
void ReadLocalFileWorkV2_init(ReadLocalFileWorkV2* this, App* app, char* buf, loff_t offset,
      size_t size, const char* fileHandleID, unsigned accessFlags, uint16_t targetID,
      PathInfo *pathInfo, int64_t* nodeResult, SynchronizedCounter* counter,
      fhgfs_bool firstWriteDoneForTarget)
{
   RetryableWork_init( (RetryableWork*)this);

   this->app = app;

   this->buf = buf;
   this->offset = offset;
   this->size = size;

   this->fileHandleID   = fileHandleID;
   this->targetID       = targetID;
   this->pathInfo       = pathInfo;
   this->accessFlags    = accessFlags;

   this->nodeResult = nodeResult;
   this->counter = counter;

   this->firstWriteDoneForTarget = firstWriteDoneForTarget;

   // assign virtual functions
   ( (Work*)this)->uninit = ReadLocalFileWorkV2_uninit;

   ( (Work*)this)->process = ReadLocalFileWorkV2_process;
}

struct ReadLocalFileWorkV2* ReadLocalFileWorkV2_construct(App* app, char* buf, loff_t offset,
      size_t size, const char* fileHandleID, unsigned accessFlags, uint16_t targetID,
      PathInfo* pathInfo, int64_t* nodeResult, SynchronizedCounter* counter,
      fhgfs_bool firstWriteDoneForTarget)
{
   struct ReadLocalFileWorkV2* this = (ReadLocalFileWorkV2*)os_kmalloc(sizeof(*this) );

   ReadLocalFileWorkV2_init(this, app, buf, offset, size, fileHandleID, accessFlags, targetID,
      pathInfo, nodeResult, counter, firstWriteDoneForTarget);

   return this;
}

void ReadLocalFileWorkV2_uninit(Work* this)
{
   RetryableWork_uninit( (Work*)this);
}

void ReadLocalFileWorkV2_destruct(Work* this)
{
   ReadLocalFileWorkV2_uninit( (Work*)this);

   os_kfree(this);
}


#endif /* READLOCALFILEWORKV2_H_ */
