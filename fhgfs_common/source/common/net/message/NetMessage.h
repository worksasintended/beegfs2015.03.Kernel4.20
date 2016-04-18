#ifndef NETMESSAGE_H_
#define NETMESSAGE_H_

#include <common/net/sock/NetworkInterfaceCard.h>
#include <common/net/sock/Socket.h>
#include <common/toolkit/HighResolutionStats.h>
#include <common/toolkit/serialization/Serialization.h>
#include <common/Common.h>
#include "NetMessageLogHelper.h"
#include "NetMessageTypes.h"


// common message constants
// ========================
#define NETMSG_PREFIX_STR        "fhgfs03" // must be exactly(!!) 8 bytes long
#define NETMSG_PREFIX_STR_LEN    8
#define NETMSG_MIN_LENGTH        NETMSG_HEADER_LENGTH
#define NETMSG_HEADER_LENGTH     32 /* length of the header (see struct NetMessageHeader) */
#define NETMSG_MAX_MSG_SIZE      65536    // 64kB
#define NETMSG_MAX_PAYLOAD_SIZE  ((unsigned)(NETMSG_MAX_MSG_SIZE - NETMSG_HEADER_LENGTH))

#define NETMSG_DEFAULT_USERID    (~0) // non-zero to avoid mixing up with root userID


struct NetMessageHeader
{
   unsigned       msgLength; // in bytes
   unsigned       msgFeatureFlags; // feature flags for derived messages (depend on msgType)
// char*          msgPrefix; // NETMSG_PREFIX_STR (8 bytes)
   unsigned short msgType; // the type of payload, defined as NETMSGTYPE_x
   uint8_t        msgCompatFeatureFlags; /* for derived messages, similar to msgFeatureFlags, but
                                            "compat" because there is no check whether receiver
                                            understands these flags, so they might be ignored. */
// unsigned char  msgPaddingChar1; // padding (available for future use)
   unsigned       msgUserID; // system user ID for per-user msg queues, stats etc.
   uint16_t       msgTargetID; // targetID (not groupID) for per-target workers on storage server
// uint16_t       msgPaddingShort1; // padding (available for future use)
// unsigned       msgPaddingInt1; // padding (available for future use)
};

/*
 * This is used as a return code for the testingEquals method defined later
 * We allow undefined here, so we do not neccessarily need to implement the
 * method for all messages. If it is not implemented it will just return
 * UNDEFINED
 */
enum TestingEqualsRes
{
   TestingEqualsRes_FALSE = 0,
   TestingEqualsRes_TRUE = 1,
   TestingEqualsRes_UNDEFINED = 2,
};

class NetMessage
{
   friend class AbstractNetMessageFactory;
   friend class TestMsgSerializationBase;
   
   public:
      virtual ~NetMessage() {}

      static void deserializeHeader(char* buf, size_t bufLen, NetMessageHeader* outHeader);

