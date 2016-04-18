/*
 * EntryInfo methods
 */

#include <common/app/log/LogContext.h>
#include <common/storage/EntryInfo.h>
#include <common/toolkit/serialization/Serialization.h>

#define MIN_ENTRY_ID_LEN 1 // usually A-B-C, but also "root" and "disposal"

/**
 * Serialize into outBuf, 4-byte aligned
 */
size_t EntryInfo::serialize(char* outBuf)
{
   size_t bufPos = 0;

   // DirEntryType
   bufPos += Serialization::serializeUInt(&outBuf[bufPos], this->entryType);

   // flags
   bufPos += Serialization::serializeInt(&outBuf[bufPos], this->flags);

   // parentEntryID
   bufPos += Serialization::serializeStrAlign4(&outBuf[bufPos],
      this->getParentEntryID().length(), this->getParentEntryID().c_str() );

   // entryID
   bufPos += Serialization::serializeStrAlign4(&outBuf[bufPos],
      this->getEntryID().length(), this->getEntryID().c_str() );

   // fileName
   bufPos += Serialization::serializeStrAlign4(&outBuf[bufPos],
      this->getFileName().length(), this->getFileName().c_str() );

   // ownerNodeID
   bufPos += Serialization::serializeUShort(&outBuf[bufPos], this->ownerNodeID);

   // padding for 4-byte alignment
   bufPos += Serialization::serialLenUShort();


   return bufPos;
}

/**
 * deserialize the given buffer
 */
bool EntryInfo::deserialize(const char* buf, size_t bufLen, unsigned* outLen)
{
   unsigned bufPos = 0;

   std::string parentEntryID;
   std::string entryID;
   std::string fileName;
   unsigned entryType;
   int statFlags;
   uint16_t ownerNodeID;
   const char* logContext = "EntryInfo deserialiazation";

   {
      // DirEntryType
      unsigned typeBufLen;
      std::string serialType = "DirEntryType";

      if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &entryType, &typeBufLen) )
      {
         LogContext(logContext).logErr("Deserialization failed: " + serialType);
         return false;
      }

      bufPos += typeBufLen;
   }

   {
      // flags
      unsigned flagsBufLen;
      std::string serialType = "Flags";

      if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos,
         &statFlags, &flagsBufLen) )
      {
         LogContext(logContext).logErr("Deserialization failed: " + serialType);
         return false;
      }

      bufPos += flagsBufLen;
   }

   {
      // parentEntryID
      unsigned parentBufLen;
      const char* parentEntryIDChar;
      unsigned parentEntryIDLen;
      std::string serialType = "ParentEntryID";

      if(!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &parentEntryIDLen, &parentEntryIDChar, &parentBufLen) )
      {
         LogContext(logContext).logErr("Deserialization failed: " + serialType);
         return false;
      }

      // Note: the root ("/") has parentEntryID = ""
      if (unlikely((parentEntryIDLen < MIN_ENTRY_ID_LEN) && (parentEntryIDLen > 0)) )
      {
         LogContext(logContext).logErr("Deserialization failed: parentEntryIDLen too small");
         return false;
      }

      parentEntryID.assign(parentEntryIDChar, parentEntryIDLen);
      bufPos += parentBufLen;
   }

   {
      // entryID
      unsigned entryBufLen;
      const char* entryIDChar;
      unsigned entryIDLen;
      std::string serialType = "EntryID";


      if(!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &entryIDLen, &entryIDChar, &entryBufLen) )
      {
         LogContext(logContext).logErr("Deserialization failed: " + serialType);
         return false;
      }

      if (unlikely(entryIDLen < MIN_ENTRY_ID_LEN) )
      {
         LogContext(logContext).logErr(std::string("Deserialization failed: "
            "entryIDLen too small: ") + StringTk::uintToStr(entryIDLen) +
            " '" + std::string(entryIDChar) + "'");
         return false;
      }


      entryID.assign(entryIDChar, entryIDLen);
      bufPos += entryBufLen;
   }

   {
      // fileName
      unsigned nameBufLen;
      const char* fileNameChar;
      unsigned fileNameLen;
      std::string serialType = "FileName";

      if(!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &fileNameLen, &fileNameChar, &nameBufLen) )
      {
         LogContext(logContext).logErr("Deserialization failed: " + serialType);
         return false;
      }

      fileName.assign(fileNameChar, fileNameLen);
      bufPos += nameBufLen;
   }

   {
      // ownerNodeID
      unsigned ownerBufLen;
      std::string serialType = "OwnerNodeID";

      if(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos,
         &ownerNodeID, &ownerBufLen) )
      {
         LogContext(logContext).logErr("Deserialization failed: " + serialType);
         return false;
      }

      bufPos += ownerBufLen;
   }

   {
      // padding for 4-byte alignment
      bufPos += Serialization::serialLenUShort();
   }

   this->set(ownerNodeID, parentEntryID, entryID, fileName, (DirEntryType) entryType, statFlags);

   *outLen = bufPos;

   return true;
}

/**
 * Required size for serialization, 4-byte aligned
 */
unsigned EntryInfo::serialLen(void)
{
   unsigned length =
      Serialization::serialLenUInt()                                        + // entryType
      Serialization::serialLenInt()                                         + // flags
      Serialization::serialLenStrAlign4(this->parentEntryID.length() )      +
      Serialization::serialLenStrAlign4(this->entryID.length() )            +
      Serialization::serialLenStrAlign4(this->fileName.length() )           +
      Serialization::serialLenUShort()                                      + // ownerNodeID
      Serialization::serialLenUShort();                                       // padding / alignment

   return length;
}


/**
 * Compare our (this) values with compareInfo.
 *
 * Note: This method is rather slow, only use it for uncritical code paths, i.e. unit tests
 */
bool EntryInfo::compare(EntryInfo* compareInfo)
{
   const char* logContext = "EntryInfo comparison";
   size_t thisLen = serialLen();
   size_t compareLen = compareInfo->serialLen();

   if (thisLen != compareLen)
      return false;

   // calloc to avoid invalid valgrind report (uninitialized value)
   void *thisBuf    = calloc(1, thisLen);
   void *compareBuf = calloc(1, compareLen);

   bool retVal = true;

   size_t thisSerializeLen = serialize((char*)thisBuf);
   size_t compareSerializeLen = compareInfo->serialize((char*)compareBuf);

   if (thisSerializeLen != thisLen || compareSerializeLen != thisLen)
   {
      LogContext(logContext).logErr("Bug: Unexpected serialization length mismatch!");
      retVal = false;
   }
   else
   {
      int cmpRes = memcmp(thisBuf, compareBuf, thisLen);
      if (cmpRes)
         retVal = false;

   }

   free(thisBuf);
   free(compareBuf);

   return retVal;
}
