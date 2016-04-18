#include "MkLocalDirMsg.h"


bool MkLocalDirMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   { // userID
      unsigned userIDLen;
      if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &userID, &userIDLen) )
         return false;

      bufPos += userIDLen;
   }

   { // groupID
      unsigned groupIDLen;
      if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &groupID, &groupIDLen) )
         return false;

      bufPos += groupIDLen;
   }

   { // mode
      unsigned modeLen;
      if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos, &mode, &modeLen) )
         return false;

      bufPos += modeLen;
   }

   { // entryInfo
      unsigned entryBufLen;
      if (!entryInfo.deserialize(&buf[bufPos], bufLen-bufPos, &entryBufLen) )
         return false;

      bufPos += entryBufLen;
   }

   { // stripePattern
      unsigned patternBufLen;
      if(!StripePattern::deserializePatternPreprocess(&buf[bufPos], bufLen-bufPos,
         &patternHeader, &patternStart, &patternBufLen) )
         return false;

      bufPos += patternBufLen;
   }

   { // parentNodeID
      unsigned parentNodeBufLen;

      if(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos,
         &parentNodeID, &parentNodeBufLen) )
         return false;

      bufPos += parentNodeBufLen;
   }

   if(isMsgHeaderFeatureFlagSet(MKLOCALDIRMSG_FLAG_MIRRORED) )
   { // mirrorNodeID
      unsigned mirrorBufLen;

      if(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos,
         &mirrorNodeID, &mirrorBufLen) )
         return false;

      bufPos += mirrorBufLen;
   }

#ifdef BEEGFS_HSM_DEPRECATED
   { // hsmCollocationID
      unsigned hsmCollocationIDBufLen;

      if ( !Serialization::deserializeUShort(&buf[bufPos], bufLen - bufPos, &hsmCollocationID,
         &hsmCollocationIDBufLen) )
         return false;

      bufPos += hsmCollocationIDBufLen;
   }
#endif

   {
      unsigned defaultACLXAttrBufLen;

      unsigned defaultACLVecElemNum;
      const char* defaultACLVecStart;
      if (!Serialization::deserializeCharVectorPreprocess(&buf[bufPos], bufLen-bufPos,
           &defaultACLVecElemNum, &defaultACLVecStart, &defaultACLXAttrBufLen) )
         return false;

      if (!Serialization::deserializeCharVector(defaultACLXAttrBufLen, defaultACLVecElemNum,
           defaultACLVecStart, &this->defaultACLXAttr) )
         return false;

      bufPos += defaultACLXAttrBufLen;
   }

   {
      unsigned accessACLXAttrBufLen;

      unsigned accessACLVecElemNum;
      const char* accessACLVecStart;
      if (!Serialization::deserializeCharVectorPreprocess(&buf[bufPos], bufLen-bufPos,
           &accessACLVecElemNum, &accessACLVecStart, &accessACLXAttrBufLen) )
         return false;

      if (!Serialization::deserializeCharVector(accessACLXAttrBufLen, accessACLVecElemNum,
           accessACLVecStart, &this->accessACLXAttr) )
         return false;

      bufPos += accessACLXAttrBufLen;
   }

   return true;
}

void MkLocalDirMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // userID
   bufPos += Serialization::serializeUInt(&buf[bufPos], userID);
   
   // groupID
   bufPos += Serialization::serializeUInt(&buf[bufPos], groupID);

   // mode
   bufPos += Serialization::serializeInt(&buf[bufPos], mode);

   // entryInfo
   bufPos += entryInfoPtr->serialize(&buf[bufPos]);

   // stripePattern
   bufPos += pattern->serialize(&buf[bufPos]);

   // parentNodeID
   bufPos += Serialization::serializeUShort(&buf[bufPos], parentNodeID);

   // mirrorNodeID
   if(isMsgHeaderFeatureFlagSet(MKLOCALDIRMSG_FLAG_MIRRORED) )
      bufPos += Serialization::serializeUShort(&buf[bufPos], mirrorNodeID);

#ifdef BEEGFS_HSM_DEPRECATED
   // hsmCollocationID
   bufPos += Serialization::serializeUShort(&buf[bufPos], hsmCollocationID);
#endif

   // defaultACLXAttr
   bufPos += Serialization::serializeCharVector(&buf[bufPos], &this->defaultACLXAttr);

   // accessACLXAttr
   bufPos += Serialization::serializeCharVector(&buf[bufPos], &this->accessACLXAttr);
}

