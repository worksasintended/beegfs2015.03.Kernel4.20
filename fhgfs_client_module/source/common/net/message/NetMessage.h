#ifndef NETMESSAGE_H_
#define NETMESSAGE_H_

#include <common/net/sock/NetworkInterfaceCard.h>
#include <common/net/sock/Socket.h>
#include <common/toolkit/Serialization.h>
#include <common/Common.h>
#include "NetMessageTypes.h"

/**
 * Note: This "class" is not meant to be instantiated directly (consider it to be abstract).
 * It contains some "virtual" methods, as you can see in the struct NetMessage. Only the virtual
 * Method processIncoming(..) has a default implementation.
 * Derived classes have a destructor with a NetMessage-Pointer (instead of the real type)
 * because of the generic virtual destructor signature.
 * Derived classes normally have two constructors: One has no arguments and is used for
 * deserialization. The other one is the standard constructor.
 */

// common message constants
// ========================
#define NETMSG_PREFIX_STR        "fhgfs03" // must be exactly(!!) 8 bytes long
#define NETMSG_PREFIX_STR_LEN    8
#define NETMSG_MIN_LENGTH        NETMSG_HEADER_LENGTH
#define NETMSG_HEADER_LENGTH     32 /* length of the header (see struct NetMessageHeader) */
#define NETMSG_MAX_MSG_SIZE      65536    // 64kB
#define NETMSG_MAX_PAYLOAD_SIZE  ((unsigned)(NETMSG_MAX_MSG_SIZE - NETMSG_HEADER_LENGTH))

#define NETMSG_DEFAULT_USERID    (~0) // non-zero to avoid mixing up with root userID


// forward declaration
struct App;


struct NetMessageHeader;
typedef struct NetMessageHeader NetMessageHeader;
struct NetMessage;
typedef struct NetMessage NetMessage;

static inline void NetMessage_init(NetMessage* this, unsigned short msgType);
static inline NetMessage* NetMessage_construct(unsigned short msgType);
static inline void NetMessage_uninit(NetMessage* this);
static inline void NetMessage_destruct(NetMessage* this);

extern void __NetMessage_deserializeHeader(char* buf, size_t bufLen,
   struct NetMessageHeader* outHeader);
extern void __NetMessage_serializeHeader(NetMessage* this, char* buf);

extern void _NetMessage_serializeDummy(NetMessage* this, char* buf);
extern fhgfs_bool _NetMessage_deserializeDummy(NetMessage* this, const char* buf, size_t bufLen);
extern unsigned _NetMessage_calcMessageLengthDummy(NetMessage* this);


static inline unsigned NetMessage_extractMsgLengthFromBuf(char* recvBuf);
static inline fhgfs_bool NetMessage_serialize(NetMessage* this, char* buf, size_t bufLen);
static inline fhgfs_bool NetMessage_checkHeaderFeatureFlagsCompat(NetMessage* this);

static inline void NetMessage_virtualDestruct(NetMessage* this);

// virtual functions
extern fhgfs_bool NetMessage_processIncoming(NetMessage* this, struct App* app,
   fhgfs_sockaddr_in* fromAddr, struct Socket* sock, char* respBuf, size_t bufLen);
extern unsigned NetMessage_getSupportedHeaderFeatureFlagsMask(NetMessage* this);

// getters & setters
static inline unsigned short NetMessage_getMsgType(NetMessage* this);
static inline unsigned NetMessage_getMsgHeaderFeatureFlags(NetMessage* this);
static inline void NetMessage_setMsgHeaderFeatureFlags(NetMessage* this,
   unsigned msgFeatureFlags);
static inline fhgfs_bool NetMessage_isMsgHeaderFeatureFlagSet(NetMessage* this, unsigned flag);
static inline void NetMessage_addMsgHeaderFeatureFlag(NetMessage* this, unsigned flag);
static inline void NetMessage_setMsgHeaderCompatFeatureFlags(NetMessage* this,
   uint8_t msgCompatFeatureFlags);
static inline fhgfs_bool NetMessage_isMsgHeaderCompatFeatureFlagSet(NetMessage* this,
   uint8_t flag);
static inline void NetMessage_addMsgHeaderCompatFeatureFlag(NetMessage* this, uint8_t flag);
static inline unsigned NetMessage_getMsgLength(NetMessage* this);
static inline void NetMessage_setMsgHeaderUserID(NetMessage* this, unsigned userID);
static inline void NetMessage_setMsgHeaderTargetID(NetMessage* this, uint16_t userID);

static inline void _NetMessage_setMsgType(NetMessage* this, unsigned short msgType);


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
// unsigned       msgPaddingInt2; // padding (available for future use)
};


struct NetMessage
{
   struct NetMessageHeader msgHeader;

   // virtual functions
   void (*uninit) (NetMessage* this);

   void (*serializePayload) (NetMessage* this, char* buf);
   fhgfs_bool (*deserializePayload) (NetMessage* this, const char* buf, size_t bufLen);
   unsigned (*calcMessageLength) (NetMessage* this);

   fhgfs_bool (*processIncoming) (NetMessage* this, struct App* app, fhgfs_sockaddr_in* fromAddr,
      struct Socket* sock, char* respBuf, size_t bufLen);
   unsigned (*getSupportedHeaderFeatureFlagsMask) (NetMessage* this);

};


