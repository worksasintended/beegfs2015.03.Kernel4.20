#include <common/toolkit/Serialization.h>

#include "GetXAttrRespMsg.h"

fhgfs_bool GetXAttrRespMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   GetXAttrRespMsg* thisCast = (GetXAttrRespMsg*)this;

   size_t bufPos = 0;
   const char** valueBufPtr = (const char**)&thisCast->valueBuf;

   {
      unsigned valueFieldLen;
      if (!Serialization_deserializeCharArray(&buf[bufPos], bufLen-bufPos, &thisCast->valueBufLen,
          valueBufPtr, &valueFieldLen) )
         return fhgfs_false;

      bufPos += valueFieldLen;
   }

   {
      unsigned sizeFieldLen;
      if (!Serialization_deserializeInt(&buf[bufPos], bufLen-bufPos, &thisCast->size,
          &sizeFieldLen) )
         return fhgfs_false;

      bufPos += sizeFieldLen;
   }

   {
      unsigned returnCodeFieldLen;
      if (!Serialization_deserializeInt(&buf[bufPos], bufLen-bufPos, &thisCast->returnCode,
          &returnCodeFieldLen) )
         return fhgfs_false;

      bufPos += returnCodeFieldLen;
   }

   return fhgfs_true;
}
