#include "StorageTargetInfo.h"

#include <common/net/message/storage/StatStoragePathMsg.h>
#include <common/net/message/storage/StatStoragePathRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/serialization/Serialization.h>


/**
 * Serialize into outBuf
 */
size_t StorageTargetInfo::serialize(char* outBuf)
{
   size_t bufPos = 0;

   // targetID
   bufPos += Serialization::serializeUInt16(&outBuf[bufPos], this->targetID);

   // pathStr
   bufPos += Serialization::serializeStrAlign4(&outBuf[bufPos], this->pathStr.length(),
      this->pathStr.c_str());

   // diskSpaceTotal
   bufPos += Serialization::serializeInt64(&outBuf[bufPos], this->diskSpaceTotal);

   // diskSpaceFree
   bufPos += Serialization::serializeInt64(&outBuf[bufPos], this->diskSpaceFree);

   // inodesTotal
   bufPos += Serialization::serializeInt64(&outBuf[bufPos], this->inodesTotal);

   // inodesFree
   bufPos += Serialization::serializeInt64(&outBuf[bufPos], this->inodesFree);

   // consistencyState
   bufPos += Serialization::serializeUInt8(&outBuf[bufPos],
         (uint8_t)this->consistencyState);


   return bufPos;
}

/**
 * deserialize the given buffer
 */
bool StorageTargetInfo::deserialize(const char* buf, size_t bufLen, unsigned* outLen)
{
   unsigned bufPos = 0;

   {
      // targetID
      unsigned targetIDBufLen;

      if ( !Serialization::deserializeUInt16(&buf[bufPos], bufLen - bufPos, &(this->targetID),
         &targetIDBufLen) )
         return false;

      bufPos += targetIDBufLen;
   }

   {
      // pathStr
      const char* pathStrChar;
      unsigned pathStrBufLen;
      unsigned pathStrLen;

      if ( !Serialization::deserializeStrAlign4(&buf[bufPos], bufLen - bufPos, &pathStrLen,
         &pathStrChar, &pathStrBufLen) )
         return false;

      this->pathStr.assign(pathStrChar, pathStrLen);
      bufPos += pathStrBufLen;
   }

   {
      // diskSpaceTotal
      unsigned diskSpaceTotalBufLen;

      if ( !Serialization::deserializeInt64(&buf[bufPos], bufLen - bufPos, &(this->diskSpaceTotal),
         &diskSpaceTotalBufLen) )
         return false;

      bufPos += diskSpaceTotalBufLen;
   }

   {
      // diskSpaceFree
      unsigned diskSpaceFreeBufLen;

      if ( !Serialization::deserializeInt64(&buf[bufPos], bufLen - bufPos, &(this->diskSpaceFree),
         &diskSpaceFreeBufLen) )
         return false;

      bufPos += diskSpaceFreeBufLen;
   }

   {
      // inodesTotal
      unsigned inodesTotalBufLen;

      if ( !Serialization::deserializeInt64(&buf[bufPos], bufLen - bufPos, &(this->inodesTotal),
         &inodesTotalBufLen) )
         return false;

      bufPos += inodesTotalBufLen;
   }

   {
      // inodesFree
      unsigned inodesFreeBufLen;

      if ( !Serialization::deserializeInt64(&buf[bufPos], bufLen - bufPos, &(this->inodesFree),
         &inodesFreeBufLen) )
         return false;

      bufPos += inodesFreeBufLen;
   }

   {
      // consistencyState
      unsigned consistencyStateBufLen;
      uint8_t consistencyStateInt;

      if ( !Serialization::deserializeUInt8(&buf[bufPos], bufLen - bufPos,
         &consistencyStateInt, &consistencyStateBufLen) )
         return false;

      this->consistencyState = static_cast<TargetConsistencyState>(consistencyStateInt);
      bufPos += consistencyStateBufLen;
   }


   *outLen = bufPos;

   return true;
}

/**
 * Required size for serialization
 */
unsigned StorageTargetInfo::serialLen()
{
   unsigned length =
      Serialization::serialLenUInt16() + // targetID
      Serialization::serialLenStrAlign4(this->pathStr.length())   + // pathStr
      Serialization::serialLenInt64() + // diskSpaceTotal
      Serialization::serialLenInt64() + // diskSpaceFree
      Serialization::serialLenInt64() + // inodesTotal
      Serialization::serialLenInt64() + // inodesFree
      Serialization::serialLenUInt8(); // consistencyState

      return length;
}

/**
 * @param targetID only required for storage servers, leave empty otherwise.
 * @return false on comm error, true otherwise.
 */
FhgfsOpsErr StorageTargetInfo::statStoragePath(Node* node, uint16_t targetID, int64_t* outFree,
   int64_t* outTotal, int64_t* outInodesFree, int64_t* outInodesTotal)
{
   FhgfsOpsErr retVal;
   bool commRes;
   char* respBuf = NULL;
   NetMessage* respMsg = NULL;
   StatStoragePathRespMsg* respMsgCast;

   StatStoragePathMsg msg(targetID);

   // connect & communicate
   commRes = MessagingTk::requestResponse(
      node, &msg, NETMSGTYPE_StatStoragePathResp, &respBuf, &respMsg);
   if(!commRes)
      return FhgfsOpsErr_COMMUNICATION;

   // handle result
   respMsgCast = (StatStoragePathRespMsg*)respMsg;

   retVal = (FhgfsOpsErr)respMsgCast->getResult();

   *outFree = respMsgCast->getSizeFree();
   *outTotal = respMsgCast->getSizeTotal();
   *outInodesFree = respMsgCast->getInodesFree();
   *outInodesTotal = respMsgCast->getInodesTotal();

   // cleanup
   SAFE_DELETE(respMsg);
   SAFE_FREE(respBuf);

   return retVal;
}
