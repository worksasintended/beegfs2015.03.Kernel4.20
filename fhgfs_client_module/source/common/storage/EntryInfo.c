#include <common/toolkit/Serialization.h>
#include <common/storage/EntryInfo.h>
#include <os/OsDeps.h>

#define MIN_ENTRY_ID_LEN 1 // usually A-B-C, but also "root" and "disposal"

/**
 * Serialize into outBuf, 4-byte aligned
 */
size_t EntryInfo_serialize(const EntryInfo* this, char* outBuf)
{
   size_t bufPos = 0;

   // DirEntryType
   bufPos += Serialization_serializeUInt(&outBuf[bufPos], this->entryType);

   // flags
   bufPos += Serialization_serializeInt(&outBuf[bufPos], this->flags);

   // parentEntryID
   bufPos += Serialization_serializeStrAlign4(&outBuf[bufPos],
      os_strlen(this->parentEntryID), this->parentEntryID);

   if (unlikely(os_strlen(this->entryID) < MIN_ENTRY_ID_LEN) )
   {
      printk_fhgfs(KERN_WARNING, "EntryID too small!\n"); // server side deserialization will fail
      dump_stack();
   }

   // entryID
   bufPos += Serialization_serializeStrAlign4(&outBuf[bufPos],
      os_strlen(this->entryID), this->entryID);

   // fileName
   bufPos += Serialization_serializeStrAlign4(&outBuf[bufPos],
      os_strlen(this->fileName), this->fileName);

   // ownerNodeID
   bufPos += Serialization_serializeUShort(&outBuf[bufPos], this->ownerNodeID);

   // padding for 4-byte alignment
   bufPos += Serialization_serialLenUShort();


   return bufPos;
}

/**
 * deserialize the given buffer
 */
fhgfs_bool EntryInfo_deserialize(const char* buf, size_t bufLen, unsigned* outLen,
   EntryInfo* outThis)
{
   unsigned bufPos = 0;

   const char* parentEntryID;
   unsigned parentEntryIDLen;

   const char* entryID;
   unsigned entryIDLen;

   const char* fileName;
   unsigned fileNameLen;

   unsigned entryType;
   int statFlags;
   uint16_t ownerNodeID;

   {
      // DirEntryType
      unsigned typeBufLen;
      const char* serialType = "DirEntryType";

      if(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, &entryType, &typeBufLen) )
      {
         printk_fhgfs(KERN_INFO, "Deserialization failed: %s\n", serialType);
         return fhgfs_false;
      }

      bufPos += typeBufLen;
   }

   {
      // flags
      unsigned flagsBufLen;
      const char* serialType = "Flags";

      if(!Serialization_deserializeInt(&buf[bufPos], bufLen-bufPos,
         &statFlags, &flagsBufLen) )
      {
         printk_fhgfs(KERN_INFO, "Deserialization failed: %s:\n", serialType);
         return fhgfs_false;
      }

      bufPos += flagsBufLen;
   }

   {
      // parentEntryID
      unsigned parentBufLen;
      const char* serialType = "ParentEntryID";

      if(!Serialization_deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &parentEntryIDLen, &parentEntryID, &parentBufLen) )
      {
         printk_fhgfs(KERN_INFO, "Deserialization failed: %s:\n", serialType);
         return fhgfs_false;
      }

      // Note: the root ("/") has parentEntryID = ""
      if (unlikely((parentEntryIDLen < MIN_ENTRY_ID_LEN) && (parentEntryIDLen > 0)) )
      {
         printk_fhgfs(KERN_INFO, "Deserialization failed: parentEntryIDLen too small.\n");
         return fhgfs_false;
      }

      bufPos += parentBufLen;
   }

   {
      // entryID
      unsigned entryBufLen;
      const char* serialType = "EntryID";

      if(!Serialization_deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &entryIDLen, &entryID, &entryBufLen) )
      {
         printk_fhgfs(KERN_INFO, "Deserialization failed: %s:\n", serialType);
         return fhgfs_false;
      }

      if (unlikely(entryIDLen < MIN_ENTRY_ID_LEN) )
      {
         printk_fhgfs(KERN_INFO, "Deserialization failed: entryIDLen too small.\n");
         return fhgfs_false;
      }

      bufPos += entryBufLen;
   }

   {
      // fileName
      unsigned fileNameBufLen;
      const char* serialType = "FileName";

      if(!Serialization_deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &fileNameLen, &fileName, &fileNameBufLen) )
      {
         printk_fhgfs(KERN_INFO, "Deserialization failed: %s:\n", serialType);
         return fhgfs_false;
      }

      bufPos += fileNameBufLen;
   }

   {
      // ownerNodeID
      unsigned ownerBufLen;
      const char* serialType = "OwnerNodeID";

      if(!Serialization_deserializeUShort(&buf[bufPos], bufLen-bufPos,
         &ownerNodeID, &ownerBufLen) )
      {
         printk_fhgfs(KERN_INFO, "Deserialization failed: %s:\n", serialType);
         return fhgfs_false;
      }

      bufPos += ownerBufLen;
   }

   {
      // padding for 4-byte alignment
      bufPos +=  Serialization_serialLenUShort();
   }

   EntryInfo_init(outThis, ownerNodeID, parentEntryID, entryID, fileName,
      (DirEntryType) entryType, statFlags);

   *outLen = bufPos;

   return fhgfs_true;
}

/**
 * Required size for serialization, 4-byte aligned
 */
unsigned EntryInfo_serialLen(const EntryInfo* this)
{
   return Serialization_serialLenUInt()                                 + // entryType
      Serialization_serialLenInt()                                      + // flags
      Serialization_serialLenStrAlign4(os_strlen(this->parentEntryID) ) +
      Serialization_serialLenStrAlign4(os_strlen(this->entryID) )       +
      Serialization_serialLenStrAlign4(os_strlen(this->fileName) )      +
      Serialization_serialLenUShort()                                   + // ownerNodeID
      Serialization_serialLenUShort();                                    // padding for alignment
}

