#ifndef FSYNCCHUNKFILEWORK_H_
#define FSYNCCHUNKFILEWORK_H_

#include <app/App.h>
#include <common/Common.h>
#include <common/net/message/NetMessage.h>
#include <common/net/sock/Socket.h>
#include <common/components/worker/RetryableWork.h>
#include <common/storage/striping/StripePattern.h>
#include <common/toolkit/SynchronizedCounter.h>
#include <common/toolkit/vector/StrCpyVec.h>
#include <common/toolkit/vector/IntCpyVec.h>
#include <common/toolkit/vector/Int64CpyVec.h>
#include <toolkit/BitStore.h>


struct FSyncChunkFileWork;
typedef struct FSyncChunkFileWork FSyncChunkFileWork;

static inline void FSyncChunkFileWork_init(FSyncChunkFileWork* this, App* app, unsigned nodeIndex,
   const char* fileHandleID, StripePattern* pattern, UInt16Vec* targetIDs, FhgfsOpsErr* nodeResults,
   SynchronizedCounter* counter, fhgfs_bool forceRemoteFlush, fhgfs_bool firstWriteDoneForTarget,
   fhgfs_bool checkSession, fhgfs_bool doSyncOnClose);
static inline FSyncChunkFileWork* FSyncChunkFileWork_construct(App* app, unsigned nodeIndex,
   const char* fileHandleID, StripePattern* pattern,UInt16Vec* targetIDs, FhgfsOpsErr* nodeResults,
   SynchronizedCounter* counter, fhgfs_bool forceRemoteFlush, fhgfs_bool firstWriteDoneForTarget,
   fhgfs_bool checkSession, fhgfs_bool doSyncOnClose);
static inline void FSyncChunkFileWork_uninit(Work* this);
static inline void FSyncChunkFileWork_destruct(Work* this);

extern FhgfsOpsErr __FSyncChunkFileWork_communicate(FSyncChunkFileWork* this, Node* node,
   char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen);
extern fhgfs_bool __FSyncChunkFileWork_prepareRetry(FSyncChunkFileWork* this);

// virtual functions
extern void FSyncChunkFileWork_process(Work* this, char* bufIn, unsigned bufInLen,
   char* bufOut, unsigned bufOutLen);

// getters & setters
static inline void FSyncChunkFileWork_setUseBuddyMirrorSecond(FSyncChunkFileWork* this,
   fhgfs_bool useBuddyMirrorSecond);
static inline void FSyncChunkFileWork_setMsgUserID(FSyncChunkFileWork* this, unsigned msgUserID);


struct FSyncChunkFileWork
{
   RetryableWork retryableWork;

   App* app;

   unsigned nodeIndex;
   const char* fileHandleID;
   StripePattern* pattern;
   UInt16Vec* targetIDs;

   unsigned msgUserID; // user ID to be set in net msg header

   FhgfsOpsErr* nodeResults;
   SynchronizedCounter* counter;

   fhgfs_bool forceRemoteFlush; // true if the server must sync the file (if a flush was called)

   fhgfs_bool checkSession; // true if the server must check the session

   fhgfs_bool doSyncOnClose; // true if the server must sync the file (if a file was closed)

   fhgfs_bool firstWriteDoneForTarget; /* true if the first chunk was written to the storage target,
                                          it's needed for the session check*/

   fhgfs_bool useBuddyMirrorSecond;
};


void FSyncChunkFileWork_init(FSyncChunkFileWork* this, App* app, unsigned nodeIndex,
   const char* fileHandleID, StripePattern* pattern, UInt16Vec* targetIDs, FhgfsOpsErr* nodeResults,
   SynchronizedCounter* counter, fhgfs_bool forceRemoteFlush, fhgfs_bool firstWriteDoneForTarget,
   fhgfs_bool checkSession, fhgfs_bool doSyncOnClose)
{
   RetryableWork_init( (RetryableWork*)this);

   this->app = app;

   this->nodeIndex = nodeIndex;
   this->fileHandleID = fileHandleID;
   this->pattern = pattern;
   this->targetIDs = targetIDs;

   this->msgUserID = NETMSG_DEFAULT_USERID;

   this->nodeResults = nodeResults;
   this->counter = counter;

   this->forceRemoteFlush = forceRemoteFlush;

   this->firstWriteDoneForTarget = firstWriteDoneForTarget;

   this->checkSession = checkSession;

   this->doSyncOnClose = doSyncOnClose;

   this->useBuddyMirrorSecond = fhgfs_false;

   // assign virtual functions
   ( (Work*)this)->uninit = FSyncChunkFileWork_uninit;

   ( (Work*)this)->process = FSyncChunkFileWork_process;
}

struct FSyncChunkFileWork* FSyncChunkFileWork_construct(App* app, unsigned nodeIndex,
   const char* fileHandleID, StripePattern* pattern, UInt16Vec* targetIDs,
   FhgfsOpsErr* nodeResults, SynchronizedCounter* counter, fhgfs_bool forceRemoteFlush,
   fhgfs_bool firstWriteDoneForTarget, fhgfs_bool checkSession, fhgfs_bool doSyncOnClose)
{
   struct FSyncChunkFileWork* this = (FSyncChunkFileWork*)os_kmalloc(sizeof(*this) );

   if(likely(this) )
      FSyncChunkFileWork_init(this, app, nodeIndex, fileHandleID, pattern, targetIDs, nodeResults,
         counter, forceRemoteFlush, firstWriteDoneForTarget, checkSession, doSyncOnClose);

   return this;
}

void FSyncChunkFileWork_uninit(Work* this)
{
   RetryableWork_uninit(this);
}

void FSyncChunkFileWork_destruct(Work* this)
{
   FSyncChunkFileWork_uninit(this);

   os_kfree(this);
}

void FSyncChunkFileWork_setUseBuddyMirrorSecond(FSyncChunkFileWork* this,
   fhgfs_bool useBuddyMirrorSecond)
{
   this->useBuddyMirrorSecond = useBuddyMirrorSecond;
}

void FSyncChunkFileWork_setMsgUserID(FSyncChunkFileWork* this, unsigned msgUserID)
{
   this->msgUserID = msgUserID;
}

#endif /*FSYNCCHUNKFILEWORK_H_*/
