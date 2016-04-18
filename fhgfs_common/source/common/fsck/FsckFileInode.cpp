#include "FsckFileInode.h"
#include <common/toolkit/serialization/Serialization.h>

/**
 * Serialize into outBuf
 */
size_t FsckFileInode::serialize(char* outBuf)
{
   size_t bufPos = 0;

   // internalID
   bufPos += Serialization::serializeUInt64(&outBuf[bufPos], this->getInternalID());

   // id
   bufPos += Serialization::serializeStrAlign4(&outBuf[bufPos], this->getID().length(),
      this->getID().c_str());

   // parentDirID
   bufPos += Serialization::serializeStrAlign4(&outBuf[bufPos], this->getParentDirID().length(),
      this->getParentDirID().c_str());

   // parentNodeID
   bufPos += Serialization::serializeUShort(&outBuf[bufPos], this->getParentNodeID());

   // pathInfo
   bufPos += this->pathInfo.serialize(&outBuf[bufPos]);

   // mode
   bufPos += Serialization::serializeInt(&outBuf[bufPos], this->getMode());

   // userID
   bufPos += Serialization::serializeUInt(&outBuf[bufPos], this->getUserID());

   // groupID
   bufPos += Serialization::serializeUInt(&outBuf[bufPos], this->getGroupID());

   // filesize
   bufPos += Serialization::serializeInt64(&outBuf[bufPos], this->getFileSize());

   // creationTime
   bufPos += Serialization::serializeInt64(&outBuf[bufPos], this->getCreationTime());

   // modificationTime
   bufPos += Serialization::serializeInt64(&outBuf[bufPos], this->getModificationTime());

   // lastAccessTime
   bufPos += Serialization::serializeInt64(&outBuf[bufPos], this->getLastAccessTime());

   // numHardLinks
   bufPos += Serialization::serializeUInt(&outBuf[bufPos], this->getNumHardLinks());

   // usedBlocks
   bufPos += Serialization::serializeUInt64(&outBuf[bufPos], this->getUsedBlocks());

   // stripeTargets
   bufPos += Serialization::serializeUInt16Vector(&outBuf[bufPos], this->getStripeTargets());

   // stripePatternType
   bufPos += Serialization::serializeInt(&outBuf[bufPos], (int) this->getStripePatternType());

   // chunkSize
   bufPos += Serialization::serializeUInt(&outBuf[bufPos], this->getChunkSize());

   // saveNodeID
   bufPos += Serialization::serializeUShort(&outBuf[bufPos], this->getSaveNodeID());

   //isInlined
   bufPos += Serialization::serializeBool(&outBuf[bufPos], this->getIsInlined());

   // readable
   bufPos += Serialization::serializeBool(&outBuf[bufPos], this->getReadable());

   return bufPos;
}

/**
 * deserialize the given buffer
 */
