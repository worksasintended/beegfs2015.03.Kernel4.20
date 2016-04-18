#include "ResyncLocalFileMsg.h"

bool ResyncLocalFileMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   { // relativePathStr
      unsigned relativePathStrBufLen;

      if ( !Serialization::deserializeStrAlign4(&buf[bufPos], bufLen - bufPos, &relativePathStr,
         &relativePathStrBufLen) )
         return false;

      bufPos += relativePathStrBufLen;
   }

   { // resyncToTargetID
      unsigned resyncToTargetIDBufLen;

      if ( !Serialization::deserializeUShort(&buf[bufPos], bufLen - bufPos, &resyncToTargetID,
         &resyncToTargetIDBufLen) )
         return false;

      bufPos += resyncToTargetIDBufLen;
   }

   { // offset
      unsigned offsetLen;
      if ( !Serialization::deserializeInt64(&buf[bufPos], bufLen - bufPos, &offset, &offsetLen) )
         return false;

      bufPos += offsetLen;
   }

   {
      // count
      unsigned countLen;
      if ( !Serialization::deserializeInt(&buf[bufPos], bufLen - bufPos, &count, &countLen) )
         return false;

      bufPos += countLen;
   }

   if ( this->isMsgHeaderFeatureFlagSet(RESYNCLOCALFILEMSG_FLAG_SETATTRIBS) )
   {
      {
         // mode
         unsigned modeLen;
         if ( !Serialization::deserializeInt(&buf[bufPos], bufLen - bufPos, &(chunkAttribs.mode),
            &modeLen) )
            return false;

         bufPos += modeLen;
      }

      {
         // userID
         unsigned userIDLen;
         if ( !Serialization::deserializeUInt(&buf[bufPos], bufLen - bufPos, &(chunkAttribs.userID),
            &userIDLen) )
            return false;

         bufPos += userIDLen;
      }

      {
         // groupID
         unsigned groupIDLen;
         if ( !Serialization::deserializeUInt(&buf[bufPos], bufLen - bufPos,
            &(chunkAttribs.groupID), &groupIDLen) )
            return false;

         bufPos += groupIDLen;
      }
   }

   { // dataBuf
      dataBuf = &buf[bufPos];
      bufPos += count;
   }

   return true;
}

void ResyncLocalFileMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // relativePathStr
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], relativePathStr.length(),
      relativePathStr.c_str());

   // resyncToTargetID
   bufPos += Serialization::serializeUInt16(&buf[bufPos], resyncToTargetID);

   // offset
   bufPos += Serialization::serializeInt64(&buf[bufPos], offset);

   // count
   bufPos += Serialization::serializeInt(&buf[bufPos], count);

   if ( this->isMsgHeaderFeatureFlagSet(RESYNCLOCALFILEMSG_FLAG_SETATTRIBS) )
   {
      // mode
      bufPos += Serialization::serializeInt(&buf[bufPos], chunkAttribs.mode);

      // userID
      bufPos += Serialization::serializeUInt(&buf[bufPos], chunkAttribs.userID);

      // groupID
      bufPos += Serialization::serializeUInt(&buf[bufPos], chunkAttribs.groupID);
   }

   // dataBuf
   if (dataBuf)
   {
      memcpy(&buf[bufPos], dataBuf, count);
      bufPos += count;
   }
}
