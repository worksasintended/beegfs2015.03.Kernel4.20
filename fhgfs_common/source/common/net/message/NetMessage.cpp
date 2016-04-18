#include <common/toolkit/serialization/Serialization.h>
#include "NetMessage.h"

/**
 * Reads the (common) header part of a message from a buffer.
 *
 * @outHeader msgType will be set to NETMSGTYPE_Invalid when a problem is detected.
 */
void NetMessage::deserializeHeader(char* buf, size_t bufLen, NetMessageHeader* outHeader)
{
   size_t bufPos = 0;
   
   // check min buffer length
   if(unlikely(bufLen < NETMSG_HEADER_LENGTH) )
   {
      outHeader->msgType = NETMSGTYPE_Invalid;
      return;
   }

   { // message length
      unsigned msgLenBufLen = 0;

      Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &outHeader->msgLength,
         &msgLenBufLen);
   
      bufPos += msgLenBufLen;
   }

   { // feature flags
      unsigned msgFeatureFlagsBufLen = 0;

      Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &outHeader->msgFeatureFlags,
         &msgFeatureFlagsBufLen);
   
      bufPos += msgFeatureFlagsBufLen;
   }

   // check message prefix
   if(unlikely(*(int64_t*) NETMSG_PREFIX_STR != *(int64_t*)&buf[bufPos] ) )
   {
      outHeader->msgType = NETMSGTYPE_Invalid;
      return;
   }

   bufPos += NETMSG_PREFIX_STR_LEN; // position after msgPrefix
   
   { // message type
      unsigned msgTypeBufLen;

      Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos, &outHeader->msgType,
         &msgTypeBufLen);

      bufPos += msgTypeBufLen;
   }
   
   { // feature flags
      unsigned msgCompatFeatureFlagsBufLen = 0;

      Serialization::deserializeUInt8(&buf[bufPos], bufLen-bufPos,
         &outHeader->msgCompatFeatureFlags, &msgCompatFeatureFlagsBufLen);

      bufPos += msgCompatFeatureFlagsBufLen;
   }

   // skip padding
   
   bufPos += Serialization::serialLenChar(); // paddingChar1

   { // userID
      unsigned msgUserIDBufLen = 0;

      Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &outHeader->msgUserID,
         &msgUserIDBufLen);

      bufPos += msgUserIDBufLen;
   }

   { // targetID
      unsigned msgTargetIDBufLen = 0;

      Serialization::deserializeUInt16(&buf[bufPos], bufLen-bufPos, &outHeader->msgTargetID,
         &msgTargetIDBufLen);

      bufPos += msgTargetIDBufLen;
   }

   // skip padding

   bufPos += Serialization::serialLenUInt16(); // paddingShort1
   bufPos += Serialization::serialLenUInt(); // paddingInt1
}


/**
 * Writes the (common) header part of a message to a buffer.
 * 
 * Note the min required size for the buf parameter! Message-specific data can be stored
 * from &buf[NETMSG_HEADER_LENGTH] on.
 * The msg->msgPrefix field is ignored and will always be stored correctly in the buffer.
 * 
 * @param buf min size is NETMSG_HEADER_LENGTH
 * @return false on error (e.g. bufLen too small), true otherwise
 */
void NetMessage::serializeHeader(char* buf)
{
   size_t bufPos = 0;
   
   // message length
   
   bufPos += Serialization::serializeUInt(&buf[bufPos], getMsgLength() );

   // feature flags

   bufPos += Serialization::serializeUInt(&buf[bufPos], getMsgHeaderFeatureFlags() );
   
   // message prefix
   
   memcpy(&buf[bufPos], NETMSG_PREFIX_STR, NETMSG_PREFIX_STR_LEN);
   
   bufPos += NETMSG_PREFIX_STR_LEN; // position after msgPrefix

   // message type
   
   bufPos += Serialization::serializeUShort(&buf[bufPos], this->msgHeader.msgType);

   // compat feature flags

   bufPos += Serialization::serializeUInt8(&buf[bufPos], getMsgHeaderCompatFeatureFlags() );

   // padding
   
   bufPos += Serialization::serializeChar(&buf[bufPos], 0); // paddingChar1

   // userID

   bufPos += Serialization::serializeUInt(&buf[bufPos], this->msgHeader.msgUserID);

   // targetID

   bufPos += Serialization::serializeUInt16(&buf[bufPos], this->msgHeader.msgTargetID);

   // padding

   bufPos += Serialization::serializeUInt16(&buf[bufPos], 0); // paddingShort1
   bufPos += Serialization::serializeUInt(&buf[bufPos], 0); // paddingInt1
}


