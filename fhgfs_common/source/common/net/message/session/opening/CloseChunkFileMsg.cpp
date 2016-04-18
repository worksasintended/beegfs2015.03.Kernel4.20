#include "CloseChunkFileMsg.h"


bool CloseChunkFileMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   {  // sessionID
      unsigned sessionBufLen;

      if(!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &sessionIDLen, &sessionID, &sessionBufLen) )
         return false;

      bufPos += sessionBufLen;
   }
   
   {  // fileHandleID
      unsigned handleBufLen;

      if(!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
         &fileHandleIDLen, &fileHandleID, &handleBufLen) )
         return false;

      bufPos += handleBufLen;
   }

   {  // PathInfo
      unsigned pathBufLen;

      if (!this->pathInfo.deserialize(&buf[bufPos], bufLen-bufPos, &pathBufLen) )
         return false;

      this->pathInfoPtr = &this->pathInfo; // to allow re-serialization of this msg object

      bufPos += pathBufLen;
   }

   if ( this->isMsgHeaderFeatureFlagSet(CLOSECHUNKFILEMSG_FLAG_WRITEENTRYINFO) )
   {
      {  // entryInfoBufLen
         unsigned entryInfoBufLenBufLen;

         if ( !Serialization::deserializeUInt(&buf[bufPos], bufLen - bufPos, &entryInfoBufLen,
            &entryInfoBufLenBufLen) )
            return false;

         bufPos += entryInfoBufLenBufLen;
      }

      {  // entryInfoBuf
         this->entryInfoBuf = &buf[bufPos];

         bufPos += this->entryInfoBufLen;
      }
   }

   {  // targetID
      unsigned targetBufLen;

      if(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos,
         &targetID, &targetBufLen) )
         return false;

      bufPos += targetBufLen;
   }

#ifdef BEEGFS_HSM_DEPRECATED
   {  // collocationID
      unsigned hsmCollocationIDBufLen;

      if(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos,
         &(this->hsmCollocationID), &hsmCollocationIDBufLen) )
         return false;

      bufPos += hsmCollocationIDBufLen;
   }

#endif

   return true;
}

void CloseChunkFileMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // sessionID
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], sessionIDLen, sessionID);
   
   // fileHandleID
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], fileHandleIDLen, fileHandleID);

   // pathInfo
   bufPos += this->pathInfoPtr->serialize(&buf[bufPos]);

   if ( isMsgHeaderFeatureFlagSet(CLOSECHUNKFILEMSG_FLAG_WRITEENTRYINFO) )
   {
      // entryInfoBufLen
      bufPos += Serialization::serializeUInt(&buf[bufPos], entryInfoBufLen);

      // entryInfoBuf
      memcpy(&buf[bufPos], entryInfoBuf, entryInfoBufLen);
      bufPos += entryInfoBufLen;
   }

   // targetID
   bufPos += Serialization::serializeUShort(&buf[bufPos], targetID);

#ifdef BEEGFS_HSM_DEPRECATED
   // collocationID
   bufPos += Serialization::serializeUShort(&buf[bufPos], this->hsmCollocationID);
#endif
}

