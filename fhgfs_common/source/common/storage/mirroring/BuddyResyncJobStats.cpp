#include "BuddyResyncJobStats.h"

#include <common/toolkit/serialization/Serialization.h>

size_t BuddyResyncJobStats::serialize(char* outBuf)
{
   size_t bufPos = 0;

   // BuddyResyncJobStatus
   bufPos += Serialization::serializeInt(&outBuf[bufPos], (int)this->status);

   // startTime
   bufPos += Serialization::serializeInt64(&outBuf[bufPos], (int)this->startTime);

   // endTime
   bufPos += Serialization::serializeInt64(&outBuf[bufPos], (int)this->endTime);

   // discoveredFiles
   bufPos += Serialization::serializeUInt64(&outBuf[bufPos], this->discoveredFiles);

   // discoveredDirs
   bufPos += Serialization::serializeUInt64(&outBuf[bufPos], this->discoveredDirs);

   // matchedFiles
   bufPos += Serialization::serializeUInt64(&outBuf[bufPos], this->matchedFiles);

   // matchedDirs
   bufPos += Serialization::serializeUInt64(&outBuf[bufPos], this->matchedDirs);

   // syncedFiles
   bufPos += Serialization::serializeUInt64(&outBuf[bufPos], this->syncedFiles);

   // syncedDirs
   bufPos += Serialization::serializeUInt64(&outBuf[bufPos], this->syncedDirs);

   // errorFiles
   bufPos += Serialization::serializeUInt64(&outBuf[bufPos], this->errorFiles);

   // errorDirs
   bufPos += Serialization::serializeUInt64(&outBuf[bufPos], this->errorDirs);

   return bufPos;
}

/**
 * deserialize the given buffer
 */
bool BuddyResyncJobStats::deserialize(const char* buf, size_t bufLen, unsigned* outLen)
{
   unsigned bufPos = 0;

   {
      // BuddyResyncJobStatus
      unsigned statusBufLen;
      int intStatus;

      if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos, &intStatus, &statusBufLen) )
         return false;

      status = (BuddyResyncJobStatus)intStatus;

      bufPos += statusBufLen;
   }

   {
      // startTime
      unsigned startTimeBufLen;

      if(!Serialization::deserializeInt64(&buf[bufPos], bufLen - bufPos, &startTime,
         &startTimeBufLen))
         return false;

      bufPos += startTimeBufLen;
   }

   {
      // endTime
      unsigned endTimeBufLen;

      if(!Serialization::deserializeInt64(&buf[bufPos], bufLen - bufPos, &endTime, &endTimeBufLen))
         return false;

      bufPos += endTimeBufLen;
   }

   {
      // discoveredFiles
      unsigned discoveredFilesBufLen;

      if(!Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos,
         &discoveredFiles, &discoveredFilesBufLen) )
         return false;

      bufPos += discoveredFilesBufLen;
   }

   {
      // discoveredDirs
      unsigned discoveredDirsBufLen;

      if(!Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos,
         &discoveredDirs, &discoveredDirsBufLen) )
         return false;

      bufPos += discoveredDirsBufLen;
   }

   {
      // matchedFiles
      unsigned matchedFilesBufLen;

      if(!Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos,
         &matchedFiles, &matchedFilesBufLen) )
         return false;

      bufPos += matchedFilesBufLen;
   }

   {
      // matchedDirs
      unsigned matchedDirsBufLen;

      if(!Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos,
         &matchedDirs, &matchedDirsBufLen) )
         return false;

      bufPos += matchedDirsBufLen;
   }

   {
      // syncedFiles
      unsigned syncedFilesBufLen;

      if(!Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos,
         &syncedFiles, &syncedFilesBufLen) )
         return false;

      bufPos += syncedFilesBufLen;
   }

   {
      // syncedDirs
      unsigned syncedDirsBufLen;

      if(!Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos,
         &syncedDirs, &syncedDirsBufLen) )
         return false;

      bufPos += syncedDirsBufLen;
   }

   {
      // errorFiles
      unsigned errorFilesBufLen;

      if(!Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos,
         &errorFiles, &errorFilesBufLen) )
         return false;

      bufPos += errorFilesBufLen;
   }

   {
      // errorDirs
      unsigned errorDirsBufLen;

      if(!Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos,
         &errorDirs, &errorDirsBufLen) )
         return false;

      bufPos += errorDirsBufLen;
   }

   *outLen = bufPos;

   return true;
}

/**
 * Required size for serialization, 4-byte aligned
 */
unsigned BuddyResyncJobStats::serialLen(void)
{
   unsigned length =
      Serialization::serialLenInt()    + // status
      Serialization::serialLenInt64() +  // startTime
      Serialization::serialLenInt64() +  // endTime
      Serialization::serialLenUInt64() + // discoveredFiles
      Serialization::serialLenUInt64() + // discoveredDirs
      Serialization::serialLenUInt64() + // matchedFiles
      Serialization::serialLenUInt64() + // matchedDirs
      Serialization::serialLenUInt64() + // syncedFiles
      Serialization::serialLenUInt64() + // syncedDirs
      Serialization::serialLenUInt64() + // errorFiles
      Serialization::serialLenUInt64();  // errorDirs

   return length;
}
