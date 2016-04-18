#ifndef FHGFSOPSCOMMKITVEC_H_
#define FHGFSOPSCOMMKITVEC_H_

#include <net/filesystem/FhgfsOpsRemoting.h>
#include <toolkit/FhgfsPage.h>
#include <toolkit/FhgfsChunkPageVec.h>

#include <linux/fs.h>

#include "FhgfsOpsCommKitCommon.h"
#include "RemotingIOInfo.h"


struct FhgfsCommKitVec;
typedef struct FhgfsCommKitVec FhgfsCommKitVec;

struct CommKitVecHelper;
typedef struct CommKitVecHelper CommKitVecHelper;


extern int64_t FhgfsOpsCommKitVec_rwFileCommunicate(App* app, RemotingIOInfo* ioInfo,
   FhgfsCommKitVec* comm, Fhgfs_RWType rwType);


static inline FhgfsCommKitVec FhgfsOpsCommKitVec_assignRWfileState(FhgfsChunkPageVec* pageVec,
   unsigned pageIdx, unsigned numPages, loff_t offset, uint16_t targetID, char* msgBuf);

static inline void FhgfsOpsCommKitVec_setRWfileStateUseBuddyMirrorSecond(
   fhgfs_bool useBuddyMirrorSecond, FhgfsCommKitVec* outComm);
static inline void FhgfsOpsCommKitVec_setRWfileStateUseServersideMirroring(
   fhgfs_bool useServersideMirroring, FhgfsCommKitVec* outComm);
static inline void FhgfsOpsCommKitVec_setRWFileStateFirstWriteDone(
   fhgfs_bool firstWriteDoneForTarget, FhgfsCommKitVec* outComm);

static inline size_t FhgfsOpsCommKitVec_getRemainingDataSize(FhgfsCommKitVec* comm);
static inline loff_t FhgfsOpsCommKitVec_getOffset(FhgfsCommKitVec* comm);



#ifdef FhgfsOpsCommKitVec // local (file/class-like) declarations

static void __FhgfsOpsCommKitVec_readfileStagePREPARE(CommKitVecHelper* commHelper,
   FhgfsCommKitVec* comm);
static void __FhgfsOpsCommKitVec_readfileStageRECVHEADER(CommKitVecHelper* commHelper,
   FhgfsCommKitVec* comm);
static void __FhgfsOpsCommKitVec_readfileStageRECVDATA(CommKitVecHelper* commHelper,
   FhgfsCommKitVec* comm);
static void __FhgfsOpsCommKitVec_handleReadError(CommKitVecHelper* commHelper, FhgfsCommKitVec* comm,
   FhgfsPage* fhgfsPage, ssize_t recvRes, size_t missingPgLen);
static void __FhgfsOpsCommKitVec_readfileStageSOCKETEXCEPTION(CommKitVecHelper* commHelper,
   FhgfsCommKitVec* comm);
static void __FhgfsOpsCommKitVec_readfileStageCLEANUP(CommKitVecHelper* commHelper,
   FhgfsCommKitVec* comm);
static void __FhgfsOpsCommKitVec_readfileStageHandlePages(CommKitVecHelper* commHelper,
   FhgfsCommKitVec* comm);



static void __FhgfsOpsCommKitVec_writefileStagePREPARE(CommKitVecHelper* commHelper,
   FhgfsCommKitVec* comm);
static void __FhgfsOpsCommKitVec_writefileStageSENDHEADER(CommKitVecHelper* commHelper,
   FhgfsCommKitVec* comm);
static void __FhgfsOpsCommKitVec_writefileStageSENDDATA(CommKitVecHelper* commHelper,
   FhgfsCommKitVec* comm);
static void __FhgfsOpsCommKitVec_writefileStageRECV(CommKitVecHelper* commHelper,
   FhgfsCommKitVec* comm);
static void __FhgfsOpsCommKitVec_writefileStageSOCKETEXCEPTION(CommKitVecHelper* commHelper,
   FhgfsCommKitVec* comm);
static void __FhgfsOpsCommKitVec_writefileStageCLEANUP(CommKitVecHelper* commHelper,
   FhgfsCommKitVec* comm);
static void __FhgfsOpsCommKitVec_writefileStageHandlePages(CommKitVecHelper* commHelper,
   FhgfsCommKitVec* comm);

static int64_t __FhgfsOpsCommKitVec_writefileCommunicate(CommKitVecHelper* commHelper,
   FhgfsCommKitVec* comm);
static int64_t __FhgfsOpsCommKitVec_readfileCommunicate(CommKitVecHelper* commHelper,
   FhgfsCommKitVec* comm);


