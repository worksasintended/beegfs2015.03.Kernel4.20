#include "FsckChunk.h"

#include <common/toolkit/serialization/Serialization.h>

/**
 * Serialize into outBuf
 */
size_t FsckChunk::serialize(char* outBuf)
{
   size_t bufPos = 0;

   // id
   bufPos += Serialization::serializeStrAlign4(&outBuf[bufPos], this->getID().length(),
      this->getID().c_str());

   // targetID
   bufPos += Serialization::serializeUShort(&outBuf[bufPos], this->getTargetID());

   // savedPath
   bufPos += Serialization::serializePath(&outBuf[bufPos], &(this->savedPath));

   // filesize
   bufPos += Serialization::serializeInt64(&outBuf[bufPos], this->getFileSize());

   // usedBlocks
   bufPos += Serialization::serializeUInt64(&outBuf[bufPos], this->getUsedBlocks());

   // creationTime
   bufPos += Serialization::serializeInt64(&outBuf[bufPos], this->getCreationTime());

   // modificationTime
   bufPos += Serialization::serializeInt64(&outBuf[bufPos], this->getModificationTime());

   // lastAccessTime
   bufPos += Serialization::serializeInt64(&outBuf[bufPos], this->getLastAccessTime());

   // userID
   bufPos += Serialization::serializeUInt(&outBuf[bufPos], this->getUserID());

   // groupID
   bufPos += Serialization::serializeUInt(&outBuf[bufPos], this->getGroupID());

   // buddyGroupID
   bufPos += Serialization::serializeUShort(&outBuf[bufPos], this->getBuddyGroupID());

   return bufPos;
}

/**
 * deserialize the given buffer
 */
bool FsckChunk::deserialize(const char* buf, size_t bufLen, unsigned* outLen)
{
   unsigned bufPos = 0;

   std::string id;
   uint16_t targetID;
   Path savedPath;
   int64_t fileSize;
   uint64_t usedBlocks;
   int64_t creationTime;
   int64_t modificationTime;
   int64_t lastAccessTime;
   unsigned userID;
   unsigned groupID;
   uint16_t buddyGroupID;

   {
      // id
      unsigned idBufLen;
      const char* idChar;
      unsigned idLen;

      if(!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &idLen, &idChar, &idBufLen) )
         return false;

      id.assign(idChar, idLen);
      bufPos += idBufLen;
   }

   {
      // targetID
      unsigned targetIDBufLen;

      if(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos, &targetID, &targetIDBufLen))
         return false;

      bufPos += targetIDBufLen;
   }

   {
      // savedPath
      unsigned savedPathBufLen;
      PathDeserializationInfo pathDeserInfo;

      if (! Serialization::deserializePathPreprocess(&buf[bufPos], bufLen-bufPos, &pathDeserInfo,
         &savedPathBufLen))
         return false;

      bufPos += savedPathBufLen;

      Serialization::deserializePath(pathDeserInfo, &savedPath);
   }

   {
      // filesize
      unsigned filesizeBufLen;

      if(!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos, &fileSize, &filesizeBufLen))
         return false;

      bufPos += filesizeBufLen;
   }

   {
      // usedBlocks
      unsigned usedBlocksBufLen;

      if(!Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos, &usedBlocks,
         &usedBlocksBufLen))
         return false;

      bufPos += usedBlocksBufLen;
   }

   {
      // creationTime
      unsigned creationTimeBufLen;

      if(!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos, &creationTime,
         &creationTimeBufLen))
         return false;

      bufPos += creationTimeBufLen;
   }

   {
      // modificationTime
      unsigned modificationTimeBufLen;

      if(!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos, &modificationTime,
         &modificationTimeBufLen))
         return false;

      bufPos += modificationTimeBufLen;
   }

   {
      // lastAccessTime
      unsigned lastAccessTimeBufLen;

      if(!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos, &lastAccessTime,
         &lastAccessTimeBufLen))
         return false;

      bufPos += lastAccessTimeBufLen;
   }

   {
      // userID
      unsigned userIDBufLen;

      if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &userID,
         &userIDBufLen))
         return false;

      bufPos += userIDBufLen;
   }

   {
      // groupID
      unsigned groupIDBufLen;

      if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &groupID,
         &groupIDBufLen))
         return false;

      bufPos += groupIDBufLen;
   }

   {
      // buddyGroupID
      unsigned buddyGroupIDBufLen;

      if ( !Serialization::deserializeUShort(&buf[bufPos], bufLen - bufPos, &buddyGroupID,
         &buddyGroupIDBufLen) )
         return false;

      bufPos += buddyGroupIDBufLen;
   }

   this->update(id, targetID, savedPath, fileSize, usedBlocks, creationTime, modificationTime,
      lastAccessTime, userID, groupID, buddyGroupID);
   *outLen = bufPos;

   return true;
}

/**
 * Required size for serialization
 */
unsigned FsckChunk::serialLen()
{
   unsigned length =
      Serialization::serialLenStrAlign4(this->getID().length() )   + // id
      Serialization::serialLenUShort() + // targetID
      Serialization::serialLenPath(&(this->savedPath)) + // savedPath
      Serialization::serialLenInt64() + // filesize
      Serialization::serialLenUInt64() + // usedBlocks
      Serialization::serialLenInt64() + // creationTime
      Serialization::serialLenInt64() + // modificationTime
      Serialization::serialLenInt64() + // lastAccessTime
      Serialization::serialLenUInt() + // userID
      Serialization::serialLenUInt() + // groupID
      Serialization::serialLenUShort(); // buddyGroupID

      return length;
}
