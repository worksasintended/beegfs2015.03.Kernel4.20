#ifndef FHGFSOPSCOMMKITCOMMON_H_
#define FHGFSOPSCOMMKITCOMMON_H_

#include <app/log/Logger.h>
#include <common/net/message/session/rw/WriteLocalFileRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/SocketTk.h>
#include <common/Common.h>
#include <net/filesystem/RemotingIOInfo.h>
#include <nodes/NodeStoreEx.h>


#define BEEGFS_COMMKIT_MSGBUF_SIZE         4096
#define RW_FILE_STAGE_PREPARE             0 /* initial stage for commkit read/write
                                               (see Read-/WritefileStage) */

#define COMMKIT_DEBUG_READ_SEND      (1 << 0)
#define COMMKIT_DEBUG_READ_HEADER    (1 << 1)
#define COMMKIT_DEBUG_RECV_DATA      (1 << 2)
#define COMMKIT_DEBUG_WRITE_HEADER   (1 << 3)
#define COMMKIT_DEBUG_WRITE_POLL     (1 << 4)
#define COMMKIT_DEBUG_WRITE_SEND     (1 << 5)
#define COMMKIT_DEBUG_WRITE_RECV     (1 << 6)

// #define BEEGFS_COMMKIT_DEBUG 0b010

#ifndef BEEGFS_COMMKIT_DEBUG
#define BEEGFS_COMMKIT_DEBUG 0
#endif

#ifdef BEEGFS_COMMKIT_DEBUG
#define CommKitErrorInjectRate   101 // fail a send/receive req every X jiffies
#endif



enum ReadfileStage;
typedef enum ReadfileStage ReadfileStage;
enum WritefileStage;
typedef enum WritefileStage WritefileStage;

struct RWfileStatesHelper;
typedef struct RWfileStatesHelper RWfileStatesHelper;



static inline PollSocketEx __FhgfsOpsCommKitCommon_assignPollSocket(Socket* sock, short events);
static inline void __FhgfsOpsCommKitCommon_pollStateSocks(RWfileStatesHelper* statesHelper,
   size_t numStates);
static inline void __FhgfsOpsCommKitCommon_handlePollError(RWfileStatesHelper* statesHelper,
   int pollRes);
static inline const char* readFileStateEnumToStr(enum ReadfileStage stage);
static inline const char* writeFileStateEnumToStr(enum WritefileStage stage);



enum ReadfileStage
{
   ReadfileStage_PREPARE = RW_FILE_STAGE_PREPARE,
   ReadfileStage_SEND,
   ReadfileStage_RECVHEADER,
   ReadfileStage_RECVDATA,
   ReadfileStage_RECVFOOTER,
   ReadfileStage_SOCKETEXCEPTION,
   ReadfileStage_SOCKETINVALIDATE,
   ReadfileStage_CLEANUP,
   ReadfileStage_RETRYWAIT,
   ReadfileStage_HANDLEPAGES,
   ReadfileStage_DONE,

   ReadfileStage_LAST // only for error checking
};

static char* ReadfileStageStr[] =
{
   ENUMSTR(ReadfileStage_PREPARE),
   ENUMSTR(ReadfileStage_SEND),
   ENUMSTR(ReadfileStage_RECVHEADER),
   ENUMSTR(ReadfileStage_RECVDATA),
   ENUMSTR(ReadfileStage_RECVFOOTER),
   ENUMSTR(ReadfileStage_SOCKETEXCEPTION),
   ENUMSTR(ReadfileStage_SOCKETINVALIDATE),
   ENUMSTR(ReadfileStage_CLEANUP),
   ENUMSTR(ReadfileStage_RETRYWAIT),
   ENUMSTR(ReadfileStage_HANDLEPAGES)
   ENUMSTR(ReadfileStage_DONE),
};

enum WritefileStage
{
   WritefileStage_PREPARE = RW_FILE_STAGE_PREPARE,
   WritefileStage_SENDHEADER,
   WritefileStage_SENDDATA,
   WritefileStage_RECV,
   WritefileStage_SOCKETEXCEPTION,
   WritefileStage_SOCKETINVALIDATE,
   WritefileStage_CLEANUP,
   WritefileStage_RETRYWAIT,
   WritefileStage_HANDLEPAGES,
   WritefileStage_DONE,

   WritefileStage_LAST // only for error checking
};

static char* WritefileStageStr[] =
{
   ENUMSTR(WritefileStage_PREPARE),
   ENUMSTR(WritefileStage_SENDHEADER),
   ENUMSTR(WritefileStage_SENDDATA),
   ENUMSTR(WritefileStage_RECV),
   ENUMSTR(WritefileStage_SOCKETEXCEPTION),
   ENUMSTR(WritefileStage_SOCKETINVALIDATE),
   ENUMSTR(WritefileStage_CLEANUP),
   ENUMSTR(WritefileStage_RETRYWAIT),
   ENUMSTR(WritefileStage_HANDLEPAGES),
   ENUMSTR(WritefileStage_DONE),
};

