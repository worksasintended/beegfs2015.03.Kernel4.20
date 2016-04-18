#include "WriteLocalFileMsg.h"

bool WriteLocalFileMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   {  // offset
      unsigned offsetLen;
      if(!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos, &offset, &offsetLen) )
         return false;

      bufPos += offsetLen;
   }

   {  // count
      unsigned countLen;
      if(!Serialization::deserializeInt64(&buf[bufPos], bufLen-bufPos, &count, &countLen) )
         return false;

      bufPos += countLen;
   }

   {  // accessFlags
      unsigned accessFlagsLen;
      if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &accessFlags, &accessFlagsLen) )
         return false;
   
      bufPos += accessFlagsLen;
   }
   
   if(isMsgHeaderFeatureFlagSet(WRITELOCALFILEMSG_FLAG_USE_QUOTA))
   {
      {  // userID
         unsigned userIDLen;
         if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &userID, &userIDLen) )
            return false;

         bufPos += userIDLen;
      }

      {  // groupID
         unsigned groupIDLen;
         if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &groupID, &groupIDLen) )
            return false;

         bufPos += groupIDLen;
      }
   }

   {  // fileHandleID
      unsigned handleBufLen;

      if(!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &fileHandleIDLen, &fileHandleID, &handleBufLen) )
         return false;

      bufPos += handleBufLen;
   }

   {  // sessionID
      unsigned sessionBufLen;

      if(!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &sessionIDLen, &sessionID, &sessionBufLen) )
         return false;

      bufPos += sessionBufLen;
   }

   {  // PathInfo
      unsigned pathInfoBufLen;

      if (!this->pathInfo.deserialize(&buf[bufPos], bufLen-bufPos, &pathInfoBufLen) )
         return false;

      bufPos += pathInfoBufLen;
   }

   {  // targetID
      unsigned targetBufLen;
   
      if(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos, &targetID, &targetBufLen) )
         return false;
   
      bufPos += targetBufLen;
   }

   return true;
}

void WriteLocalFileMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // offset
   bufPos += Serialization::serializeInt64(&buf[bufPos], offset);

   // count
   bufPos += Serialization::serializeInt64(&buf[bufPos], count);
   
   // accessFlags
   bufPos += Serialization::serializeUInt(&buf[bufPos], accessFlags);

   if(isMsgHeaderFeatureFlagSet(WRITELOCALFILEMSG_FLAG_USE_QUOTA))
   {
      // userID
      bufPos += Serialization::serializeUInt(&buf[bufPos], userID);

      // groupID
      bufPos += Serialization::serializeUInt(&buf[bufPos], groupID);
   }

   // fileHandleID
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], fileHandleIDLen, fileHandleID);

   // sessionID
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], sessionIDLen, sessionID);

   // pathInfo
   bufPos += this->pathInfoPtr->serialize(&buf[bufPos]);

   // targetID
   bufPos += Serialization::serializeUShort(&buf[bufPos], targetID);
}
