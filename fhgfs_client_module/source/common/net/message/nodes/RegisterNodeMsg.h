#ifndef REGISTERNODEMSG_H_
#define REGISTERNODEMSG_H_


#include <common/net/message/NetMessage.h>
#include <common/net/sock/NetworkInterfaceCard.h>


struct RegisterNodeMsg;
typedef struct RegisterNodeMsg RegisterNodeMsg;

static inline void RegisterNodeMsg_init(RegisterNodeMsg* this);
static inline void RegisterNodeMsg_initFromNodeData(RegisterNodeMsg* this,
   const char* nodeID, uint16_t nodeNumID, int nodeType, NicAddressList* nicList,
   const BitStore* nodeFeatureFlags, uint16_t portUDP, uint16_t portTCP);
static inline RegisterNodeMsg* RegisterNodeMsg_construct(void);
static inline RegisterNodeMsg* RegisterNodeMsg_constructFromNodeData(
   const char* nodeID, uint16_t nodeNumID, int nodeType, NicAddressList* nicList,
   const BitStore* nodeFeatureFlags, uint16_t portUDP, uint16_t portTCP);
static inline void RegisterNodeMsg_uninit(NetMessage* this);
static inline void RegisterNodeMsg_destruct(NetMessage* this);

// virtual functions
extern void RegisterNodeMsg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool RegisterNodeMsg_deserializePayload(NetMessage* this,
   const char* buf, size_t bufLen);
extern unsigned RegisterNodeMsg_calcMessageLength(NetMessage* this);
extern fhgfs_bool __RegisterNodeMsg_processIncoming(NetMessage* this, struct App* app,
   fhgfs_sockaddr_in* fromAddr, struct Socket* sock, char* respBuf, size_t bufLen);



/**
 * Registers a new node with the management daemon.
 *
 * This also provides the mechanism to retrieve a numeric node ID from the management node.
 */
struct RegisterNodeMsg
{
   NetMessage netMessage;

   unsigned nodeIDLen;
   const char* nodeID;
   uint16_t nodeNumID;
   int nodeType;
   uint16_t rootNumID; // 0 means unknown/undefined
   uint64_t instanceVersion; // not used currently
   uint64_t nicListVersion; // not used currently
   uint16_t portUDP; // 0 means "undefined"
   uint16_t portTCP; // 0 means "undefined"

   // for serialization
   const BitStore* nodeFeatureFlags; // not owned by this object
   NicAddressList* nicList; // not owned by this object!

   // for deserialization
   const char* nodeFeatureFlagsStart; // points to location in receive buffer (from preprocess)
   unsigned nicListElemNum; // NETMSG_NICLISTELEM_SIZE defines the element size
   const char* nicListStart; // see NETMSG_NICLISTELEM_SIZE for element structure
};


void RegisterNodeMsg_init(RegisterNodeMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_RegisterNode);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = RegisterNodeMsg_uninit;

   ( (NetMessage*)this)->serializePayload = RegisterNodeMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = RegisterNodeMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = RegisterNodeMsg_calcMessageLength;
}

/**
 * @param nodeID just a reference, so do not free it as long as you use this object
 * @param nicList just a reference, so do not free it as long as you use this object
 * @param nodeFeatureFlags just a reference, so do not free it as long as you use this object
 */
void RegisterNodeMsg_initFromNodeData(RegisterNodeMsg* this,
   const char* nodeID, uint16_t nodeNumID, int nodeType,  NicAddressList* nicList,
   const BitStore* nodeFeatureFlags, uint16_t portUDP, uint16_t portTCP)
{
   RegisterNodeMsg_init(this);

   this->nodeID = nodeID;
   this->nodeIDLen = os_strlen(nodeID);
   this->nodeNumID = nodeNumID;

   this->nodeType = nodeType;

   this->rootNumID = 0; /// 0 means unknown/undefined

   this->instanceVersion = 0; // reserverd for future use

   this->nodeFeatureFlags = nodeFeatureFlags;

   this->nicListVersion = 0; // reserverd for future use
   this->nicList = nicList;

   this->portUDP = portUDP;
   this->portTCP = portTCP;
}

RegisterNodeMsg* RegisterNodeMsg_construct(void)
{
   struct RegisterNodeMsg* this = os_kmalloc(sizeof(*this) );

   RegisterNodeMsg_init(this);

   return this;
}

/**
 * @param nodeID just a reference, so do not free it as long as you use this object
 * @param nicList just a reference, so do not free it as long as you use this object
 * @param nodeFeatureFlags just a reference, so do not free it as long as you use this object
 */
RegisterNodeMsg* RegisterNodeMsg_constructFromNodeData(
   const char* nodeID, uint16_t nodeNumID, int nodeType, NicAddressList* nicList,
   const BitStore* nodeFeatureFlags, uint16_t portUDP, uint16_t portTCP)
{
   struct RegisterNodeMsg* this = os_kmalloc(sizeof(*this) );

   RegisterNodeMsg_initFromNodeData(this, nodeID, nodeNumID, nodeType, nicList, nodeFeatureFlags,
      portUDP, portTCP);

   return this;
}

void RegisterNodeMsg_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void RegisterNodeMsg_destruct(NetMessage* this)
{
   RegisterNodeMsg_uninit(this);

   os_kfree(this);
}




#endif /* REGISTERNODEMSG_H_ */
