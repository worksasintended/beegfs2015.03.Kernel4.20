#include <common/net/message/storage/mirroring/ResyncLocalFileRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <net/msghelpers/MsgHelperIO.h>
#include <toolkit/StorageTkEx.h>

#include <program/Program.h>

#include "ResyncLocalFileMsgEx.h"

bool ResyncLocalFileMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   #ifdef BEEGFS_DEBUG
      const char* logContext = "ResyncLocalFileMsg incoming";
      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, Log_DEBUG,
         std::string("Received a ResyncLocalFileMsg from: ") + peer);
   #endif // BEEGFS_DEBUG

   App* app = Program::getApp();
   ChunkStore* chunkStore = app->getChunkDirStore();
   StorageTargets* storageTargets = app->getStorageTargets();
   FhgfsOpsErr retVal = FhgfsOpsErr_SUCCESS;

   const char* dataBuf = getDataBuf();
   uint16_t targetID = getResyncToTargetID();
   size_t count = getCount();
   int64_t offset = getOffset();
   std::string relativeChunkPathStr = getRelativePathStr();
   int writeErrno;
   bool writeRes;

   int openFlags = O_WRONLY | O_CREAT;
   SessionQuotaInfo quotaInfo(false, false, 0, 0);
  // mode_t fileMode = STORAGETK_DEFAULTCHUNKFILEMODE;

   int targetFD = app->getTargetFD(targetID, true);
   int fd;

   // always truncate when we write the very first block of a file
   if(!offset && !isMsgHeaderFeatureFlagSet (RESYNCLOCALFILEMSG_FLAG_NODATA) )
      openFlags |= O_TRUNC;

   FhgfsOpsErr openRes = chunkStore->openChunkFile(targetFD, NULL, relativeChunkPathStr, true,
      openFlags, &fd, &quotaInfo);

   if (openRes != FhgfsOpsErr_SUCCESS)
   {
      LogContext(__func__).logErr(
         "Error resyncing chunk; Could not open FD; chunkPath: "
            + relativeChunkPathStr);
      retVal = FhgfsOpsErr_PATHNOTEXISTS;

      storageTargets->setState(targetID, TargetConsistencyState_BAD);

      goto send_response;
   }

   if(isMsgHeaderFeatureFlagSet (RESYNCLOCALFILEMSG_FLAG_NODATA)) // do not sync actual data
      goto set_attribs;

   if(isMsgHeaderFeatureFlagSet (RESYNCLOCALFILEMSG_CHECK_SPARSE))
      writeRes = doWriteSparse(fd, dataBuf, count, offset, writeErrno);
   else
      writeRes = doWrite(fd, dataBuf, count, offset, writeErrno);

   if(unlikely(!writeRes) )
   { // write error occured (could also be e.g. disk full)
      LogContext(__func__).logErr(
         "Error resyncing chunk; chunkPath: " + relativeChunkPathStr + "; error: "
            + System::getErrString(writeErrno));

      storageTargets->setState(targetID, TargetConsistencyState_BAD);

      retVal = fhgfsErrFromSysErr(writeErrno);
   }

   if(isMsgHeaderFeatureFlagSet (RESYNCLOCALFILEMSG_FLAG_TRUNC))
   {
      int truncErrno;
      // we trunc after a possible write, so we need to trunc at offset+count
      bool truncRes = doTrunc(fd, offset + count, truncErrno);

      if(!truncRes)
      {
         LogContext(__func__).logErr(
            "Error resyncing chunk; chunkPath: " + relativeChunkPathStr + "; error: "
               + System::getErrString(truncErrno));

         storageTargets->setState(targetID, TargetConsistencyState_BAD);

         retVal = fhgfsErrFromSysErr(truncErrno);
      }
   }

