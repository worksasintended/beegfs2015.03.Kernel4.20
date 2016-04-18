#include "FsckSetEventLoggingMsg.h"

bool FsckSetEventLoggingMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   {
      // enableLogging
      unsigned enableLoggingBufLen;

      if ( !Serialization::deserializeBool(&buf[bufPos], bufLen - bufPos,
         &(this->enableLogging), &enableLoggingBufLen) )
         return false;

      bufPos += enableLoggingBufLen;
   }

   {
      // portUDP
      unsigned portUDPBufLen;

      if ( !Serialization::deserializeUInt(&buf[bufPos], bufLen - bufPos,
         &(this->portUDP), &portUDPBufLen) )
         return false;

      bufPos += portUDPBufLen;
   }

   {
      // nicList
      unsigned nicListBufLen;

      if ( !Serialization::deserializeNicListPreprocess(&buf[bufPos], bufLen - bufPos,
         &(this->nicListElemNum), &(this->nicListStart), &nicListBufLen) )
         return false;

      bufPos += nicListBufLen;
   }

   return true;
}

void FsckSetEventLoggingMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // enableLogging
   bufPos += Serialization::serializeBool(&buf[bufPos], this->enableLogging);

   // portUDP
   bufPos += Serialization::serializeUInt(&buf[bufPos], this->portUDP);

   // nicList
   bufPos += Serialization::serializeNicList(&buf[bufPos], this->nicList);
}
