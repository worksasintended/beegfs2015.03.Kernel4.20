#ifndef HEARTBEATMSG_H_
#define HEARTBEATMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/net/sock/NetworkInterfaceCard.h>
#include <toolkit/BitStore.h>


struct HeartbeatMsgEx;
typedef struct HeartbeatMsgEx HeartbeatMsgEx;

static inline void HeartbeatMsgEx_init(HeartbeatMsgEx* this);
static inline void HeartbeatMsgEx_initFromNodeData(HeartbeatMsgEx* this,
   const char* nodeID, uint16_t nodeNumID, int nodeType, NicAddressList* nicList,
   const BitStore* nodeFeatureFlags);
static inline HeartbeatMsgEx* HeartbeatMsgEx_construct(void);
static inline HeartbeatMsgEx* HeartbeatMsgEx_constructFromNodeData(
   const char* nodeID, uint16_t nodeNumID, int nodeType, NicAddressList* nicList,
   const BitStore* nodeFeatureFlags);
static inline void HeartbeatMsgEx_uninit(NetMessage* this);
static inline void HeartbeatMsgEx_destruct(NetMessage* this);

extern void __HeartbeatMsgEx_processIncomingRoot(HeartbeatMsgEx* this, struct App* app);

// virtual functions
extern void HeartbeatMsgEx_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool HeartbeatMsgEx_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen);
extern unsigned HeartbeatMsgEx_calcMessageLength(NetMessage* this);
extern fhgfs_bool __HeartbeatMsgEx_processIncoming(NetMessage* this, struct App* app,
   fhgfs_sockaddr_in* fromAddr, struct Socket* sock, char* respBuf, size_t bufLen);

// inliners
static inline void HeartbeatMsgEx_parseNicList(HeartbeatMsgEx* this,
   NicAddressList* outNicList);
static inline void HeartbeatMsgEx_parseNodeFeatureFlags(HeartbeatMsgEx* this,
   BitStore* outFeatureFlags);

// getters & setters
static inline const char* HeartbeatMsgEx_getNodeID(HeartbeatMsgEx* this);
static inline uint16_t HeartbeatMsgEx_getNodeNumID(HeartbeatMsgEx* this);
static inline int HeartbeatMsgEx_getNodeType(HeartbeatMsgEx* this);
static inline unsigned HeartbeatMsgEx_getFhgfsVersion(HeartbeatMsgEx* this);
static inline void HeartbeatMsgEx_setFhgfsVersion(HeartbeatMsgEx* this, unsigned fhgfsVersion);
static inline uint16_t HeartbeatMsgEx_getRootNumID(HeartbeatMsgEx* this);
static inline const char* HeartbeatMsgEx_getAckID(HeartbeatMsgEx* this);
static inline void HeartbeatMsgEx_setAckID(HeartbeatMsgEx* this, const char* ackID);
static inline void HeartbeatMsgEx_setPorts(HeartbeatMsgEx* this,
   uint16_t portUDP, uint16_t portTCP);
static inline uint16_t HeartbeatMsgEx_getPortUDP(HeartbeatMsgEx* this);
static inline uint16_t HeartbeatMsgEx_getPortTCP(HeartbeatMsgEx* this);


struct HeartbeatMsgEx
{
   NetMessage netMessage;

   unsigned nodeIDLen;
   const char* nodeID;
   int nodeType;
   unsigned fhgfsVersion;
   uint16_t nodeNumID;
   uint16_t rootNumID; // 0 means unknown/undefined
   uint64_t instanceVersion; // not used currently
   uint64_t nicListVersion; // not used currently
   uint16_t portUDP; // 0 means "undefined"
   uint16_t portTCP; // 0 means "undefined"
   unsigned ackIDLen;
   const char* ackID;

   // for serialization
   const BitStore* nodeFeatureFlags; // not owned by this object
   NicAddressList* nicList; // not owned by this object

   // for deserialization
   const char* nodeFeatureFlagsStart; // points to location in receive buffer (from preprocess)
   unsigned nicListElemNum; // NETMSG_NICLISTELEM_SIZE defines the element size
   const char* nicListStart; // see NETMSG_NICLISTELEM_SIZE for element structure
};


void HeartbeatMsgEx_init(HeartbeatMsgEx* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_Heartbeat);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = HeartbeatMsgEx_uninit;

   ( (NetMessage*)this)->serializePayload = HeartbeatMsgEx_serializePayload;
   ( (NetMessage*)this)->deserializePayload = HeartbeatMsgEx_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = HeartbeatMsgEx_calcMessageLength;

   ( (NetMessage*)this)->processIncoming = __HeartbeatMsgEx_processIncoming;
}

