#include <common/toolkit/Serialization.h>
#include <common/storage/PathInfo.h>
#include <os/OsDeps.h>

/**
 * Serialize into outBuf, 4-byte aligned
 */
size_t PathInfo_serialize(const PathInfo* this, char* outBuf)
{
   size_t bufPos = 0;

   // flags
   bufPos += Serialization_serializeUInt(&outBuf[bufPos], this->flags);

   if (this->flags & PATHINFO_FEATURE_ORIG)
   {
      // origParentUID
      bufPos += Serialization_serializeUInt(&outBuf[bufPos], this->origParentUID);

      // origParentEntryID
      bufPos += Serialization_serializeStrAlign4(&outBuf[bufPos],
         os_strlen(this->origParentEntryID), this->origParentEntryID);
   }

   return bufPos;
}

/**
 * deserialize the given buffer
 */
fhgfs_bool PathInfo_deserialize(const char* buf, size_t bufLen, unsigned* outLen,
   PathInfo* outThis)
{
   unsigned bufPos = 0;

   unsigned flags;

   unsigned origParentUID;
   unsigned origParentEntryIDLen;
   const char* origParentEntryID;

   {
      // flags
      unsigned flagsBufLen;
      const char* serialType = "Flags";

      if(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos,
         &flags, &flagsBufLen) )
      {
         printk_fhgfs(KERN_INFO, "Deserialization failed: %s:\n", serialType);
         return fhgfs_false;
      }

      bufPos += flagsBufLen;
   }

   if (flags & PATHINFO_FEATURE_ORIG)
   {  // file has origParentUID and origParentEntryID
      {
         // origParentUID
         unsigned origBufLen;
         const char* serialType = "origParentUID";

         if(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos,
            &origParentUID, &origBufLen) )
         {
            printk_fhgfs(KERN_INFO, "Deserialization failed: %s:\n", serialType);
            return fhgfs_false;
         }

         bufPos += origBufLen;
      }

      {
         // origParentEntryID
         unsigned origBufLen;
         const char* serialType = "origParentEntryID";

         if(!Serialization_deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
            &origParentEntryIDLen, &origParentEntryID, &origBufLen) )
         {
            printk_fhgfs(KERN_INFO, "Deserialization failed: %s:\n", serialType);
            return fhgfs_false;
         }

         bufPos += origBufLen;
      }
   }
   else
   {  // either a directory or a file stored in old format
      origParentUID = 0;
      origParentEntryID = NULL;
   }


   PathInfo_init(outThis, origParentUID, origParentEntryID, flags);

   *outLen = bufPos;

   return fhgfs_true;
}

/**
 * Required size for serialization, 4-byte aligned
 */
unsigned PathInfo_serialLen(const PathInfo* this)
{
   unsigned length = Serialization_serialLenUInt();                              // flags

   if (this->flags & PATHINFO_FEATURE_ORIG)
   {
      length +=
      Serialization_serialLenUInt()                                           +  // origParentUID
      Serialization_serialLenStrAlign4(os_strlen(this->origParentEntryID) );
   }

   return length;
}