      /**
       * Processes this incoming message.
       * 
       * Note: Some messages might be received over a datagram socket, so the response
       * must be atomic (=> only a single sendto()-call)
       * 
       * @param fromAddr must be NULL for stream sockets
       * @return false if the client should be disconnected (e.g. because it sent invalid data)
       * @throw SocketException on communication error
       */
      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats)
      {
         // Note: Has to be implemented appropriately by derived classes.
         // Empty implementation provided here for invalid messages and other messages
         // that don't require this way of processing (e.g. some response messages).

         return false;
      }

      /**
       * Resume processing of a message that requires multiple processing steps.
       * Use needsProcessingResume to control whether this will be called.
       *
       * @throw SocketException on communication error
       */
      virtual bool processIncomingResume(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats)
      {
         // Note: notes for processIncoming() also apply here.
         // Note: Be aware that the worker thread will change between processinIncoming() and
            // processIncomingResume() so the respBuf will also be a new one

         return false;
      }

      /**
       * Returns all feature flags that are supported by this message. Defaults to "none", so this
       * method needs to be overridden by derived messages that actually support header feature
       * flags.
       *
       * @return combination of all supported feature flags
       */
      virtual unsigned getSupportedHeaderFeatureFlagsMask() const
      {
         return 0;
      }


      /*
       * Test if a given message is equal to this message. Can be implemented
       * for each message and is used in unit testing framework.
       *
       * @param msg The message to be tested.
       * @return a value according to TestingEqualsRes, which indicates that the
       * message is either equal, not equal or equality is undefined (because
       * method is not implemented)
       */
      virtual TestingEqualsRes testingEquals(NetMessage* msg)
      {
         return TestingEqualsRes_UNDEFINED;
      }

   protected:
      NetMessage(unsigned short msgType)
      {
         this->msgHeader.msgLength = 0;
         this->msgHeader.msgFeatureFlags = 0;
         this->msgHeader.msgType = msgType;
         this->msgHeader.msgCompatFeatureFlags = 0;
         this->msgHeader.msgUserID = NETMSG_DEFAULT_USERID;
         this->msgHeader.msgTargetID = 0;

         this->releaseSockAfterProcessing = true;
      }
      

      virtual void serializePayload(char* buf) = 0;
      virtual bool deserializePayload(const char* buf, size_t bufLen) = 0;
      virtual unsigned calcMessageLength() = 0;
      

      // getters & setters
      void setMsgType(unsigned short msgType)
      {
         this->msgHeader.msgType = msgType;
      }


   private:
      NetMessageHeader msgHeader;

      bool releaseSockAfterProcessing; /* false if sock was already released during
                                          processIncoming(), e.g. due to early response. */


      void serializeHeader(char* buf);
      
      
   public:
      // inliners
      
      /**
       * recvBuf must be at least NETMSG_MIN_LENGTH long
       */
      static unsigned extractMsgLengthFromBuf(char* recvBuf)
      {
         unsigned msgLength;
         unsigned msgLengthBufLen;
         Serialization::deserializeUInt(recvBuf, sizeof(msgLength), &msgLength, &msgLengthBufLen);

         return msgLength;
      }

      bool serialize(char* buf, size_t bufLen)
      {
         if(unlikely(bufLen < getMsgLength() ) )
            return false;
         
         serializeHeader(buf);
         serializePayload(&buf[NETMSG_HEADER_LENGTH]);
         
         return true;
      }
      
      void invalidateMsgLength()
      {
         msgHeader.msgLength = 0;
      }
      
      /**
       * Check if the msg sender has set an incompatible feature flag.
       *
       * @return false if an incompatible feature flag was set
       */
      bool checkHeaderFeatureFlagsCompat() const
      {
         unsigned unsupportedFlags = ~getSupportedHeaderFeatureFlagsMask();
         if(unlikely(msgHeader.msgFeatureFlags & unsupportedFlags) )
            return false; // an unsupported flag was set

         return true;
      }

      // getters & setters

      unsigned short getMsgType() const
      {
         return msgHeader.msgType;
      }
      
      /**
       * Note: calling this method is expensive, so use it only in error/debug code paths.
       *
       * @return human-readable message type (intended for log messages).
       */
      std::string getMsgTypeStr() const
      {
         return NetMsgStrMapping().defineToStr(msgHeader.msgType);
      }

      unsigned getMsgLength()
      {
         if(!msgHeader.msgLength)
            msgHeader.msgLength = calcMessageLength();
            
         return msgHeader.msgLength;
      }
      
      unsigned getMsgHeaderFeatureFlags() const
      {
         return msgHeader.msgFeatureFlags;
      }

      /**
       * Note: You probably rather want to call addMsgHeaderFeatureFlag() instead of this.
       */
      void setMsgHeaderFeatureFlags(unsigned msgFeatureFlags)
      {
         this->msgHeader.msgFeatureFlags = msgFeatureFlags;
      }

      /**
       * Test flag. (For convenience and readability.)
       *
       * @return true if given flag is set.
       */
      bool isMsgHeaderFeatureFlagSet(unsigned flag) const
      {
         return (this->msgHeader.msgFeatureFlags & flag) != 0;
      }

      /**
       * Add another flag without clearing the previously set flags.
       *
       * Note: The receiver will reject this message if it doesn't know the given feature flag.
       */
      void addMsgHeaderFeatureFlag(unsigned flag)
      {
         this->msgHeader.msgFeatureFlags |= flag;
      }

      /**
       * Remove a certain flag without clearing the other previously set flags.
       */
      void unsetMsgHeaderFeatureFlag(unsigned flag)
      {
         this->msgHeader.msgFeatureFlags &= ~flag;
      }

      uint8_t getMsgHeaderCompatFeatureFlags() const
      {
         return msgHeader.msgCompatFeatureFlags;
      }

      void setMsgHeaderCompatFeatureFlags(uint8_t msgCompatFeatureFlags)
      {
         this->msgHeader.msgCompatFeatureFlags = msgCompatFeatureFlags;
      }

      /**
       * Test flag. (For convenience and readability.)
       *
       * @return true if given flag is set.
       */
      bool isMsgHeaderCompatFeatureFlagSet(uint8_t flag) const
      {
         return (this->msgHeader.msgCompatFeatureFlags & flag) != 0;
      }

      /**
       * Add another flag without clearing the previously set flags.
       *
       * Note: "compat" means these flags might not be understood and will then just be ignored by
       * the receiver (e.g. if the receiver is an older fhgfs version).
       */
      void addMsgHeaderCompatFeatureFlag(uint8_t flag)
      {
         this->msgHeader.msgCompatFeatureFlags |= flag;
      }

      unsigned getMsgHeaderUserID() const
      {
         return msgHeader.msgUserID;
      }

      void setMsgHeaderUserID(unsigned userID)
      {
         this->msgHeader.msgUserID = userID;
      }

      unsigned getMsgHeaderTargetID() const
      {
         return msgHeader.msgTargetID;
      }

      /**
       * @param targetID this has to be an actual targetID (not a groupID); (default is 0 if
       * targetID is not applicable).
       */
      void setMsgHeaderTargetID(uint16_t targetID)
      {
         this->msgHeader.msgTargetID = targetID;
      }

      bool getReleaseSockAfterProcessing() const
      {
         return releaseSockAfterProcessing;
      }

      void setReleaseSockAfterProcessing(bool releaseSockAfterProcessing)
      {
         this->releaseSockAfterProcessing = releaseSockAfterProcessing;
      }

};

#endif /*NETMESSAGE_H_*/
