#ifndef REMOVENODEMSGEX_H_
#define REMOVENODEMSGEX_H_

#include <common/net/message/NetMessage.h>


struct RemoveNodeMsgEx;
typedef struct RemoveNodeMsgEx RemoveNodeMsgEx;

static inline void RemoveNodeMsgEx_init(RemoveNodeMsgEx* this);
static inline void RemoveNodeMsgEx_initFromNodeData(RemoveNodeMsgEx* this,
   const char* nodeID, uint16_t nodeNumID, NodeType nodeType);
static inline RemoveNodeMsgEx* RemoveNodeMsgEx_construct(void);
static inline RemoveNodeMsgEx* RemoveNodeMsgEx_constructFromNodeData(
   const char* nodeID, uint16_t nodeNumID, NodeType nodeType);
static inline void RemoveNodeMsgEx_uninit(NetMessage* this);
static inline void RemoveNodeMsgEx_destruct(NetMessage* this);

// virtual functions
extern void RemoveNodeMsgEx_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool RemoveNodeMsgEx_deserializePayload(NetMessage* this,
   const char* buf, size_t bufLen);
extern unsigned RemoveNodeMsgEx_calcMessageLength(NetMessage* this);
extern fhgfs_bool __RemoveNodeMsgEx_processIncoming(NetMessage* this, struct App* app,
   fhgfs_sockaddr_in* fromAddr, struct Socket* sock, char* respBuf, size_t bufLen);

// getters & setters
static inline const char* RemoveNodeMsgEx_getNodeID(RemoveNodeMsgEx* this);
static inline uint16_t RemoveNodeMsgEx_getNodeNumID(RemoveNodeMsgEx* this);
static inline NodeType RemoveNodeMsgEx_getNodeType(RemoveNodeMsgEx* this);
static inline const char* RemoveNodeMsgEx_getAckID(RemoveNodeMsgEx* this);
static inline void RemoveNodeMsgEx_setAckID(RemoveNodeMsgEx* this, const char* ackID);


struct RemoveNodeMsgEx
{
   NetMessage netMessage;

   unsigned nodeIDLen;
   const char* nodeID;
   uint16_t nodeNumID;
   int16_t nodeType;
   unsigned ackIDLen;
   const char* ackID;
};


void RemoveNodeMsgEx_init(RemoveNodeMsgEx* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_RemoveNode);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = RemoveNodeMsgEx_uninit;

   ( (NetMessage*)this)->serializePayload = RemoveNodeMsgEx_serializePayload;
   ( (NetMessage*)this)->deserializePayload = RemoveNodeMsgEx_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = RemoveNodeMsgEx_calcMessageLength;

   ( (NetMessage*)this)->processIncoming = __RemoveNodeMsgEx_processIncoming;
}

/**
 * @param nodeID just a reference, so do not free it as long as you use this object!
 */
void RemoveNodeMsgEx_initFromNodeData(RemoveNodeMsgEx* this,
   const char* nodeID, uint16_t nodeNumID, NodeType nodeType)
{
   RemoveNodeMsgEx_init(this);

   this->nodeID = nodeID;
   this->nodeIDLen = os_strlen(nodeID);

   this->nodeNumID = nodeNumID;

   this->nodeType = (int16_t)nodeType;

   this->ackID = "";
   this->ackIDLen = 0;
}

RemoveNodeMsgEx* RemoveNodeMsgEx_construct(void)
{
   struct RemoveNodeMsgEx* this = os_kmalloc(sizeof(*this) );

   RemoveNodeMsgEx_init(this);

   return this;
}

/**
 * @param nodeID just a reference, so do not free it as long as you use this object!
 */
RemoveNodeMsgEx* RemoveNodeMsgEx_constructFromNodeData(
   const char* nodeID, uint16_t nodeNumID, NodeType nodeType)
{
   struct RemoveNodeMsgEx* this = os_kmalloc(sizeof(*this) );

   RemoveNodeMsgEx_initFromNodeData(this, nodeID, nodeNumID, nodeType);

   return this;
}

void RemoveNodeMsgEx_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void RemoveNodeMsgEx_destruct(NetMessage* this)
{
   RemoveNodeMsgEx_uninit(this);

   os_kfree(this);
}

const char* RemoveNodeMsgEx_getNodeID(RemoveNodeMsgEx* this)
{
   return this->nodeID;
}

uint16_t RemoveNodeMsgEx_getNodeNumID(RemoveNodeMsgEx* this)
{
   return this->nodeNumID;
}

NodeType RemoveNodeMsgEx_getNodeType(RemoveNodeMsgEx* this)
{
   return (NodeType)this->nodeType;
}

const char* RemoveNodeMsgEx_getAckID(RemoveNodeMsgEx* this)
{
   return this->ackID;
}

/**
 * @param ackID just a reference
 */
static inline void RemoveNodeMsgEx_setAckID(RemoveNodeMsgEx* this, const char* ackID)
{
   this->ackID = ackID;
   this->ackIDLen = os_strlen(ackID);
}

#endif /* REMOVENODEMSGEX_H_ */