/**
 * Additional data that is required or useful for all the states.
 * (This is shared states data.)
 *
 * invariant: (numRetryWaiters + numDone + numUnconnectable + numPollSocks) <= numStates
 */
struct RWfileStatesHelper
{
   App* app;
   Logger* log;
   char* logContext;

   RemotingIOInfo* ioInfo;

   size_t numRetryWaiters; // number of states with a communication error
   size_t numDone; // number of finished states

   size_t numAcquiredConns; // number of acquired conns to decide waiting or not (avoids deadlock)
   size_t numUnconnectable; // states that didn't get a conn this round (reset to 0 in each round)

   fhgfs_bool pollTimedOut;
   fhgfs_bool pollTimeoutLogged;
   fhgfs_bool connFailedLogged;

   PollSocketEx* pollSocks;
   size_t numPollSocks; // states with socks in pollSocks array (reset to 0 in each round)

   unsigned currentRetryNum; // number of used up retries (in case of comm errors)
   unsigned maxNumRetries; // 0 if infinite number of retries enabled
};

static inline const char* writeFileStateEnumToStr(enum WritefileStage stage)
{
   if (stage >= sizeof(WritefileStageStr) / sizeof(char*) )
   {
      return "Invalid WitefileStage enum";
   }

   return WritefileStageStr[stage];
}

static inline const char* readFileStateEnumToStr(enum ReadfileStage stage)
{
   if (stage >= ReadfileStage_LAST )
   {
      return "Invalid ReadfileStage enum";
   }

   return ReadfileStageStr[stage];
}



PollSocketEx __FhgfsOpsCommKitCommon_assignPollSocket(Socket* sock, short events)
{
   PollSocketEx pollSock =
   {
      .sock = sock,
      .events = events,
      .revents = 0,
   };

   return pollSock;
}

/**
 * Poll the state socks that need to be polled.
 * This will call _handlePollError() if something goes wrong with the poll(), e.g. timeout.
 *
 * Note: Caller must have checked that there actually are socks that need polling
 *    (statesHelper.numPollSocks>0) before calling this.
 */
void __FhgfsOpsCommKitCommon_pollStateSocks(RWfileStatesHelper* statesHelper, size_t numStates)
{
   int pollRes;

   size_t numWaiters = statesHelper->numPollSocks + statesHelper->numRetryWaiters +
      statesHelper->numDone + statesHelper->numUnconnectable;

   /* if not all the states are polling or done, some states must be ready to do something
      immediately => set timeout to 0 (non-blocking) */
   /* note: be very careful with timeoutMS==0, because that means we don't release the CPU,
      so we need to ensure that timeoutMS==0 is no permanent condition. */
   int timeoutMS = (numWaiters < numStates) ? 0 : CONN_LONG_TIMEOUT;

   BEEGFS_BUG_ON_DEBUG(numWaiters > numStates, "numWaiters > numStates should never happen");

   pollRes = SocketTk_pollEx(statesHelper->pollSocks, statesHelper->numPollSocks, timeoutMS);
   if(unlikely( (timeoutMS && pollRes <= 0) || (pollRes < 0) ) )
      __FhgfsOpsCommKitCommon_handlePollError(statesHelper, pollRes);
}

void __FhgfsOpsCommKitCommon_handlePollError(RWfileStatesHelper* statesHelper, int pollRes)
{
   // note: notifies the waiting sockets via statesHelper.pollTimedOut (they do not care whether
   // it was a timeout or an error)

   if(!statesHelper->pollTimeoutLogged)
   {
      if(!pollRes)
      { // no incoming data => timout or interrupted
         if(Thread_isSignalPending() )
         { // interrupted
            Logger_logFormatted(statesHelper->log, 3, statesHelper->logContext,
               "Communication (poll() ) interrupted by signal.");
         }
         else
         { // timout
            Logger_logErrFormatted(statesHelper->log, statesHelper->logContext,
               "Communication (poll() ) timeout for %d sockets.",
               (int)statesHelper->numPollSocks);
         }
      }
      else
      { // error
         Logger_logErrFormatted(statesHelper->log, statesHelper->logContext,
            "Communication (poll() ) error for %lld sockets. ErrCode: %d",
            (long long)statesHelper->numPollSocks, pollRes);
      }
   }

   statesHelper->pollTimeoutLogged = fhgfs_true;
   statesHelper->pollTimedOut = fhgfs_true;
}


#endif /*FHGFSOPSCOMMKITCOMMON_H_*/