bool FsckFileInode::deserialize(const char* buf, size_t bufLen, unsigned* outLen)
{
   unsigned bufPos = 0;

   uint64_t internalID;

   std::string id; // filesystem-wide unique string
   std::string parentDirID;
   uint16_t parentNodeID;

   PathInfo pathInfo;

   int mode;
   unsigned userID;
   unsigned groupID;

   int64_t fileSize;
   int64_t creationTime;
   int64_t modificationTime;
   int64_t lastAccessTime;
   unsigned numHardLinks;

   uint64_t usedBlocks;

   UInt16Vector stripeTargets;
   int intStripePatternType;
   unsigned chunkSize;

   uint16_t saveNodeID;
   bool isInlined;
   bool readable;

   {
      // internalID
      unsigned internalIDBufLen;

      if ( !Serialization::deserializeUInt64(&buf[bufPos], bufLen - bufPos, &internalID,
         &internalIDBufLen) )
         return false;

      bufPos += internalIDBufLen;
   }

   {
      // id
      unsigned idBufLen;
      const char* idChar;
      unsigned idLen;

      if ( !Serialization::deserializeStrAlign4(&buf[bufPos], bufLen - bufPos, &idLen, &idChar,
         &idBufLen) )
         return false;

      id.assign(idChar, idLen);
      bufPos += idBufLen;
   }

   {
      // parentDirID
      unsigned parentDirIDBufLen;
      const char* parentDirIDChar;
      unsigned parentDirIDLen;

      if ( !Serialization::deserializeStrAlign4(&buf[bufPos], bufLen - bufPos, &parentDirIDLen,
         &parentDirIDChar, &parentDirIDBufLen) )
         return false;

      parentDirID.assign(parentDirIDChar, parentDirIDLen);
      bufPos += parentDirIDBufLen;
   }

   {
      // parentNodeID
      unsigned parentNodeIDBufLen;

      if(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos,
         &parentNodeID, &parentNodeIDBufLen) )
         return false;

      bufPos += parentNodeIDBufLen;
   }

   {
      // pathInfo
      unsigned pathInfoBufLen;

      if (! pathInfo.deserialize(&buf[bufPos], bufLen-bufPos, &pathInfoBufLen) )
         return false;

      bufPos += pathInfoBufLen;
   }

   {
      // mode
      unsigned modeBufLen;

      if ( !Serialization::deserializeInt(&buf[bufPos], bufLen - bufPos, &mode,
         &modeBufLen) )
         return false;

      bufPos += modeBufLen;
   }

   {
      // userID
      unsigned userIDBufLen;

      if ( !Serialization::deserializeUInt(&buf[bufPos], bufLen - bufPos, &userID,
         &userIDBufLen) )
         return false;

      bufPos += userIDBufLen;
   }

   {
      // groupID
      unsigned groupIDBufLen;

      if ( !Serialization::deserializeUInt(&buf[bufPos], bufLen - bufPos, &groupID,
         &groupIDBufLen) )
         return false;

      bufPos += groupIDBufLen;
   }

   {
      // filesize
      unsigned filesizeBufLen;

      if ( !Serialization::deserializeInt64(&buf[bufPos], bufLen - bufPos, &fileSize,
         &filesizeBufLen) )
         return false;

      bufPos += filesizeBufLen;
   }

   {
      // creationTime
      unsigned creationTimeBufLen;

      if ( !Serialization::deserializeInt64(&buf[bufPos], bufLen - bufPos, &creationTime,
         &creationTimeBufLen) )
         return false;

      bufPos += creationTimeBufLen;
   }

   {
      // modificationTime
      unsigned modificationTimeBufLen;

      if ( !Serialization::deserializeInt64(&buf[bufPos], bufLen - bufPos, &modificationTime,
         &modificationTimeBufLen) )
         return false;

      bufPos += modificationTimeBufLen;
   }

   {
      // lastAccessTime
      unsigned lastAccessTimeBufLen;

      if ( !Serialization::deserializeInt64(&buf[bufPos], bufLen - bufPos, &lastAccessTime,
         &lastAccessTimeBufLen) )
         return false;

      bufPos += lastAccessTimeBufLen;
   }

   {
      // numHardLinks
      unsigned numHardLinksBufLen;

      if ( !Serialization::deserializeUInt(&buf[bufPos], bufLen - bufPos, &numHardLinks,
         &numHardLinksBufLen) )
         return false;

      bufPos += numHardLinksBufLen;
   }

   {
      // usedBlocks
      unsigned usedBlocksBufLen;

      if ( !Serialization::deserializeUInt64(&buf[bufPos], bufLen - bufPos, &usedBlocks,
         &usedBlocksBufLen) )
         return false;

      bufPos += usedBlocksBufLen;
   }

   {
      // stripeTargets
      unsigned elemNum;
      const char* vecStart;
      unsigned vecBufLen;

      if ( !Serialization::deserializeUInt16VectorPreprocess(&buf[bufPos], bufLen - bufPos,
         &elemNum, &vecStart, &vecBufLen) )
         return false;

      if ( !Serialization::deserializeUInt16Vector(vecBufLen, elemNum, vecStart, &stripeTargets) )
         return false;

      bufPos += vecBufLen;
   }

   {
      // stripePatternType
      unsigned stripePatternTypeBufLen;

      if ( !Serialization::deserializeInt(&buf[bufPos], bufLen - bufPos, &intStripePatternType,
         &stripePatternTypeBufLen) )
         return false;

      bufPos += stripePatternTypeBufLen;
   }

   {
      // chunkSize
      unsigned chunkSizeBufLen;

      if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
         &chunkSize, &chunkSizeBufLen) )
         return false;

      bufPos += chunkSizeBufLen;
   }

   {
      // saveNodeID
      unsigned saveNodeIDBufLen;

      if(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos,
         &saveNodeID, &saveNodeIDBufLen) )
         return false;

      bufPos += saveNodeIDBufLen;
   }

   {
      // isInlined
      unsigned isInlinedBufLen;

      if ( !Serialization::deserializeBool(&buf[bufPos], bufLen - bufPos, &isInlined,
         &isInlinedBufLen) )
         return false;

      bufPos += isInlinedBufLen;
   }

   {
      // readable
      unsigned readableBufLen;

      if ( !Serialization::deserializeBool(&buf[bufPos], bufLen - bufPos, &readable,
         &readableBufLen) )
         return false;

      bufPos += readableBufLen;
   }

   this->update(internalID, id, parentDirID, parentNodeID, pathInfo, mode, userID, groupID,
      fileSize, usedBlocks, creationTime, modificationTime, lastAccessTime, numHardLinks,
      stripeTargets, (FsckStripePatternType) intStripePatternType, chunkSize, saveNodeID, isInlined,
      readable);

   *outLen = bufPos;

   return true;
}

/**
 * Required size for serialization
 */
unsigned FsckFileInode::serialLen()
{
   unsigned length =
      Serialization::serialLenUInt64() + // internalID
      Serialization::serialLenStrAlign4(this->getID().length()) + // id
      Serialization::serialLenStrAlign4(this->getParentDirID().length()) + // parentDirID
      Serialization::serialLenUShort() + // parentNodeID
      this->pathInfo.serialLen() + // pathInfo
      Serialization::serialLenInt() + // mode
      Serialization::serialLenUInt() + // userID
      Serialization::serialLenUInt() + // groupID
      Serialization::serialLenInt64() + // filesize
      Serialization::serialLenInt64() + // creationTime
      Serialization::serialLenInt64() + // modificationTime
      Serialization::serialLenInt64() + // lastAccessTime
      Serialization::serialLenUInt() + // numHardLinks
      Serialization::serialLenUInt64() + // usedBlocks
      Serialization::serialLenUInt16Vector(this->getStripeTargets()) + // stripeTargets
      Serialization::serialLenInt() + // stripePatternType
      Serialization::serialLenUInt() + // chunkSize
      Serialization::serialLenUShort() + // saveNodeID
      Serialization::serialLenBool() + // isInlined
      Serialization::serialLenBool(); // readable

   return length;
}
