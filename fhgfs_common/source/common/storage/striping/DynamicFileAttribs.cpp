#include <common/toolkit/serialization/Serialization.h>
#include "DynamicFileAttribs.h"



unsigned DynamicFileAttribs::serialize(char* buf) const
{
   size_t bufPos = 0;

   // storageVersion
   bufPos += Serialization::serializeUInt64(&buf[bufPos], this->storageVersion);

   // fileSize
   bufPos += Serialization::serializeInt64(&buf[bufPos], this->fileSize);

   // numBlocks
   bufPos += Serialization::serializeUInt64(&buf[bufPos], this->numBlocks);

   // modificationTimeSecs
   bufPos += Serialization::serializeInt64(&buf[bufPos], this->modificationTimeSecs);

   // lastAccessTimeSecs
   bufPos += Serialization::serializeInt64(&buf[bufPos], this->lastAccessTimeSecs);

   return bufPos;
}

bool DynamicFileAttribs::deserialize(const char* buf, size_t bufLen, unsigned* outLen)
{
   size_t bufPos = 0;

   {
      // storageVersion
      unsigned storageVersionLen;

      if (!Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos, &this->storageVersion,
         &storageVersionLen) )
         return false;

      bufPos += storageVersionLen;
   }

   {
      // fileSize
      unsigned fileSizeLen;

      if (!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos, &this->fileSize,
         &fileSizeLen) )
         return false;

      bufPos += fileSizeLen;
   }

   {
      // numBlocks
      unsigned numBlocksLen;

      if (!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos, &this->numBlocks,
         &numBlocksLen) )
         return false;

      bufPos += numBlocksLen;
   }

   {
      // modificationTimeSecs
      unsigned modificationTimeSecsLen;

      if (!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos, &this->modificationTimeSecs,
         &modificationTimeSecsLen) )
         return false;

      bufPos += modificationTimeSecsLen;
   }

   {
      // lastAccessTimeSecs
      unsigned lastAccessTimeSecsLen;

      if (!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos, &this->lastAccessTimeSecs,
         &lastAccessTimeSecsLen) )
         return false;

      bufPos += lastAccessTimeSecsLen;
   }

   *outLen = bufPos;

   return true;
}

unsigned DynamicFileAttribs::serialLen()
{
   unsigned len = 0;

   len += Serialization::serialLenUInt64();            // storageVersion
   len += Serialization::serialLenInt64();             // fileSize
   len += Serialization::serialLenUInt64();            // numBlocks
   len += Serialization::serialLenInt64();             // modificationTimeSecs
   len += Serialization::serialLenInt64();             // lastAccessTimeSecs

   return len;
}

bool dynamicFileAttribsEquals(const DynamicFileAttribs& first, const DynamicFileAttribs& second)
{
   // storageVersion
   if(first.storageVersion != second.storageVersion)
      return false;

   // fileSize
   if(first.fileSize != second.fileSize)
      return false;

   // numBlocks
   if(first.numBlocks != second.numBlocks)
      return false;

   // modificationTimeSecs
   if(first.modificationTimeSecs != second.modificationTimeSecs)
      return false;

   // lastAccessTimeSecs
   if(first.lastAccessTimeSecs != second.lastAccessTimeSecs)
      return false;

   return true;
}
