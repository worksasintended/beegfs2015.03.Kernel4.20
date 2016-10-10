#include <program/Program.h>
#include <common/storage/StorageErrors.h>
#include <common/toolkit/SessionTk.h>
#include <common/toolkit/VersionTk.h>
#include <net/msghelpers/MsgHelperIO.h>
#include <toolkit/StorageTkEx.h>
#include "ReadLocalFileV2MsgEx.h"

#include <sys/sendfile.h>
#include <sys/mman.h>


#define READ_USE_TUNEFILEREAD_TRIGGER   (4*1024*1024)  /* seq IO trigger for tuneFileReadSize */

#define READ_BUF_OFFSET_PROTO_MIN   (sizeof(int64_t) ) /* for prepended length info */
#define READ_BUF_END_PROTO_MIN      (sizeof(int64_t) ) /* for appended length info */


/* reserve more than necessary at buf start to achieve page cache alignment */
const size_t READ_BUF_OFFSET =
   BEEGFS_MAX( (long)READ_BUF_OFFSET_PROTO_MIN, sysconf(_SC_PAGESIZE) );
/* reserve more than necessary at buf end to achieve page cache alignment */
const size_t READ_BUF_END_RESERVE =
   BEEGFS_MAX( (long)READ_BUF_END_PROTO_MIN, sysconf(_SC_PAGESIZE) );
/* read buffer size cutoff for protocol data */
const size_t READ_BUF_LEN_PROTOCOL_CUTOFF =
   READ_BUF_OFFSET + READ_BUF_END_RESERVE;


bool ReadLocalFileV2MsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   const char* logContext = "ReadChunkFileV2Msg incoming";

   bool retVal = true; // return value

   #ifdef BEEGFS_DEBUG
      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, Log_DEBUG,
         std::string("Received a ReadLocalFileV2Msg from: ") + peer);

