#include <common/storage/StorageErrors.h>
#include "MkDirRespMsg.h"

fhgfs_bool MkDirRespMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   MkDirRespMsg* thisCast = (MkDirRespMsg*)this;

   size_t bufPos = 0;

   { // result
      unsigned resultBufLen;

      if(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, &thisCast->result,
         &resultBufLen) )
         return fhgfs_false;

      bufPos += resultBufLen;
   }

   if ( (FhgfsOpsErr)thisCast->result == FhgfsOpsErr_SUCCESS)
   { // entryInfo, empty object if result != FhgfsOpsErr_SUCCESS, deserialization would fail then
      unsigned entryLen;

      if (!EntryInfo_deserialize(&buf[bufPos], bufLen-bufPos, &entryLen, &thisCast->entryInfo) )
         return fhgfs_false;

      bufPos += entryLen;
   }

   return fhgfs_true;
}


