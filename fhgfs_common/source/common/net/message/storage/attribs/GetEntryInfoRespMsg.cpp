#include "GetEntryInfoRespMsg.h"

void GetEntryInfoRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // result
   bufPos += Serialization::serializeInt(&buf[bufPos], result);

   // stripePattern
   bufPos += pattern->serialize(&buf[bufPos]);
   
   // pathInfo
   bufPos += this->pathInfoPtr->serialize(&buf[bufPos]);

   // mirrorNodeID
   bufPos += Serialization::serializeUInt16(&buf[bufPos], mirrorNodeID);
}

bool GetEntryInfoRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   {  // result
      unsigned resultFieldLen;
      if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos, &result, &resultFieldLen) )
         return false;

      bufPos += resultFieldLen;
   }
   
   {  // stripePattern
      unsigned patternBufLen;
      if(!StripePattern::deserializePatternPreprocess(&buf[bufPos], bufLen-bufPos,
         &patternHeader, &patternStart, &patternBufLen) )
         return false;

      bufPos += patternBufLen;
   }

   { // pathInfo
      unsigned fieldLen;

      if (!this->pathInfo.deserialize(&buf[bufPos], bufLen-bufPos, &fieldLen) )
         return false;

      bufPos += fieldLen;
   }

   {  // mirrorNodeID
      unsigned mirrorFieldLen;
      if(!Serialization::deserializeUInt16(&buf[bufPos], bufLen-bufPos,
         &mirrorNodeID, &mirrorFieldLen) )
         return false;

      bufPos += mirrorFieldLen;
   }



   return true;
}