//      LOG_DEBUG(logContext, Log_SPAM,
//         std::string("Values for sessionID/FD/offset/count: ") +
//         std::string(getSessionID() ) + "/" + StringTk::intToStr(getFD() ) + "/" +
//         StringTk::intToStr(getOffset() ) + "/" + StringTk::intToStr(getCount() ) );
   #endif // BEEGFS_DEBUG

   int64_t readRes = 0;

   std::string fileHandleID(getFileHandleID() );
   bool isMirrorSession = isMsgHeaderFeatureFlagSet(READLOCALFILEMSG_FLAG_BUDDYMIRROR);

   // do session check only when it is not a mirror session
   bool useSessionCheck = isMirrorSession ? false :
      isMsgHeaderFeatureFlagSet(READLOCALFILEMSG_FLAG_SESSION_CHECK);

   App* app = Program::getApp();
   SessionStore* sessions = app->getSessions();
   Session* session = sessions->referenceSession(getSessionID(), true);
   this->sessionLocalFiles = session->getLocalFiles();

   // select the right targetID

   uint16_t targetID = getTargetID();

   if(isMirrorSession )
   { // given targetID refers to a buddy mirror group
      MirrorBuddyGroupMapper* mirrorBuddies = app->getMirrorBuddyGroupMapper();

      targetID = isMsgHeaderFeatureFlagSet(READLOCALFILEMSG_FLAG_BUDDYMIRROR_SECOND) ?
         mirrorBuddies->getSecondaryTargetID(targetID) :
         mirrorBuddies->getPrimaryTargetID(targetID);

      // note: only log message here, error handling will happen below through invalid targetFD
      if(unlikely(!targetID) )
         LogContext(logContext).logErr("Invalid mirror buddy group ID: " +
            StringTk::uintToStr(getTargetID() ) );
   }

   // check if we already have a session for this file...

   this->sessionLocalFile = sessionLocalFiles->referenceSession(
      fileHandleID, targetID, isMirrorSession);
   if(!sessionLocalFile)
   { // sessionLocalFile not exists yet => create, insert, re-get it
      if(useSessionCheck)
      { // server crashed during the write, maybe lost some data send error to client
         LogContext log("ReadChunkFileV2Msg incoming");
         log.log(Log_WARNING, "Potential cache loss for open file handle. (Server crash detected.) "
            "No session for file available. "
            "FileHandleID: " + fileHandleID);

         sendLengthInfo(sock, -FhgfsOpsErr_STORAGE_SRV_CRASHED);
         goto release_session;
      }

      std::string fileID = SessionTk::fileIDFromHandleID(fileHandleID);
      int openFlags = SessionTk::sysOpenFlagsFromFhgfsAccessFlags(getAccessFlags() );
      
      this->sessionLocalFile = new SessionLocalFile(fileHandleID, targetID, fileID, openFlags,
         false);

      if(isMirrorSession)
         sessionLocalFile->setIsMirrorSession(true);

      sessionLocalFile = sessionLocalFiles->addAndReferenceSession(sessionLocalFile);
   }
   else
   { // session file exists
      if(useSessionCheck && sessionLocalFile->isServerCrashed() )
      { // server crashed during the write, maybe lost some data send error to client
         LogContext log("ReadChunkFileV2Msg incoming");
         log.log(Log_SPAM, "Potential cache loss for open file handle. (Server crash detected.) "
            "The session is marked as dirty. "
            "FileHandleID: " + fileHandleID);

         sendLengthInfo(sock, -FhgfsOpsErr_STORAGE_SRV_CRASHED);
         goto release_session;
      }
   }

   this->needFileUnlockAndRelease = true;

   /* Note: the session file must be unlocked/released before we send the finalizing info,
       because otherwise we have a race when the client assumes the read is complete and tries
       to close the file (while the handle is actually still referenced on the server). */
   /* Note: we also must be careful to update the current offset before sending the final length
       info because otherwise the session file might have been released already and we have no
       longer access to the offset. */

   readRes = -1;
   try
   {
      // prepare file descriptor (if file not open yet then open it if it exists already)
      FhgfsOpsErr openRes = openFile(sessionLocalFile);
      if(openRes != FhgfsOpsErr_SUCCESS)
      {
         releaseFile();
         sendLengthInfo(sock, -openRes);
         goto release_session;
      }

      // check if file exists
      if(sessionLocalFile->getFD() == -1)
      { // file didn't exist (not an error) => send EOF
         releaseFile();
         sendLengthInfo(sock, 0);
         goto release_session;
      }

      // the actual read workhorse...
      
      readRes = incrementalReadStatefulAndSendV2(
         sock, respBuf, bufLen, sessionLocalFile, stats);

      LOG_DEBUG(logContext, Log_SPAM, "sending completed. "
         "readRes: " + StringTk::int64ToStr(readRes) );
      IGNORE_UNUSED_VARIABLE(readRes);

   }
   catch(SocketException& e)
   {
      LogContext(logContext).logErr(std::string("SocketException occurred: ") + e.what() );
      LogContext(logContext).log(Log_WARNING, "Details: "
         "sessionID: " + std::string(getSessionID() ) + "; "
         "fileHandle: " + fileHandleID + "; "
         "offset: " + StringTk::int64ToStr(getOffset() ) + "; "
         "count: " + StringTk::int64ToStr(getCount() ) );

      if(this->needFileUnlockAndRelease)
      {
         sessionLocalFile->setOffset(-1); /* invalidate offset (we can only do this if still locked,
            but that's not a prob if we update offset correctly before send - see notes above) */

         sessionLocalFiles->releaseSession(sessionLocalFile);
      }

      retVal = false;
      goto release_session;
   }

   
   if(unlikely(this->needFileUnlockAndRelease) )
   { /* this typically shoudn't be called here because then we would have a potential client race
        (see notes above) */
      sessionLocalFiles->releaseSession(sessionLocalFile);
   }

release_session:

   sessions->releaseSession(session);

   // update operation counters

   if(likely(readRes > 0) )
      app->getNodeOpStats()->updateNodeOp(
         sock->getPeerIP(), StorageOpCounter_READOPS, readRes, getMsgHeaderUserID() );

   return retVal;
}


/**
 * Note: This is similar to incrementalReadAndSend, but uses the offset from sessionLocalFile
 * to avoid calling seek every time.
 * Note: This is Version 2, it uses a preceding length info for each chunk.
 *
 * Warning: Do not use the returned value to set the new offset, as there might be other threads
 * that also did something with the file (i.e. the io-lock is released somewhere within this
 * method).
 *
 * @return number of bytes read or some arbitrary negative value otherwise
 */
int64_t ReadLocalFileV2MsgEx::incrementalReadStatefulAndSendV2(Socket* sock,
   char* buf, size_t bufLen, SessionLocalFile* sessionLocalFile, HighResolutionStats* stats)
  
