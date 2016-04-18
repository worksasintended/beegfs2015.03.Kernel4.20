#include <common/toolkit/serialization/Serialization.h>
#include <net/msghelpers/MsgHelperIO.h>
#include <nodes/NodeStoreEx.h>
#include <program/Program.h>

#include "SessionLocalFile.h"
#include <storage/ChunkStore.h>

unsigned SessionLocalFile::serialize(char* buf)
{
   size_t bufPos = 0;

   // removeOnRelease
   bufPos += Serialization::serializeBool(&buf[bufPos], this->removeOnRelease);

   // fileHandleID
   std::string fileHandleID = this->fileHandleID;
   bufPos += Serialization::serializeStr(&buf[bufPos], fileHandleID.length(), fileHandleID.c_str());

   // targetID
   bufPos += Serialization::serializeUShort(&buf[bufPos], this->targetID);

   // fileID
   std::string fileID = this->fileID;
   bufPos += Serialization::serializeStr(&buf[bufPos], fileID.length(), fileID.c_str() );

   // openFlags
   bufPos += Serialization::serializeInt(&buf[bufPos], this->openFlags);

   // offset
   bufPos += Serialization::serializeInt64(&buf[bufPos], this->offset);

   // isMirrorSession
   bufPos += Serialization::serializeBool(&buf[bufPos], this->isMirrorSession);

   // serverCrashed
   bufPos += Serialization::serializeBool(&buf[bufPos], this->serverCrashed);

   // mirror nodeNumID
   if(!this->mirrorNode)
      bufPos += Serialization::serializeUShort(&buf[bufPos], 0); //serialize an invalid nodeNumID
   else
      bufPos += Serialization::serializeUShort(&buf[bufPos], this->mirrorNode->getNumID() );

   // writeCounter
   bufPos += Serialization::serializeInt64(&buf[bufPos], this->writeCounter.read() );

   // readCounter
   bufPos += Serialization::serializeInt64(&buf[bufPos], this->readCounter.read() );

   // lastReadAheadTrigger
   bufPos += Serialization::serializeInt64(&buf[bufPos], this->lastReadAheadTrigger.read() );

   return bufPos;
}