/**
 * @param nodeID just a reference, so do not free it as long as you use this object
 * @param nicList just a reference, so do not free it as long as you use this object
 * @param nodeFeatureFlags just a reference, so do not free it as long as you use this object
 */
void HeartbeatMsgEx_initFromNodeData(HeartbeatMsgEx* this,
   const char* nodeID, uint16_t nodeNumID, int nodeType, NicAddressList* nicList,
   const BitStore* nodeFeatureFlags)
{
   HeartbeatMsgEx_init(this);

   this->nodeID = nodeID;
   this->nodeIDLen = os_strlen(nodeID);
   this->nodeNumID = nodeNumID;
   
   this->nodeType = nodeType;
   this->fhgfsVersion = 0;

   this->rootNumID = 0; // 0 means undefined/unknown

   this->instanceVersion = 0; // reserverd for future use

   this->nodeFeatureFlags = nodeFeatureFlags;

   this->nicListVersion = 0; // reserverd for future use
   this->nicList = nicList;

   this->portUDP = 0; // 0 means "undefined"
   this->portTCP = 0; // 0 means "undefined"

   this->ackID = "";
   this->ackIDLen = 0;
   
}

HeartbeatMsgEx* HeartbeatMsgEx_construct(void)
{
   struct HeartbeatMsgEx* this = os_kmalloc(sizeof(*this) );

   HeartbeatMsgEx_init(this);

   return this;
}

/**
 * @param nodeID just a reference, so do not free it as long as you use this object
 * @param nicList just a reference, so do not free it as long as you use this object
 * @param nodeFeatureFlags just a reference, so do not free it as long as you use this object
 */
HeartbeatMsgEx* HeartbeatMsgEx_constructFromNodeData(
   const char* nodeID, uint16_t nodeNumID, int nodeType, NicAddressList* nicList,
   const BitStore* nodeFeatureFlags)
{
   struct HeartbeatMsgEx* this = os_kmalloc(sizeof(*this) );

   HeartbeatMsgEx_initFromNodeData(this, nodeID, nodeNumID, nodeType, nicList, nodeFeatureFlags);

   return this;
}

void HeartbeatMsgEx_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void HeartbeatMsgEx_destruct(NetMessage* this)
{
   HeartbeatMsgEx_uninit(this);

   os_kfree(this);
}

void HeartbeatMsgEx_parseNicList(HeartbeatMsgEx* this, NicAddressList* outNicList)
{
   Serialization_deserializeNicList(this->nicListElemNum, this->nicListStart, outNicList);
}

/**
 * @param outFeatureFlags initizalized (empty) BitStore
 */
void HeartbeatMsgEx_parseNodeFeatureFlags(HeartbeatMsgEx* this, BitStore* outFeatureFlags)
{
   unsigned deserLen;

   BitStore_deserialize(outFeatureFlags, this->nodeFeatureFlagsStart, &deserLen);
}

const char* HeartbeatMsgEx_getNodeID(HeartbeatMsgEx* this)
{
   return this->nodeID;
}

uint16_t HeartbeatMsgEx_getNodeNumID(HeartbeatMsgEx* this)
{
   return this->nodeNumID;
}

int HeartbeatMsgEx_getNodeType(HeartbeatMsgEx* this)
{
   return this->nodeType;
}

unsigned HeartbeatMsgEx_getFhgfsVersion(HeartbeatMsgEx* this)
{
   return this->fhgfsVersion;
}

void HeartbeatMsgEx_setFhgfsVersion(HeartbeatMsgEx* this, unsigned fhgfsVersion)
{
   this->fhgfsVersion = fhgfsVersion;
}

uint16_t HeartbeatMsgEx_getRootNumID(HeartbeatMsgEx* this)
{
   return this->rootNumID;
}

const char* HeartbeatMsgEx_getAckID(HeartbeatMsgEx* this)
{
   return this->ackID;
}

/**
 * @param ackID just a reference
 */
void HeartbeatMsgEx_setAckID(HeartbeatMsgEx* this, const char* ackID)
{
   this->ackID = ackID;
   this->ackIDLen = os_strlen(ackID);
}

void HeartbeatMsgEx_setPorts(HeartbeatMsgEx* this, uint16_t portUDP, uint16_t portTCP)
{
   this->portUDP = portUDP;
   this->portTCP = portTCP;
}

uint16_t HeartbeatMsgEx_getPortUDP(HeartbeatMsgEx* this)
{
   return this->portUDP;
}

uint16_t HeartbeatMsgEx_getPortTCP(HeartbeatMsgEx* this)
{
   return this->portTCP;
}


#endif /*HEARTBEATMSG_H_*/