{
   /* note on protocol: this works by sending an int64 before each data chunk, which contains the
      length of the next data chunk; or a zero if no more data can be read; or a negative fhgfs
      error code in case of an error */

   /* note on session offset: the session offset must always be set before sending the data to the
      client (otherwise the client could send the next request before we updated the offset, which
      would lead to a race condition) */

   const char* logContext = "ReadChunkFileV2Msg (read incremental)";
   Config* cfg = Program::getApp()->getConfig();

   char* dataBuf = &buf[READ_BUF_OFFSET]; // offset for prepended data length info
   char* sendBuf = dataBuf - READ_BUF_OFFSET_PROTO_MIN;

   const ssize_t dataBufLen = bufLen - READ_BUF_LEN_PROTOCOL_CUTOFF; /* cutoff for prepended and
      finalizing length info */

   int fd = sessionLocalFile->getFD();
   int64_t oldOffset = sessionLocalFile->getOffset();
   int64_t newOffset = getOffset();

   bool skipReadAhead =
      unlikely(isMsgHeaderFeatureFlagSet(READLOCALFILEMSG_FLAG_DISABLE_IO) ||
      sessionLocalFile->getIsDirectIO());

   ssize_t readAheadSize = skipReadAhead ? 0 : cfg->getTuneFileReadAheadSize();
   ssize_t readAheadTriggerSize = cfg->getTuneFileReadAheadTriggerSize();

   if( (oldOffset < 0) || (oldOffset != newOffset) )
   {
      sessionLocalFile->resetReadCounter(); // reset sequential read counter
      sessionLocalFile->resetLastReadAheadTrigger();
   }
   else
   { // read continues at previous offset
      LOG_DEBUG(logContext, Log_SPAM,
         "fileID: " + sessionLocalFile->getFileID() + "; "
         "offset: " + StringTk::int64ToStr(getOffset() ) );
   }


   uint64_t toBeRead = getCount();
   size_t maxReadAtOnceLen = dataBufLen;

   // reduce maxReadAtOnceLen to achieve better read/send aync overlap
   /* (note: reducing makes only sense if we can rely on the kernel to do some read-ahead, so don't
      reduce for direct IO and for random IO) */
   if( (sessionLocalFile->getReadCounter() >= READ_USE_TUNEFILEREAD_TRIGGER) &&
       !sessionLocalFile->getIsDirectIO() )
      maxReadAtOnceLen = BEEGFS_MIN(dataBufLen, cfg->getTuneFileReadSize() );


   off_t readOffset = getOffset();

   for( ; ; )
   {
      ssize_t readLength = BEEGFS_MIN(maxReadAtOnceLen, toBeRead);

      ssize_t readRes = unlikely(isMsgHeaderFeatureFlagSet(READLOCALFILEMSG_FLAG_DISABLE_IO) ) ?
         readLength : MsgHelperIO::pread(fd, dataBuf, readLength,  readOffset);

      LOG_DEBUG(logContext, Log_SPAM,
         "toBeRead: " + StringTk::int64ToStr(toBeRead) + "; "
         "readLength: " + StringTk::int64ToStr(readLength) + "; "
         "readRes: " + StringTk::int64ToStr(readRes) );

      if(readRes == readLength)
      { // simple success case
         toBeRead -= readRes;

         readOffset += readRes;

         int64_t newOffset = getOffset() + getCount() - toBeRead;
         sessionLocalFile->setOffset(newOffset); // update offset

         sessionLocalFile->incReadCounter(readRes); // update sequential read length

         stats->incVals.diskReadBytes += readRes; // update stats

         bool isFinal = !toBeRead;

         sendLengthInfoAndData(sock, readRes, sendBuf, isFinal);

         checkAndStartReadAhead(sessionLocalFile, readAheadTriggerSize, newOffset, readAheadSize);

         if(isFinal)
         { // we reached the end of the requested data
            releaseFile();

            return getCount();
         }
      }
      else
      { // readRes not as it should be => might be an error or just an end-of-file

         if(readRes == -1)
         { // read error occurred
            LogContext(logContext).log(Log_WARNING, "Unable to read file data. "
               "FileID: " + sessionLocalFile->getFileID() + "; "
               "SysErr: " + System::getErrString() );

            sessionLocalFile->setOffset(-1);
            releaseFile();
            sendLengthInfo(sock, -FhgfsOpsErr_INTERNAL);
            return -1;
         }
         else
         { // just an end of file
            LOG_DEBUG(logContext, Log_DEBUG,
               "Unable to read all of the requested data (=> end of file)");
            LOG_DEBUG(logContext, Log_DEBUG,
               "offset: " + StringTk::int64ToStr(getOffset() ) + "; "
               "count: " + StringTk::int64ToStr(getCount() ) + "; "
               "readLength: " + StringTk::int64ToStr(readLength) + "; " +
               "readRes: " + StringTk::int64ToStr(readRes) + "; " +
               "toBeRead: " + StringTk::int64ToStr(toBeRead) );

            readOffset += readRes;
            toBeRead -= readRes;

            sessionLocalFile->setOffset(getOffset() + getCount() - toBeRead); // update offset

            sessionLocalFile->incReadCounter(readRes); // update sequential read length

            stats->incVals.diskReadBytes += readRes; // update stats

            releaseFile();

            if(readRes > 0)
               sendLengthInfoAndData(sock, readRes, sendBuf, true);
            else
               sendLengthInfo(sock, 0);

            return(getCount() - toBeRead);
         }

      }

   } // end of for-loop

   return(getCount() - toBeRead);
}

