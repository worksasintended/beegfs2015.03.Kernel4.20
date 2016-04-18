#include "TruncLocalFileMsg.h"


bool TruncLocalFileMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   {  // filesize
      unsigned filesizeLen;
      if(!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos, &filesize, &filesizeLen) )
         return false;

      bufPos += filesizeLen;
   }

   if(isMsgHeaderFeatureFlagSet(TRUNCLOCALFILEMSG_FLAG_USE_QUOTA))
   {
      // userID

      unsigned userIDLen;
      if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &userID, &userIDLen) )
         return false;

      bufPos += userIDLen;

      // groupID

      unsigned groupIDLen;
      if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &groupID, &groupIDLen) )
         return false;

      bufPos += groupIDLen;
   }

   {  // entryID
      unsigned entryBufLen;

      if(!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &entryIDLen, &entryID, &entryBufLen) )
         return false;

      bufPos += entryBufLen;
   }

   {  // PathInfo
      unsigned pathInfoBufLen;

      if (!this->pathInfo.deserialize(&buf[bufPos], bufLen-bufPos, &pathInfoBufLen) )
         return false;

      this->pathInfoPtr = &this->pathInfo; // to allow re-serialization of this msg object

      bufPos += pathInfoBufLen;
   }

   {  // targetID
      unsigned targetBufLen;

      if(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos,
         &targetID, &targetBufLen) )
         return false;

      bufPos += targetBufLen;
   }

   return true;
}

void TruncLocalFileMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // filesize
   bufPos += Serialization::serializeInt64(&buf[bufPos], filesize);

   if(isMsgHeaderFeatureFlagSet(TRUNCLOCALFILEMSG_FLAG_USE_QUOTA))
   {
      // userID
      bufPos += Serialization::serializeUInt(&buf[bufPos], userID);

      // groupID
      bufPos += Serialization::serializeUInt(&buf[bufPos], groupID);
   }

   // entryID
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], entryIDLen, entryID);

   // pathInfo
   bufPos += this->pathInfoPtr->serialize(&buf[bufPos]);

   // targetID
   bufPos += Serialization::serializeUShort(&buf[bufPos], targetID);
}


