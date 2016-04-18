#include "OpenFileRespMsg.h"


fhgfs_bool OpenFileRespMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   OpenFileRespMsg* thisCast = (OpenFileRespMsg*)this;

   size_t bufPos = 0;

   { // result
      unsigned resultFieldLen;

      if(!Serialization_deserializeInt(&buf[bufPos], bufLen-bufPos, &thisCast->result,
         &resultFieldLen) )
         return fhgfs_false;
   
      bufPos += resultFieldLen;
   }

   {  // fileHandleID
      unsigned handleBufLen;

      if(!Serialization_deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &thisCast->fileHandleIDLen, &thisCast->fileHandleID, &handleBufLen) )
         return fhgfs_false;

      bufPos += handleBufLen;
   }

   {  // pathInfo
      unsigned pathInfoBufLen;

      if (!PathInfo_deserialize(&buf[bufPos], bufLen-bufPos, &pathInfoBufLen, &thisCast->pathInfo) )
         return fhgfs_false;

      bufPos += pathInfoBufLen;
   }

   {  // stripePattern
      unsigned patternBufLen;

      if(!StripePattern_deserializePatternPreprocess(&buf[bufPos], bufLen-bufPos,
         &thisCast->patternHeader, &thisCast->patternStart, &patternBufLen) )
         return fhgfs_false;
      
      bufPos += patternBufLen;
   }

   return fhgfs_true;
}
