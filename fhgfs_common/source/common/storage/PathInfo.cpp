/*
 * PathInfo methods
 */

#include <common/app/log/LogContext.h>
#include <common/storage/PathInfo.h>
#include <common/toolkit/serialization/Serialization.h>

#define MIN_ENTRY_ID_LEN 1 // usually A-B-C, but also "root" and "disposal"

/**
 * Serialize into outBuf, 4-byte aligned
 */
size_t PathInfo::serialize(char* outBuf)
{
   size_t bufPos = 0;

   // flags
   bufPos += Serialization::serializeInt(&outBuf[bufPos], this->flags);

   if (this->flags & PATHINFO_FEATURE_ORIG)
   {
      // origParentUID
      bufPos += Serialization::serializeUInt(&outBuf[bufPos], this->origParentUID);

      // origParentEntryID
      bufPos += Serialization::serializeStrAlign4(&outBuf[bufPos],
         this->origParentEntryID.length(), this->origParentEntryID.c_str() );
   }

   return bufPos;
}

/**
 * deserialize the given buffer
 */
bool PathInfo::deserialize(const char* buf, size_t bufLen, unsigned* outLen)
{
   unsigned bufPos = 0;

   int flags;
   unsigned origParentUID;
   std::string origParentEntryID;
   const char* logContext = "PathInfo deserialiazation";

   {
      // flags
      unsigned flagsBufLen;
      const char* serialType = "Flags";

      if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos,
         &flags, &flagsBufLen) )
      {
         LogContext(logContext).logErr("Deserialization failed: " + std::string(serialType) );
         return false;
      }

      bufPos += flagsBufLen;
   }

   if (flags & PATHINFO_FEATURE_ORIG)
   {
      {
         // origParentUID
         unsigned origBufLen;
         const char* serialType = "origParentUID";

         if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
            &origParentUID, &origBufLen) )
         {
            LogContext(logContext).logErr("Deserialization failed: " + std::string(serialType) );
            return false;
         }

         bufPos += origBufLen;
      }

      {
         // origParentEntryID
         unsigned origBufLen;

         const char* serialType = "origParentEntryID";

         if(!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
            &origParentEntryID, &origBufLen) )
         {
            LogContext(logContext).logErr("Deserialization failed: " + std::string(serialType) );
            return false;
         }

         bufPos += origBufLen;
      }
   }
   else
   {
      origParentUID = 0;
      // origParentEntryID = ""; auto-initialized
   }

   this->set(origParentUID, origParentEntryID, flags);

   *outLen = bufPos;

   return true;
}

/**
 * Required size for serialization, 4-byte aligned
 */
unsigned PathInfo::serialLen(void)
{
   unsigned length = Serialization::serialLenInt();                                 // flags

    if (this->flags & PATHINFO_FEATURE_ORIG)
    {
       length +=
          Serialization::serialLenUInt()                                         +  // origParentUID
          Serialization::serialLenStrAlign4(this->origParentEntryID.length() );
    }

   return length;
}

