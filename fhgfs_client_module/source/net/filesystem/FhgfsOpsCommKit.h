#ifndef FHGFSOPSCOMMKIT_H_
#define FHGFSOPSCOMMKIT_H_

#include <common/storage/PathInfo.h>
#include "FhgfsOpsCommKitCommon.h"

struct ReadfileState;
typedef struct ReadfileState ReadfileState;
struct WritefileState;
typedef struct WritefileState WritefileState;

extern void FhgfsOpsCommKit_readfileV2bCommunicate(App* app, RemotingIOInfo* ioInfo,
   ReadfileState* states, size_t numStates, PollSocketEx* pollSocks);
extern void FhgfsOpsCommKit_writefileV2bCommunicate(App* app, RemotingIOInfo* ioInfo,
   WritefileState* states, size_t numStates, PollSocketEx* pollSocks);


extern fhgfs_bool __FhgfsOpsCommKit_readfileV2bStagePREPARE(RWfileStatesHelper* statesHelper,
   ReadfileState* currentState);
extern void __FhgfsOpsCommKit_readfileV2bStageSEND(RWfileStatesHelper* statesHelper,
   ReadfileState* currentState);
extern void __FhgfsOpsCommKit_readfileV2bStageRECVHEADER(RWfileStatesHelper* statesHelper,
   ReadfileState* currentState);
extern void __FhgfsOpsCommKit_readfileV2bStageRECVDATA(RWfileStatesHelper* statesHelper,
   ReadfileState* currentState);
extern void __FhgfsOpsCommKit_readfileV2bStageSOCKETEXCEPTION(RWfileStatesHelper* statesHelper,
   ReadfileState* currentState);
extern void __FhgfsOpsCommKit_readfileV2bStageSOCKETINVALIDATE(RWfileStatesHelper* statesHelper,
   ReadfileState* currentState);
extern void __FhgfsOpsCommKit_readfileV2bStageCLEANUP(RWfileStatesHelper* statesHelper,
   ReadfileState* currentState);

extern fhgfs_bool __FhgfsOpsCommKit_readfileV2bIsDataAvailable(RWfileStatesHelper* statesHelper,
   ReadfileState* currentState);
extern void __FhgfsOpsCommKit_readfileV2bPrepareRetry(RWfileStatesHelper* statesHelper,
   ReadfileState* currentState);
extern void __FhgfsOpsCommKit_readfileV2bStartRetry(RWfileStatesHelper* statesHelper,
   ReadfileState* states, size_t numStates);


extern fhgfs_bool __FhgfsOpsCommKit_writefileV2bStagePREPARE(RWfileStatesHelper* statesHelper,
   WritefileState* currentState);
extern void __FhgfsOpsCommKit_writefileV2bStageSENDHEADER(RWfileStatesHelper* statesHelper,
   WritefileState* currentState);
extern void __FhgfsOpsCommKit_writefileV2bStageSENDDATA(RWfileStatesHelper* statesHelper,
   WritefileState* currentState);
extern void __FhgfsOpsCommKit_writefileV2bStageRECV(RWfileStatesHelper* statesHelper,
   WritefileState* currentState);
extern void __FhgfsOpsCommKit_writefileV2bStageSOCKETEXCEPTION(RWfileStatesHelper* statesHelper,
   WritefileState* currentState);
extern void __FhgfsOpsCommKit_writefileV2bStageSOCKETINVALIDATE(RWfileStatesHelper* statesHelper,
   WritefileState* currentState);
extern void __FhgfsOpsCommKit_writefileV2bStageCLEANUP(RWfileStatesHelper* statesHelper,
   WritefileState* currentState);

extern fhgfs_bool __FhgfsOpsCommKit_writefileV2bIsDataAvailable(RWfileStatesHelper* statesHelper,
   WritefileState* currentState);
extern void __FhgfsOpsCommKit_writefileV2bPrepareRetry(RWfileStatesHelper* statesHelper,
   WritefileState* currentState);
extern void __FhgfsOpsCommKit_writefileV2bStartRetry(RWfileStatesHelper* statesHelper,
   WritefileState* states, size_t numStates);



