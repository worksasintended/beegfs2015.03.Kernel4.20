/*
 * Information provided by stat()
 */

#include <common/toolkit/Serialization.h>
#include "StatData.h"

fhgfs_bool StatData_deserialize(const char* buf, size_t bufLen, StatData* outThis, unsigned* outLen)
{

   /* TODO: All those integers could be deserialized using a type conversion
    * (my_struct) (buf), which would simplify the code and also be faster.
    * The data just could not be used between architectures, or we would need to introduce
    * endian conversions */

   unsigned bufPos = 0;

   { // flags
      unsigned flagFieldLen;
      const char* serialType = "flags";

      if(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, &outThis->flags,
         &flagFieldLen) )
      {
         printk_fhgfs(KERN_INFO, "Deserialization failed: type: %s bufPos: %d bufLen: %d\n",
            serialType, bufPos, bufLen-bufPos);
         return fhgfs_false;
      }

      bufPos += flagFieldLen;
   }

   { // mode
      unsigned modeFieldLen;
      const char* serialType = "mode";

      if(!Serialization_deserializeInt(&buf[bufPos], bufLen-bufPos,
         &outThis->settableFileAttribs.mode, &modeFieldLen) )
      {
         printk_fhgfs(KERN_INFO, "Deserialization failed: type: %s bufPos: %d bufLen: %d\n",
            serialType, bufPos, bufLen-bufPos);
         return fhgfs_false;
      }

      bufPos += modeFieldLen;
   }

   {  // sumChunkBlocks
      unsigned fieldLen;
      const char* serialType = "sumChunkBlocks";

      if(!Serialization_deserializeUInt64(&buf[bufPos], bufLen-bufPos, &outThis->numBlocks,
         &fieldLen) )
      {
         printk_fhgfs(KERN_INFO, "Deserialization failed: type: %s bufPos: %d bufLen: %d\n",
            serialType, bufPos, bufLen-bufPos);
         return fhgfs_false;
      }

      bufPos += fieldLen;
   }

   { // creationTime
      unsigned createTimeFieldLen;
      const char* serialType = "creationTime";

      if(!Serialization_deserializeInt64(&buf[bufPos], bufLen-bufPos,
         &outThis->creationTimeSecs, &createTimeFieldLen) )
      {
         printk_fhgfs(KERN_INFO, "Deserialization failed: type: %s bufPos: %d bufLen: %d\n",
            serialType, bufPos, bufLen-bufPos);
         return fhgfs_false;
      }

      bufPos += createTimeFieldLen;
   }

   { // aTime
      unsigned aTimeFieldLen;
      const char* serialType = "aTime";

      if(!Serialization_deserializeInt64(&buf[bufPos], bufLen-bufPos,
         &outThis->settableFileAttribs.lastAccessTimeSecs, &aTimeFieldLen) )
      {
         printk_fhgfs(KERN_INFO, "Deserialization failed: type: %s bufPos: %d bufLen: %d\n",
            serialType, bufPos, bufLen-bufPos);
         return fhgfs_false;
      }

      bufPos += aTimeFieldLen;
   }

   { // mtime
      unsigned mTimeFieldLen;
      const char* serialType = "mTime";

      if(!Serialization_deserializeInt64(&buf[bufPos], bufLen-bufPos,
         &outThis->settableFileAttribs.modificationTimeSecs, &mTimeFieldLen) )
      {
         printk_fhgfs(KERN_INFO, "Deserialization failed: type: %s bufPos: %d bufLen: %d\n",
            serialType, bufPos, bufLen-bufPos);
         return fhgfs_false;
      }

      bufPos += mTimeFieldLen;
   }

   { // ctime
      unsigned cTimeFieldLen;
      const char* serialType = "cTime";

      if(!Serialization_deserializeInt64(&buf[bufPos], bufLen-bufPos,
         &outThis->attribChangeTimeSecs, &cTimeFieldLen) )
      {
         printk_fhgfs(KERN_INFO, "Deserialization failed: type: %s bufPos: %d bufLen: %d\n",
            serialType, bufPos, bufLen-bufPos);
         return fhgfs_false;
      }

      bufPos += cTimeFieldLen;
   }

   { // fileSize
      unsigned sizeFieldLen;
      const char* serialType = "fileSize";

      if(!Serialization_deserializeInt64(&buf[bufPos], bufLen-bufPos,
         &outThis->fileSize, &sizeFieldLen) )
      {
         printk_fhgfs(KERN_INFO, "Deserialization failed: type: %s bufPos: %d bufLen: %d\n",
            serialType, bufPos, bufLen-bufPos);
         return fhgfs_false;
      }

      bufPos += sizeFieldLen;
   }

   { // nlink
      unsigned nlinkFieldLen;
      const char* serialType = "nlink";

      if(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos,
         &outThis->nlink, &nlinkFieldLen) )
      {
         printk_fhgfs(KERN_INFO, "Deserialization failed: type: %s bufPos: %d bufLen: %d\n",
            serialType, bufPos, bufLen-bufPos);
         return fhgfs_false;
      }

      bufPos += nlinkFieldLen;
   }

   { // contentsVersion
      unsigned contentsFieldLen;
      const char* serialType = "contentsVersion";

      if(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos,
         &outThis->contentsVersion, &contentsFieldLen) )
      {
         printk_fhgfs(KERN_INFO, "Deserialization failed: type: %s bufPos: %d bufLen: %d\n",
            serialType, bufPos, bufLen-bufPos);
         return fhgfs_false;
      }

      bufPos += contentsFieldLen;
   }

   { // uid
      unsigned uidFieldLen;
      const char* serialType = "uid";

      if(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos,
         &outThis->settableFileAttribs.userID, &uidFieldLen) )
      {
         printk_fhgfs(KERN_INFO, "Deserialization failed: type: %s bufPos: %d bufLen: %d\n",
            serialType, bufPos, bufLen-bufPos);
         return fhgfs_false;
      }

      bufPos += uidFieldLen;
   }

   { // gid
      unsigned gidFieldLen;
      const char* serialType = "gid";

      if(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos,
         &outThis->settableFileAttribs.groupID, &gidFieldLen) )
      {
         printk_fhgfs(KERN_INFO, "Deserialization failed: type: %s bufPos: %d bufLen: %d\n",
            serialType, bufPos, bufLen-bufPos);
         return fhgfs_false;
      }

      bufPos += gidFieldLen;
   }

   *outLen = bufPos;

   return fhgfs_true;
}