void NetMessage_init(NetMessage* this, unsigned short msgType)
{
   os_memset(this, 0, sizeof(*this) ); // clear function pointers etc.

   // this->msgLength = 0; // zero'ed by memset
   // this->msgFeatureFlags = 0; // zero'ed by memset

   this->msgHeader.msgType = msgType;

   // this->msgCompatFeatureFlags = 0; // zero'ed by memset

   // needs to be set to actual ID by some async flushers etc
   this->msgHeader.msgUserID = FhgfsCommon_getCurrentUserID();

   // this->msgTargetID = 0; // zero'ed by memset


   this->processIncoming = NetMessage_processIncoming;
   this->getSupportedHeaderFeatureFlagsMask = NetMessage_getSupportedHeaderFeatureFlagsMask;
}

NetMessage* NetMessage_construct(unsigned short msgType)
{
   struct NetMessage* this = os_kmalloc(sizeof(struct NetMessage) );

   NetMessage_init(this, msgType);

   return this;
}

void NetMessage_uninit(NetMessage* this)
{
}

void NetMessage_destruct(NetMessage* this)
{
   NetMessage_uninit(this);

   os_kfree(this);
}


/**
 * recvBuf must be at least NETMSG_MIN_LENGTH long
 */
unsigned NetMessage_extractMsgLengthFromBuf(char* recvBuf)
{
   unsigned msgLengthBufLen;
   unsigned msgLength;
   Serialization_deserializeUInt(recvBuf, sizeof(msgLength), &msgLength, &msgLengthBufLen);

   return msgLength;
}

fhgfs_bool NetMessage_serialize(NetMessage* this, char* buf, size_t bufLen)
{
   if(unlikely(bufLen < NetMessage_getMsgLength(this) ) )
      return fhgfs_false;

   __NetMessage_serializeHeader(this, buf);
   this->serializePayload(this, &buf[NETMSG_HEADER_LENGTH]);

   return fhgfs_true;
}

/**
 * Check if the msg sender has set an incompatible feature flag.
 *
 * @return false if an incompatible feature flag was set
 */
fhgfs_bool NetMessage_checkHeaderFeatureFlagsCompat(NetMessage* this)
{
   unsigned unsupportedFlags = ~(this->getSupportedHeaderFeatureFlagsMask(this) );
   if(unlikely(this->msgHeader.msgFeatureFlags & unsupportedFlags) )
      return fhgfs_false; // an unsupported flag was set

   return fhgfs_true;
}

/**
 * Calls the virtual uninit method and kfrees the object.
 */
void NetMessage_virtualDestruct(NetMessage* this)
{
   this->uninit(this);
   os_kfree(this);
}


unsigned short NetMessage_getMsgType(NetMessage* this)
{
   return this->msgHeader.msgType;
}

unsigned NetMessage_getMsgHeaderFeatureFlags(NetMessage* this)
{
   return this->msgHeader.msgFeatureFlags;
}

/**
 * Replace all currently set feature flags.
 *
 * Note: The receiver will reject this message if it doesn't know the given feature flags.
 */
void NetMessage_setMsgHeaderFeatureFlags(NetMessage* this, unsigned msgFeatureFlags)
{
   this->msgHeader.msgFeatureFlags = msgFeatureFlags;
}

/**
 * Test flag. (For convenience and readability.)
 *
 * @return true if given flag is set.
 */
fhgfs_bool NetMessage_isMsgHeaderFeatureFlagSet(NetMessage* this, unsigned flag)
{
   return (this->msgHeader.msgFeatureFlags & flag) != 0;
}

/**
 * Add another flag without clearing the previously set flags.
 *
 * Note: The receiver will reject this message if it doesn't know the given feature flag.
 */
void NetMessage_addMsgHeaderFeatureFlag(NetMessage* this, unsigned flag)
{
   this->msgHeader.msgFeatureFlags |= flag;
}

/**
 * Replace all currently set feature flags.
 *
 * Note: "compat" means these flags might not be understood and thus ignored by the receiver (e.g.
 * if the receiver is an older fhgfs version).
 */
void NetMessage_setMsgHeaderCompatFeatureFlags(NetMessage* this, uint8_t msgCompatFeatureFlags)
{
   this->msgHeader.msgCompatFeatureFlags = msgCompatFeatureFlags;
}

/**
 * Test flag. (For convenience and readability.)
 *
 * @return true if given flag is set.
 */
fhgfs_bool NetMessage_isMsgHeaderCompatFeatureFlagSet(NetMessage* this, uint8_t flag)
{
   return (this->msgHeader.msgCompatFeatureFlags & flag) != 0;
}

/**
 * Add another flag without clearing the previously set flags.
 *
 * Note: "compat" means these flags might not be understood and thus ignored by the receiver (e.g.
 * if the receiver is an older fhgfs version).
 */
void NetMessage_addMsgHeaderCompatFeatureFlag(NetMessage* this, uint8_t flag)
{
   this->msgHeader.msgCompatFeatureFlags |= flag;
}

unsigned NetMessage_getMsgLength(NetMessage* this)
{
   if(!this->msgHeader.msgLength)
      this->msgHeader.msgLength = this->calcMessageLength(this);

   return this->msgHeader.msgLength;
}

void NetMessage_setMsgHeaderUserID(NetMessage* this, unsigned userID)
{
   this->msgHeader.msgUserID = userID;
}

/**
 * @param targetID this has to be an actual targetID (not a groupID).
 */
void NetMessage_setMsgHeaderTargetID(NetMessage* this, uint16_t targetID)
{
   this->msgHeader.msgTargetID = targetID;
}

void _NetMessage_setMsgType(NetMessage* this, unsigned short msgType)
{
   this->msgHeader.msgType = msgType;
}


#endif /*NETMESSAGE_H_*/