// inliners
static inline ReadfileState FhgfsOpsCommKit_assignReadfileState(char __user *buf, loff_t offset,
   size_t size, uint16_t targetID, char* msgBuf);
static inline WritefileState FhgfsOpsCommKit_assignWritefileState(const char __user *buf,
   loff_t offset, size_t size, uint16_t targetID, char* msgBuf);
static inline void FhgfsOpsCommKit_setWritefileStateUseBuddyMirrorSecond(
   fhgfs_bool useBuddyMirrorSecond, WritefileState* outState);
static inline void FhgfsOpsCommKit_setWritefileStateUseServersideMirroring(
   fhgfs_bool useServersideMirroring, WritefileState* outState);
static inline void FhgfsOpsCommKit_setWriteFileStateFirstWriteDone(
   fhgfs_bool firstWriteDoneForTarget, WritefileState* outState);
static inline void FhgfsOpsCommKit_setReadfileStateUseBuddyMirrorSecond(
   fhgfs_bool useBuddyMirrorSecond, ReadfileState* outState);
static inline void FhgfsOpsCommKit_setReadFileStateFirstWriteDone(
   fhgfs_bool firstWriteDoneForTarget, ReadfileState* outState);

static inline void __FhgfsOpsCommKit_readfileAddStateSockToPollSocks(
   RWfileStatesHelper* statesHelper, ReadfileState* currentState);
static inline void __FhgfsOpsCommKit_writefileAddStateSockToPollSocks(
   RWfileStatesHelper* statesHelper, WritefileState* currentState, short pollEvents);


struct ReadfileState
{
   // (set to Stage_PREPARE in the beginning, assigned by the state-creator)
   ReadfileStage stage; // the current stage of the individual communication process

   // data for spefic modes (will be assigned by the corresponding modes)
   size_t transmitted; // how much data has been transmitted already
   size_t toBeTransmitted; // how much data has to be transmitted
   size_t pollSocksIndex; // save the pollSocks-array index for the next round

   // data for all modes

   // (assigned by the state-creator)
   char __user *buf; // the data that shall be transferred
   loff_t offset; // target-node local offset
   size_t size; // size of the buf
   uint16_t targetID; // target ID
   char* msgBuf; // for serialization

   fhgfs_bool firstWriteDoneForTarget; /* true if a chunk was previously written to this target in
                                          this session; used for the session check */
   fhgfs_bool useBuddyMirrorSecond; // for buddy mirroring, this msg goes to secondary

   // (assigned by the preparation mode)
   Node* node; // target-node reference
   Socket* sock; // target-node connection

   // result data
   int64_t nodeResult; // the amount of data that has been read (or a negative fhgfs error code)
   int64_t expectedNodeResult; // the amount of data that we wanted to read

};

/**
 * Note: shares almost everything with ReadfileState, but deriving it from a common base-struct
 * would make the code (even more ;) unreadable
 */
struct WritefileState
{
   // (set to Stage_PREPARE in the beginning, assigned by the state-creator)
   WritefileStage stage; // the current stage of the individual communication process

   // data for specific stages (will be assigned by the corresponding stages)
   size_t transmitted; // how much data has been transmitted already
   size_t toBeTransmitted; // how much data has to be transmitted
   size_t pollSocksIndex; // save the pollSocks-array index for the next round

   // data for all stages

   // (assigned by the state-creator)
   const char __user *buf; // the data that shall be transferred
   loff_t offset; // target-node local offset
   size_t size; // size of the buf
   uint16_t targetID; // target ID
   char* msgBuf; // for serialization

   fhgfs_bool firstWriteDoneForTarget; /* true if data was previously written to this target in
                                          this session; used for the session check */
   fhgfs_bool useBuddyMirrorSecond; // for buddy mirroring, this msg goes to secondary
   fhgfs_bool useServersideMirroring; // data shall be forwarded to mirror by primary


   // (assigned by the preparation stage)
   Node* node; // target-node reference
   Socket* sock; // target-node connection
   WriteLocalFileRespMsg writeRespMsg; // response message

