#include "SetDirPatternMsg.h"

void SetDirPatternMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // entryInfo
   bufPos += entryInfoPtr->serialize(&buf[bufPos]);
   
   // stripePattern
   bufPos += pattern->serialize(&buf[bufPos]);

   if (isMsgHeaderFeatureFlagSet(Flags::HAS_UID))
      bufPos += Serialization::serializeUInt(buf + bufPos, uid);
}

bool SetDirPatternMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   { // entryInfo
      unsigned entryBufLen;

      bool deserRes = entryInfo.deserialize(&buf[bufPos], bufLen-bufPos, &entryBufLen);
      if(unlikely(!deserRes) )
         return false;

      bufPos += entryBufLen;
   }

   {
      // stripePattern
      unsigned patternBufLen;
      if(!StripePattern::deserializePatternPreprocess(&buf[bufPos], bufLen-bufPos,
         &patternHeader, &patternStart, &patternBufLen) )
         return false;

      bufPos += patternBufLen;
   }

   if (isMsgHeaderFeatureFlagSet(Flags::HAS_UID))
   {
      unsigned uidBufLen;
      if (!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &uid, &uidBufLen))
         return false;

      bufPos += uidBufLen;
   }

   return true;
}



