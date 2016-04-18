#include <common/toolkit/Serialization.h>

#include "ListXAttrRespMsg.h"

fhgfs_bool ListXAttrRespMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   ListXAttrRespMsg* thisCast = (ListXAttrRespMsg*)this;

   size_t bufPos = 0;
   const char** valueBufPtr = (const char**)&thisCast->valueBuf;

   { // value
      unsigned valueFieldLen;
      if (!Serialization_deserializeStrCpyVecPreprocess(&buf[bufPos], bufLen-bufPos,
          &thisCast->valueElemNum, valueBufPtr, &valueFieldLen) )
         return fhgfs_false;

      bufPos += valueFieldLen;
   }

   { // size
      unsigned sizeFieldLen;
      if (!Serialization_deserializeInt(&buf[bufPos], bufLen-bufPos, &thisCast->size,
          &sizeFieldLen) )
         return false;

      bufPos += sizeFieldLen;
   }

   { // returnCode
      unsigned returnCodeFieldLen;
      if (!Serialization_deserializeInt(&buf[bufPos], bufLen-bufPos, &thisCast->returnCode,
          &returnCodeFieldLen) )
         return fhgfs_false;

      bufPos += returnCodeFieldLen;
   }

   return fhgfs_true;
}
