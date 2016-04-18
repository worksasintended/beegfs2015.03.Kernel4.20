#include "OpenFileRespMsg.h"

void OpenFileRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // result
   bufPos += Serialization::serializeInt(&buf[bufPos], result);

   // fileHandleID
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], fileHandleIDLen, fileHandleID);
   
   // pathInfo
   bufPos += this->pathInfo.serialize(&buf[bufPos]);

   // stripePattern
   bufPos += pattern->serialize(&buf[bufPos]);
   
}

bool OpenFileRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   {  // result
      unsigned resultFieldLen;
      if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos, &result, &resultFieldLen) )
         return false;

      bufPos += resultFieldLen;
   }

   
   {  // fileHandleID
      unsigned handleBufLen;
      if(!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &fileHandleIDLen, &fileHandleID, &handleBufLen) )
         return false;

      bufPos += handleBufLen;
   }

   {  // pathInfo
      unsigned pathInfoBufLen;

      if (!this->pathInfo.deserialize(&buf[bufPos], bufLen-bufPos, &pathInfoBufLen) )
         return false;

      bufPos += pathInfoBufLen;
   }

   {   // stripePattern
      unsigned patternBufLen;
      if(!StripePattern::deserializePatternPreprocess(&buf[bufPos], bufLen-bufPos,
         &patternHeader, &patternStart, &patternBufLen) )
         return false;
      
      bufPos += patternBufLen;
   }

   return true;
}



