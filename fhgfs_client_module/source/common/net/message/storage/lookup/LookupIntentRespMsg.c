#include "LookupIntentRespMsg.h"


fhgfs_bool LookupIntentRespMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   LookupIntentRespMsg* thisCast = (LookupIntentRespMsg*)this;

   size_t bufPos = 0;

   // pre-initialize
   thisCast->lookupResult = FhgfsOpsErr_INTERNAL;
   thisCast->createResult = FhgfsOpsErr_INTERNAL;

   { // responseFlags
      unsigned responseFlagsBufLen;

      // printk_fhgfs_debug(KERN_INFO, "%s:%d\n", __func__, __LINE__);

      if(!Serialization_deserializeInt(&buf[bufPos], bufLen-bufPos, &thisCast->responseFlags,
         &responseFlagsBufLen) )
         return fhgfs_false;

      bufPos += responseFlagsBufLen;
   }

   { // lookupResult
      unsigned lookupResultBufLen;

      // printk_fhgfs_debug(KERN_INFO, "%s:%d\n", __func__, __LINE__);

      if(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, &thisCast->lookupResult,
         &lookupResultBufLen) )
         return fhgfs_false;

      bufPos += lookupResultBufLen;
   }

   if(thisCast->responseFlags & LOOKUPINTENTRESPMSG_FLAG_STAT)
   {

      // printk_fhgfs_debug(KERN_INFO, "%s:%d\n", __func__, __LINE__);

      { // statResult
         unsigned statResultFieldLen;

         if(!Serialization_deserializeInt(&buf[bufPos], bufLen-bufPos, &thisCast->statResult,
            &statResultFieldLen) )
            return fhgfs_false;

         bufPos += statResultFieldLen;
      }

      { // StatData
         unsigned statFieldLen;

         if(!StatData_deserialize(&buf[bufPos], bufLen-bufPos, &thisCast->statData, &statFieldLen) )
            return fhgfs_false;

         bufPos += statFieldLen;
         //printk_fhgfs_debug(KERN_INFO, "%s:%d statLen: %d bufPos %d\n",
         //   __func__, __LINE__, statFieldLen, bufPos);
      }


      // printk_fhgfs_debug(KERN_INFO, "%s:%d\n", __func__, __LINE__);

}

   if(thisCast->responseFlags & LOOKUPINTENTRESPMSG_FLAG_REVALIDATE)
   {
      unsigned revalidateResultFieldLen;

      // printk_fhgfs_debug(KERN_INFO, "%s:%d\n", __func__, __LINE__);

      // revalidateResult
      if(!Serialization_deserializeInt(&buf[bufPos], bufLen-bufPos,
         &thisCast->revalidateResult, &revalidateResultFieldLen) )
         return fhgfs_false;

      // printk_fhgfs_debug(KERN_INFO, "%s:%d\n", __func__, __LINE__);
      bufPos += revalidateResultFieldLen;
   }

   if(thisCast->responseFlags & LOOKUPINTENTRESPMSG_FLAG_CREATE)
   {
      unsigned createResultFieldLen;

      // printk_fhgfs_debug(KERN_INFO, "%s:%d\n", __func__, __LINE__);

      // createResult
      if(!Serialization_deserializeInt(&buf[bufPos], bufLen-bufPos,
         &thisCast->createResult, &createResultFieldLen) )
         return fhgfs_false;

      bufPos += createResultFieldLen;
   }

   if(thisCast->responseFlags & LOOKUPINTENTRESPMSG_FLAG_OPEN)
   {
      // printk_fhgfs_debug(KERN_INFO, "%s:%d\n", __func__, __LINE__);

      {  // openResult
         unsigned openResultFieldLen;

         if(!Serialization_deserializeInt(&buf[bufPos], bufLen-bufPos,
            &thisCast->openResult, &openResultFieldLen) )
            return fhgfs_false;

         bufPos += openResultFieldLen;
      }

      {  // fileHandleID
         unsigned handleBufLen;

         if(!Serialization_deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
            &thisCast->fileHandleIDLen, &thisCast->fileHandleID, &handleBufLen) )
            return fhgfs_false;

         bufPos += handleBufLen;
      }

      {  // stripePattern
         unsigned patternBufLen;

         if(!StripePattern_deserializePatternPreprocess(&buf[bufPos], bufLen-bufPos,
            &thisCast->patternHeader, &thisCast->patternStart, &patternBufLen) )
            return fhgfs_false;

         bufPos += patternBufLen;
      }

      {  // PathInfo
         unsigned pathInfoBufLen;
         if(!PathInfo_deserialize(&buf[bufPos], bufLen-bufPos, &pathInfoBufLen,
            &thisCast->pathInfo) )
            return fhgfs_false;

         bufPos += pathInfoBufLen;
      }
   }

   //printk_fhgfs_debug(KERN_INFO, "%s:%d lookupRes: %d createRes: %d \n",
   //   __func__, __LINE__, thisCast->lookupResult, thisCast->createResult);

   // only try to decode the EntryInfo if either of those was successful
   if ( (thisCast->lookupResult == FhgfsOpsErr_SUCCESS) ||
        (thisCast->createResult == FhgfsOpsErr_SUCCESS) )
   { // entryInfo
      unsigned entryBufLen;

      if (unlikely(
         !EntryInfo_deserialize(&buf[bufPos], bufLen-bufPos, &entryBufLen, &thisCast->entryInfo) ) )
      {
         printk_fhgfs(KERN_INFO,
            "EntryInfo deserialization failed. LookupResult: %d; CreateResult %d\n",
            thisCast->lookupResult, thisCast->createResult);
         return fhgfs_false;
      }

      // printk_fhgfs_debug(KERN_INFO, "%s:%d\n", __func__, __LINE__);
      bufPos += entryBufLen;
   }


   return fhgfs_true;
}


