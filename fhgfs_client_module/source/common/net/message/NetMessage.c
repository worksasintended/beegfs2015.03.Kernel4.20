#include "NetMessage.h"

/**
 * Processes this message.
 *
 * Note: Some messages might be received over a datagram socket, so the response
 * must be atomic (=> only a single sendto()-call)
 *
 * @param fromAddr must be NULL for stream sockets
 * @return fhgfs_false on error
 */
fhgfs_bool NetMessage_processIncoming(NetMessage* this, struct App* app,
   fhgfs_sockaddr_in* fromAddr, struct Socket* sock, char* respBuf, size_t bufLen)
{
   // Note: Has to be implemented appropriately by derived classes.
   // Empty implementation provided here for invalid messages and other messages
   // that don't require this way of processing (e.g. some response messages).

   return fhgfs_false;
}

/**
 * Returns all feature flags that are supported by this message. Defaults to "none", so this
 * method needs to be overridden by derived messages that actually support header feature
 * flags.
 *
 * @return combination of all supported feature flags
 */
unsigned NetMessage_getSupportedHeaderFeatureFlagsMask(NetMessage* this)
{
   return 0;
}

/**
 * Reads the (common) header part of a message from a buffer.
 *
 * Note: Message type will be set to NETMSGTYPE_Invalid if deserialization fails.
 */
void __NetMessage_deserializeHeader(char* buf, size_t bufLen, NetMessageHeader* outHeader)
{
   size_t bufPos = 0;

   // check min buffer length

   if(unlikely(bufLen < NETMSG_HEADER_LENGTH) )
   {
      outHeader->msgType = NETMSGTYPE_Invalid;
      return;
   }

   { // message length
      unsigned msgLenBufLen = 0; // "=0" to mute false compiler warnings

      Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, &outHeader->msgLength, &msgLenBufLen);

      bufPos += msgLenBufLen;
   }
   
   // verify contained msg length

   if(unlikely(outHeader->msgLength != bufLen) )
   {
      outHeader->msgType = NETMSGTYPE_Invalid;
      return;
   }

   { // feature flags
      unsigned msgFeatureFlagsBufLen = 0; // "=0" to mute false compiler warnings

      Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, &outHeader->msgFeatureFlags,
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
      unsigned msgTypeBufLen = 0; // "=0" to mute false compiler warnings

      Serialization_deserializeUShort(&buf[bufPos], bufLen-bufPos, &outHeader->msgType,
         &msgTypeBufLen);

      bufPos += msgTypeBufLen;
   }
   
   { // compat feature flags
      unsigned msgCompatFeatureFlagsBufLen = 0; // "=0" to mute false compiler warnings

      Serialization_deserializeChar(&buf[bufPos], bufLen-bufPos, &outHeader->msgCompatFeatureFlags,
         &msgCompatFeatureFlagsBufLen);

      bufPos += msgCompatFeatureFlagsBufLen;
   }

   // skip padding

   bufPos += Serialization_serialLenChar(); // paddingChar1

   { // userID
      unsigned msgUserIDBufLen = 0; // "=0" to mute false compiler warnings

      Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, &outHeader->msgUserID,
         &msgUserIDBufLen);

      bufPos += msgUserIDBufLen;
   }

   { // targetID
      unsigned msgTargetIDBufLen = 0; // "=0" to mute false compiler warnings

      Serialization_deserializeUShort(&buf[bufPos], bufLen-bufPos, &outHeader->msgTargetID,
         &msgTargetIDBufLen);

      bufPos += msgTargetIDBufLen;
   }

   // skip padding

   bufPos += Serialization_serialLenUShort(); // paddingShort1
   bufPos += Serialization_serialLenUInt(); // paddingInt1
}


/**
 * Writes the (common) header part of a message to a buffer.
 *
 * Note the min required size for the buf parameter! Message-specific data can be stored
 * from &buf[NETMSG_HEADER_LENGTH] on.
 * The msg->msgPrefix field is ignored and will always be stored correctly in the buffer.
 *
 * @param buf min size is NETMSG_HEADER_LENGTH
 * @return fhgfs_false on error (e.g. bufLen too small), fhgfs_true otherwise
 */
void __NetMessage_serializeHeader(NetMessage* this, char* buf)
{
   size_t bufPos = 0;

   // message length

   bufPos += Serialization_serializeUInt(&buf[bufPos], NetMessage_getMsgLength(this) );

   // feature flags

   bufPos += Serialization_serializeUInt(&buf[bufPos], this->msgHeader.msgFeatureFlags);

   // message prefix

   os_memcpy(&buf[bufPos], NETMSG_PREFIX_STR, NETMSG_PREFIX_STR_LEN);

   bufPos += NETMSG_PREFIX_STR_LEN; // position after msgPrefix

   // message type

   bufPos += Serialization_serializeUShort(&buf[bufPos], this->msgHeader.msgType);

   // compat feature flags

   bufPos += Serialization_serializeChar(&buf[bufPos], this->msgHeader.msgCompatFeatureFlags);

   // padding

   bufPos += Serialization_serializeChar(&buf[bufPos], 0); // paddingChar1

   // userID

   bufPos += Serialization_serializeUInt(&buf[bufPos], this->msgHeader.msgUserID);

   // targetID

   bufPos += Serialization_serializeUShort(&buf[bufPos], this->msgHeader.msgTargetID);

   // padding

   bufPos += Serialization_serializeUShort(&buf[bufPos], 0); // paddingShort1
   bufPos += Serialization_serializeUInt(&buf[bufPos], 0); // paddingInt1
}

/**
 * Dummy function for deserialize pointers
 */
fhgfs_bool _NetMessage_deserializeDummy(NetMessage* this, const char* buf, size_t bufLen)
{
   printk_fhgfs(KERN_INFO, "Bug: Deserialize function called, although it should not\n");
   os_dumpStack();

   return fhgfs_true;
}

/**
 * Dummy function for serialize pointers
 */
void _NetMessage_serializeDummy(NetMessage* this, char* buf)
{
   printk_fhgfs(KERN_INFO, "Bug: Serialize function called, although it should not\n");
   os_dumpStack();
}

/**
 * Dummy function for serialize pointers
 */
unsigned _NetMessage_calcMessageLengthDummy(NetMessage* this)
{
   printk_fhgfs(KERN_INFO, "Bug: calcMessageLength function called, but should not\n");
   os_dumpStack();

   return 0;
}