bool SessionLocalFile::deserialize(const char* buf, size_t bufLen, unsigned* outLen)
{
   size_t bufPos = 0;

   {
      // removeOnRelease
      unsigned removeOnReleaseBufLen = 0;
      if(!Serialization::deserializeBool(&buf[bufPos], bufLen-bufPos, &this->removeOnRelease,
         &removeOnReleaseBufLen) )
         return false;

      bufPos += removeOnReleaseBufLen;
   }

   {
      // fileHandleID
      unsigned fileHandleIDLen = 0;
      const char* fileHandleID = NULL;
      unsigned fileHandleIDBufLen = 0;

      if(unlikely(!Serialization::deserializeStr(&buf[bufPos], bufLen-bufPos,
         &fileHandleIDLen, &fileHandleID, &fileHandleIDBufLen) ) )
         return false;

      this->fileHandleID = fileHandleID;
      bufPos += fileHandleIDBufLen;
   }

   {
      // targetID
      unsigned targetIDBufLen = 0;

      if(unlikely(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos, &this->targetID,
         &targetIDBufLen) ) )
         return false;

      bufPos += targetIDBufLen;
   }

   {
      // fileID
      unsigned fileIDLen = 0;
      const char* fileID = NULL;
      unsigned fileIDBufLen = 0;

      if(unlikely(!Serialization::deserializeStr(&buf[bufPos], bufLen-bufPos, &fileIDLen,
         &fileID, &fileIDBufLen) ) )
         return false;

      this->fileID = fileID;
      bufPos += fileIDBufLen;
   }

   {
      // openFlags
      unsigned openFlagsBufLen = 0;

      if(unlikely(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos, &this->openFlags,
         &openFlagsBufLen) ) )
         return false;

      bufPos += openFlagsBufLen;
   }

   {
      // offset
      unsigned offsetBufLen = 0;

      if(unlikely(!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos, &this->offset,
         &offsetBufLen) ) )
         return false;

      bufPos += offsetBufLen;
   }

   {
      // isMirrorSession
      unsigned isMirrorSessionBufLen = 0;

      if(unlikely(!Serialization::deserializeBool(&buf[bufPos], bufLen-bufPos,
         &this->isMirrorSession, &isMirrorSessionBufLen) ) )
         return false;

      bufPos += isMirrorSessionBufLen;
   }

   {
      // serverCrashed
      unsigned serverCrashedBufLen = 0;

      if(unlikely(!Serialization::deserializeBool(&buf[bufPos], bufLen-bufPos,
         &this->serverCrashed, &serverCrashedBufLen) ) )
         return false;

      bufPos += serverCrashedBufLen;
   }

   {
      // mirror nodeNumID
      uint16_t mirrorNodeID = 0;
      unsigned mirrorNodeBufLen = 0;

      if(unlikely(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos, &mirrorNodeID,
         &mirrorNodeBufLen) ) )
         return false;

      if(mirrorNodeID)
      {
         NodeStoreServersEx* nodeStore = Program::getApp()->getStorageNodes();
         Node* node = nodeStore->referenceNode(mirrorNodeID);

         if(!node)
            return false;

         this->mirrorNode = node;
      }

      bufPos += mirrorNodeBufLen;
   }

   {
      // writeCounter
      unsigned writeCounterBufLen = 0;
      int64_t writeCounter;

      if(unlikely(!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos, &writeCounter,
         &writeCounterBufLen) ) )
         return false;

      this->writeCounter.set(writeCounter);
      bufPos += writeCounterBufLen;
   }

   {
      // readCounter
      unsigned readCounterBufLen = 0;
      int64_t readCounter;

      if(unlikely(!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos, &readCounter,
         &readCounterBufLen) ) )
         return false;

      this->readCounter.set(readCounter);
      bufPos += readCounterBufLen;
   }

   {
      // lastReadAheadTrigger
      unsigned lastReadAheadTriggerBufLen = 0;
      int64_t lastReadAheadTrigger;

      if(unlikely(!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos,
         &lastReadAheadTrigger, &lastReadAheadTriggerBufLen) ) )
         return false;

      this->lastReadAheadTrigger.set(lastReadAheadTrigger);
      bufPos += lastReadAheadTriggerBufLen;
   }

   *outLen = bufPos;

   return true;
}

unsigned SessionLocalFile::serialLen()
{
   size_t bufPos = 0;

   bufPos += Serialization::serialLenBool();                           // removeOnRelease
   bufPos += Serialization::serialLenStr(this->fileHandleID.length()); // fileHandleID
   bufPos += Serialization::serialLenUShort();                         // targetID
   bufPos += Serialization::serialLenStr(this->fileID.length());       // fileID
   bufPos += Serialization::serialLenInt();                            // openFlags
   bufPos += Serialization::serialLenInt64();                          // offset
   bufPos += Serialization::serialLenBool();                           // isMirrorSession
   bufPos += Serialization::serialLenBool();                           // serverCrashed
   bufPos += Serialization::serialLenUShort();                         // mirrorNode, only nodeNumID
   bufPos += Serialization::serialLenInt64();                          // writeCounter
   bufPos += Serialization::serialLenInt64();                          // readCounter
   bufPos += Serialization::serialLenInt64();                          // lastReadAheadTrigger

   return bufPos;
}

/**
 * Open a chunkFile for this session
 *
 * @param quotaInfo may be NULL if not isWriteOpen (i.e. if the file will not be created).
 * @param isWriteOpen if set to true, the file will be created if it didn't exist.
 */