/**
 * Starts read-ahead if enough sequential data has been read.
 *
 * Note: if getDisableIO() is true, we assume the caller sets readAheadSize==0, so getDisableIO()
 * is not checked explicitly within this function.
 *
 * @sessionLocalFile lastReadAheadOffset will be updated if read-head was triggered
 * @param readAheadTriggerSize the length of sequential IO that triggers read-ahead
 * @param currentOffset current file offset (where read-ahead would start)
 */
void ReadLocalFileV2MsgEx::checkAndStartReadAhead(SessionLocalFile* sessionLocalFile,
   ssize_t readAheadTriggerSize, off_t currentOffset, off_t readAheadSize)
{
   const char* logContext = "ReadChunkFileV2Msg (read-ahead)";

   if(!readAheadSize)
      return;

   int64_t readCounter = sessionLocalFile->getReadCounter();
   int64_t nextReadAheadTrigger = sessionLocalFile->getLastReadAheadTrigger() ?
      sessionLocalFile->getLastReadAheadTrigger() + readAheadSize : readAheadTriggerSize;

   if(readCounter < nextReadAheadTrigger)
      return; // we're not at the trigger point yet

   /* start read-head...
      (read-ahead is supposed to be non-blocking if there are free slots in the device IO queue) */

   LOG_DEBUG(logContext, Log_SPAM,
      std::string("Starting read-ahead... ") +
      "offset: " + StringTk::int64ToStr(currentOffset) + "; "
      "size: " + StringTk::int64ToStr(readAheadSize) );
   IGNORE_UNUSED_VARIABLE(logContext);

   MsgHelperIO::readAhead(sessionLocalFile->getFD(), currentOffset, readAheadSize);

   // update trigger

   sessionLocalFile->setLastReadAheadTrigger(readCounter);
}


/**
 * Open the file if a filedescriptor is not already set in sessionLocalFile.
 * If the file needs to be opened, this method will check the target consistency state before
 * opening.
 *
 * @return we return the special value FhgfsOpsErr_COMMUNICATION here in some cases to indirectly
 * ask the client for a retry (e.g. if target consistency is not good for buddymirrored chunks).
 */
FhgfsOpsErr ReadLocalFileV2MsgEx::openFile(SessionLocalFile* sessionLocalFile)
{
   const char* logContext = "ReadChunkFileV2Msg (open)";

   App* app = Program::getApp();

   int actualTargetID = sessionLocalFile->getTargetID();
   bool isBuddyMirrorChunk = sessionLocalFile->getIsMirrorSession();
   TargetConsistencyState consistencyState = TargetConsistencyState_BAD; // silence warning


   if(sessionLocalFile->getFD() != -1)
      return FhgfsOpsErr_SUCCESS; // file already open => nothing to be done here


   // file not open yet => get targetFD and check consistency state

   int targetFD = app->getTargetFDAndConsistencyState(actualTargetID, isBuddyMirrorChunk,
      &consistencyState);

   if(unlikely(targetFD == -1) )
   { // unknown targetID
      if(isBuddyMirrorChunk)
      { /* buddy mirrored file => fail with Err_COMMUNICATION to make the requestor retry.
           mgmt will mark this target as (p)offline in a few moments. */
         LogContext(logContext).log(Log_NOTICE, "Refusing request. "
            "Unknown targetID: " + StringTk::uintToStr(actualTargetID) );

         return FhgfsOpsErr_COMMUNICATION;
      }

      LogContext(logContext).logErr(
         "Unknown targetID: " + StringTk::uintToStr(actualTargetID) );

      return FhgfsOpsErr_UNKNOWNTARGET;
   }

   if(unlikely(consistencyState != TargetConsistencyState_GOOD) && isBuddyMirrorChunk)
   { // this is a request for a buddymirrored chunk on a non-good target
      LogContext(logContext).log(Log_NOTICE, "Refusing request. Target consistency is not good. "
         "targetID: " + StringTk::uintToStr(actualTargetID) );;

      return FhgfsOpsErr_COMMUNICATION;
   }

   FhgfsOpsErr openChunkRes = sessionLocalFile->openFile(targetFD, getPathInfo(), false, NULL);

   return openChunkRes;
}


