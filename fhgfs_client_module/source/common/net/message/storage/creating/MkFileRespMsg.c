#include <common/storage/StorageErrors.h>
#include "MkFileRespMsg.h"


fhgfs_bool MkFileRespMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   MkFileRespMsg* thisCast = (MkFileRespMsg*)this;

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
      unsigned entryBufLen;

      if (!EntryInfo_deserialize(&buf[bufPos], bufLen-bufPos, &entryBufLen, &thisCast->entryInfo) )
         return fhgfs_false;

      bufPos += entryBufLen;
   }

   return fhgfs_true;
}