FhgfsOpsErr SessionLocalFile::openFile(int targetFD, PathInfo *pathInfo,
   bool isWriteOpen, SessionQuotaInfo* quotaInfo)
{
   FhgfsOpsErr retVal = FhgfsOpsErr_SUCCESS;

   App* app = Program::getApp();
   Logger* log = app->getLogger();
   const char* logContext = "SessionLocalFile (open)";


   if(this->fileDescriptor != -1) // no lock here as optimization, with lock below
      return FhgfsOpsErr_SUCCESS; // file already open


   SafeMutexLock safeMutex(&this->sessionMutex); // L O C K


   if(this->fileDescriptor != -1)
   {
      // file already open (race with another thread) => nothing more to do here
   }
   else
   {  // open chunk file (and create dir if necessary)...

      std::string entryID = getFileID();
      PathVec chunkDirPath;
      std::string chunkFilePathStr;
      bool hasOrigFeature = pathInfo->hasOrigFeature();

      StorageTk::getChunkDirChunkFilePath(pathInfo, entryID, hasOrigFeature, chunkDirPath,
         chunkFilePathStr);

      ChunkStore* chunkDirStore = app->getChunkDirStore();

      int fd = -1;

      if (isWriteOpen)
      {  // chunk needs to be created if not exists
         int openFlags = O_CREAT | this->openFlags;

         FhgfsOpsErr openChunkRes = chunkDirStore->openChunkFile(
            targetFD, &chunkDirPath, chunkFilePathStr, hasOrigFeature, openFlags, &fd, quotaInfo);

         // fix chunk path permissions
         if (unlikely(openChunkRes == FhgfsOpsErr_NOTOWNER && quotaInfo->useQuota) )
         {
            // it already logs a message, so need to further check this ret value
            chunkDirStore->chmodV2ChunkDirPath(targetFD, &chunkDirPath, entryID);

            openChunkRes = chunkDirStore->openChunkFile(
               targetFD, &chunkDirPath, chunkFilePathStr, hasOrigFeature, openFlags, &fd,
               quotaInfo);
         }

         if (openChunkRes != FhgfsOpsErr_SUCCESS)
         {
            if (openChunkRes == FhgfsOpsErr_INTERNAL) // only log unhandled errors
               LogContext(logContext).logErr("Failed to open chunkFile: " + chunkFilePathStr);

            retVal = openChunkRes;
         }
      }
      else
      {  // just reading the file, no create
         mode_t openMode = S_IRWXU|S_IRWXG|S_IRWXO;

         fd = MsgHelperIO::openat(targetFD, chunkFilePathStr.c_str(), this->openFlags,
            openMode);

         if(fd == -1)
         { // not exists or error
            int errCode = errno;

            // we ignore ENOENT (file does not exist), as that's not an error
            if(errCode != ENOENT)
            {
               log->logErr(logContext, "Unable to open file: " + chunkFilePathStr + ". " +
                  "SysErr: " + System::getErrString(errCode) );

               retVal = fhgfsErrFromSysErr(errCode);

            }
         }
      }

      // prepare session data...
      setFDUnlocked(fd);
      setOffsetUnlocked(0);

      log->log(Log_DEBUG, logContext, "File created. ID: " + getFileID() );
   }


   safeMutex.unlock(); // U N L O C K

   return retVal;
}

/**
 * To avoid races with other writers in same session, apply new mirrorNode only if it was
 * NULL before. Otherwise release given mirrorNode and return the existing one.
 *
 * @mirrorNode will be released if a mirrorNode was set already.
 * @return existing mirrorNode if it was not NULL, given mirrorNode otherwise.
 */
Node* SessionLocalFile::setMirrorNodeExclusive(Node** mirrorNode)
{
   Node* retVal;

   SafeMutexLock safeMutex(&this->sessionMutex); // L O C K

   if(this->mirrorNode)
   { // release given mirrorNode
      Program::getApp()->getStorageNodes()->releaseNode(mirrorNode);
   }
   else
      this->mirrorNode = *mirrorNode;

   retVal = this->mirrorNode;

   safeMutex.unlock(); // U N L O C K

   return retVal;
}

/**
 * @param errCode positive(!) system error code
 */
FhgfsOpsErr SessionLocalFile::fhgfsErrFromSysErr(int64_t errCode)
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

   }

   return FhgfsOpsErr_INTERNAL;
}