#endif // local (file/class-like) declarations


struct FhgfsCommKitVec
{
   // Depending if we are going to read or write, the corresponding struct will be used
   union
   {
         // NOTE: The *initial* stage for read and write MUST be RW_FILE_STAGE_PREPARE
         struct readState
         {
               size_t reqLen;     // requested size from client to server
               size_t serverSize; // how many data the server is going to send (<= reqLen)
         } read ;

         struct writeState
         {
               WriteLocalFileRespMsg respMsg; // response message
         } write;
   };

   size_t hdrLen; // size (length) of the header message

   size_t numSuccessPages; // number of pages successfully handled

   // data for all stages

   // (assigned by the state-creator)
   FhgfsChunkPageVec* pageVec;
   loff_t initialOffset; // initial offset before sending any data

   uint16_t targetID; // target ID
   char* msgBuf; // for serialization

   fhgfs_bool firstWriteDoneForTarget; /* true if data was previously written to this target in
                                          this session; used for the session check */
   fhgfs_bool useBuddyMirrorSecond; // for buddy mirroring, this msg goes to secondary
   fhgfs_bool useServersideMirroring; // data shall be forwarded to mirror by primary

   // (assigned by the preparation stage)
   Node* node; // target-node reference
   Socket* sock; // target-node connection

   // result information
   int64_t nodeResult; /* the amount of data that have been read/written on the server side
                        * (or a negative fhgfs error code) */

   fhgfs_bool doHeader; // continue with the read/write header stage
};

/**
 * Additional data that is required or useful for all the states.
 * (This is shared states data.)
 *
 * invariant: (numRetryWaiters + isDone + numUnconnectable + numPollSocks) <= numStates
 */
struct CommKitVecHelper
{
   App* app;
   Logger* log;
   char* logContext;

   RemotingIOInfo* ioInfo;
};


/**
 * Note: Caller still MUST initialize type.{read/write}.stage = RW_FILE_STAGE_PREPARE.
 * Note: Initializes the expectedNodeResult attribute from the size argument.
 * Note: Defaults to server-side mirroring.
 */
FhgfsCommKitVec FhgfsOpsCommKitVec_assignRWfileState(FhgfsChunkPageVec* pageVec,
   unsigned pageIdx, unsigned numPages, loff_t offset, uint16_t targetID, char* msgBuf)
{
   FhgfsCommKitVec state =
   {
      .initialOffset      = offset,
      .targetID           = targetID,
      .msgBuf             = msgBuf,

      .firstWriteDoneForTarget = fhgfs_false, // set via separate setter method
      .useBuddyMirrorSecond    = fhgfs_false, // set via separate setter method
      .useServersideMirroring  = fhgfs_true, // set via separate setter method

      .nodeResult         = -FhgfsOpsErr_INTERNAL,
      .numSuccessPages    = 0,
   };

   state.pageVec = pageVec;

   return state;
}

void FhgfsOpsCommKitVec_setRWfileStateUseBuddyMirrorSecond(fhgfs_bool useBuddyMirrorSecond,
   FhgfsCommKitVec* outComm)
{
   outComm->useBuddyMirrorSecond = useBuddyMirrorSecond;
}

void FhgfsOpsCommKitVec_setRWfileStateUseServersideMirroring(fhgfs_bool useServersideMirroring,
   FhgfsCommKitVec* outComm)
{
   outComm->useServersideMirroring = useServersideMirroring;
}

void FhgfsOpsCommKitVec_setRWFileStateFirstWriteDone(fhgfs_bool firstWriteDoneForTarget,
   FhgfsCommKitVec* outComm)
{
   outComm->firstWriteDoneForTarget = firstWriteDoneForTarget;
}


/**
 * Get the overall remaining data size (chunkPageVec + last incompletely processed page)
 */
size_t FhgfsOpsCommKitVec_getRemainingDataSize(FhgfsCommKitVec* comm)
{
   return FhgfsChunkPageVec_getRemainingDataSize(comm->pageVec);
}

/**
 * Get the current offset
 */
loff_t FhgfsOpsCommKitVec_getOffset(FhgfsCommKitVec* comm)
{
   FhgfsChunkPageVec* pageVec = comm->pageVec;

   // data size already send/received (written/read)
   size_t processedSize =
      FhgfsChunkPageVec_getDataSize(pageVec) - FhgfsOpsCommKitVec_getRemainingDataSize(comm);

   return comm->initialOffset + processedSize;
}


#endif /*FHGFSOPSCOMMKITVEC_H_*/