set_attribs:

   if(isMsgHeaderFeatureFlagSet(RESYNCLOCALFILEMSG_FLAG_SETATTRIBS) &&
      (retVal == FhgfsOpsErr_SUCCESS))
   {
      SettableFileAttribs* attribs = getChunkAttribs();
      // update mode
      int chmodRes = fchmod(fd, attribs->mode);
      if(chmodRes == -1)
      { // could be an error
         int errCode = errno;

         if(errCode != ENOENT)
         { // unhandled chmod() error
            LogContext(__func__).logErr("Unable to change file mode: " + relativeChunkPathStr
               + ". SysErr: " + System::getErrString());
         }
      }

      // update UID and GID...
      int chownRes = fchown(fd, attribs->userID, attribs->groupID);
      if(chownRes == -1)
      { // could be an error
         int errCode = errno;

         if(errCode != ENOENT)
         { // unhandled chown() error
            LogContext(__func__).logErr( "Unable to change file owner: " + relativeChunkPathStr
               + ". SysErr: " + System::getErrString());
         }
      }

      if((chmodRes == -1) || (chownRes == -1))
      {
         storageTargets->setState(targetID, TargetConsistencyState_BAD);
         retVal = FhgfsOpsErr_INTERNAL;
      }
   }

   close(fd);


send_response:

   ResyncLocalFileRespMsg respMsg(retVal);
   respMsg.serialize(respBuf, bufLen);
   sock->send(respBuf, respMsg.getMsgLength(), 0);

   return true;
}

/**
 * Write until everything was written (handle short-writes) or an error occured
 */
bool ResyncLocalFileMsgEx::doWrite(int fd, const char* buf, size_t count, off_t offset,
   int& outErrno)
{
   size_t sumWriteRes = 0;

   do
   {
      ssize_t writeRes =
         MsgHelperIO::pwrite(fd, buf + sumWriteRes, count - sumWriteRes, offset + sumWriteRes);

      if (unlikely(writeRes == -1) )
      {
         sumWriteRes = (sumWriteRes > 0) ? sumWriteRes : writeRes;
         outErrno = errno;
         return false;
      }

      sumWriteRes += writeRes;

   } while (sumWriteRes != count);

   return true;
}

/**
 * Write until everything was written (handle short-writes) or an error occured
 */
bool ResyncLocalFileMsgEx::doWriteSparse(int fd, const char* buf, size_t count, off_t offset,
   int& outErrno)
{
   size_t sumWriteRes = 0;
   const char zeroBuf[ RESYNCER_SPARSE_BLOCK_SIZE ] = { 0 };

   do
   {
      size_t cmpLen = BEEGFS_MIN(count - sumWriteRes, RESYNCER_SPARSE_BLOCK_SIZE);
      int cmpRes = memcmp(buf + sumWriteRes, zeroBuf, cmpLen);

      if(!cmpRes)
      { // sparse area
         sumWriteRes += cmpLen;

         if(sumWriteRes == count)
         { // end of buf
            // we must trunc here because this might be the end of the file
            int truncRes = ftruncate(fd, offset+count);

            if(unlikely(truncRes == -1) )
            {
               outErrno = errno;
               return false;
            }
         }
      }
      else
      { // non-sparse area
         ssize_t writeRes = MsgHelperIO::pwrite(fd, buf + sumWriteRes, cmpLen,
            offset + sumWriteRes);

         if(unlikely(writeRes == -1))
         {
            outErrno = errno;
            return false;
         }

         sumWriteRes += writeRes;
      }

   } while (sumWriteRes != count);

   return true;
}

bool ResyncLocalFileMsgEx::doTrunc(int fd, off_t length, int& outErrno)
{
   int truncRes = ftruncate(fd, length);

   if (truncRes == -1)
   {
      outErrno = errno;
      return false;
   }

   return true;
}



/**
 * @param errCode positive(!) system error code
 */
FhgfsOpsErr ResyncLocalFileMsgEx::fhgfsErrFromSysErr(int64_t errCode)
{
   switch(errCode)
   {
      case 0:
      {
         return FhgfsOpsErr_SUCCESS;
      } break;

      case ENOSPC:
      {
         return FhgfsOpsErr_NOSPACE;
      } break;

      case EFBIG:
      {
         return FhgfsOpsErr_TOOBIG;
      }

      case EINVAL:
      {
         return FhgfsOpsErr_INVAL;
      }

      case ENOENT:
      {
         return FhgfsOpsErr_PATHNOTEXISTS;
      }
   }

   return FhgfsOpsErr_INTERNAL;
}