   // result data
   int64_t nodeResult; // the amount of data that has been written (or a negative fhgfs error code)
   int64_t expectedNodeResult; // the amount of data that we wanted to write
};


/**
 * Note: Initializes the expectedNodeResult attribute from the size argument
 */
ReadfileState FhgfsOpsCommKit_assignReadfileState(char __user *buf, loff_t offset, size_t size,
   uint16_t targetID, char* msgBuf)
{
   ReadfileState state =
   {
      .stage = ReadfileStage_PREPARE,

      .buf = buf,
      .offset = offset,
      .size = size,
      .targetID = targetID,
      .msgBuf = msgBuf,

      .firstWriteDoneForTarget = fhgfs_false, // set via separate setter method
      .useBuddyMirrorSecond = fhgfs_false, // set via separate setter method

      .nodeResult = -FhgfsOpsErr_INTERNAL,
      .expectedNodeResult = size,

   };

   return state;
}

/**
 * Note: Initializes the expectedNodeResult attribute from the size argument
 * Note: defaults to server-side mirroring enabled.
 */
WritefileState FhgfsOpsCommKit_assignWritefileState(const char __user *buf, loff_t offset,
   size_t size, uint16_t targetID, char* msgBuf)
{
   WritefileState state =
   {
      .stage = WritefileStage_PREPARE,

      .buf = buf,
      .offset = offset,
      .size = size,
      .targetID = targetID,
      .msgBuf = msgBuf,

      .firstWriteDoneForTarget = fhgfs_false, // set via separate setter method
      .useBuddyMirrorSecond = fhgfs_false, // set via separate setter method
      .useServersideMirroring = fhgfs_true, // set via separate setter method

      .nodeResult = -FhgfsOpsErr_INTERNAL,
      .expectedNodeResult = size,
   };

   return state;
}

void FhgfsOpsCommKit_setWritefileStateUseBuddyMirrorSecond(fhgfs_bool useBuddyMirrorSecond,
   WritefileState* outState)
{
   outState->useBuddyMirrorSecond = useBuddyMirrorSecond;
}

void FhgfsOpsCommKit_setWritefileStateUseServersideMirroring(fhgfs_bool useServersideMirroring,
   WritefileState* outState)
{
   outState->useServersideMirroring = useServersideMirroring;
}

void FhgfsOpsCommKit_setWriteFileStateFirstWriteDone(fhgfs_bool firstWriteDoneForTarget,
   WritefileState* outState)
{
   outState->firstWriteDoneForTarget = firstWriteDoneForTarget;
}

void FhgfsOpsCommKit_setReadfileStateUseBuddyMirrorSecond(fhgfs_bool useBuddyMirrorSecond,
   ReadfileState* outState)
{
   outState->useBuddyMirrorSecond = useBuddyMirrorSecond;
}

void FhgfsOpsCommKit_setReadFileStateFirstWriteDone(fhgfs_bool firstWriteDoneForTarget,
   ReadfileState* outState)
{
   outState->firstWriteDoneForTarget = firstWriteDoneForTarget;
}

void __FhgfsOpsCommKit_readfileAddStateSockToPollSocks(
   RWfileStatesHelper* statesHelper, ReadfileState* currentState)
{
   (statesHelper->pollSocks)[statesHelper->numPollSocks] =
      __FhgfsOpsCommKitCommon_assignPollSocket(currentState->sock, BEEGFS_POLLIN);
   currentState->pollSocksIndex = statesHelper->numPollSocks;
   statesHelper->numPollSocks++;
}

/**
 * @param pollEvents the events to poll for (e.g. BEEGFS_POLLIN or BEEGFS_POLLOUT)
 */
void __FhgfsOpsCommKit_writefileAddStateSockToPollSocks(
   RWfileStatesHelper* statesHelper, WritefileState* currentState, short pollEvents)
{
   (statesHelper->pollSocks)[statesHelper->numPollSocks] =
      __FhgfsOpsCommKitCommon_assignPollSocket(currentState->sock, pollEvents);
   currentState->pollSocksIndex = statesHelper->numPollSocks;
   statesHelper->numPollSocks++;
}


#endif /*FHGFSOPSCOMMKIT_H_*/
