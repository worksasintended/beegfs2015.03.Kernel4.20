#ifndef GETNODESRESPMSG_H_
#define GETNODESRESPMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/nodes/NodeList.h>


struct GetNodesRespMsg;
typedef struct GetNodesRespMsg GetNodesRespMsg;

static inline void GetNodesRespMsg_init(GetNodesRespMsg* this);
static inline void GetNodesRespMsg_initFromNodeData(GetNodesRespMsg* this,
   uint16_t rootNumID, NodeList* nideList);
static inline GetNodesRespMsg* GetNodesRespMsg_construct(void);
static inline GetNodesRespMsg* GetNodesRespMsg_constructFromNodeData(
   uint16_t rootNumID, NodeList* nodeList);
static inline void GetNodesRespMsg_uninit(NetMessage* this);
static inline void GetNodesRespMsg_destruct(NetMessage* this);

// virtual functions
extern void GetNodesRespMsg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool GetNodesRespMsg_deserializePayload(NetMessage* this,
   const char* buf, size_t bufLen);
extern unsigned GetNodesRespMsg_calcMessageLength(NetMessage* this);

// inliners
static inline void GetNodesRespMsg_parseNodeList(App* app, GetNodesRespMsg* this,
   NodeList* outNodeList);

// getters & setters
static inline uint16_t GetNodesRespMsg_getRootNumID(GetNodesRespMsg* this);


struct GetNodesRespMsg
{
   NetMessage netMessage;

   uint16_t rootNumID;

   // for serialization
   NodeList* nodeList; // not owned by this object!

   // for deserialization
   unsigned nodeListElemNum;
   const char* nodeListStart;
};


void GetNodesRespMsg_init(GetNodesRespMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_GetNodesResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = GetNodesRespMsg_uninit;

   ( (NetMessage*)this)->serializePayload = GetNodesRespMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = GetNodesRespMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = GetNodesRespMsg_calcMessageLength;
}

/**
 * @param rootID just a reference, so do not free it as long as you use this object!
 * @param nodeList just a reference, so do not free it as long as you use this object!
 */
void GetNodesRespMsg_initFromNodeData(GetNodesRespMsg* this,
   uint16_t rootNumID, NodeList* nodeList)
{
   GetNodesRespMsg_init(this);

   this->rootNumID = rootNumID;

   this->nodeList = nodeList;
}

GetNodesRespMsg* GetNodesRespMsg_construct(void)
{
   struct GetNodesRespMsg* this = os_kmalloc(sizeof(*this) );

   GetNodesRespMsg_init(this);

   return this;
}

/**
 * @param nodeID just a reference, so do not free it as long as you use this object!
 * @param nicList just a reference, so do not free it as long as you use this object!
 */
GetNodesRespMsg* GetNodesRespMsg_constructFromNodeData(
   uint16_t rootNumID, NodeList* nodeList)
{
   struct GetNodesRespMsg* this = os_kmalloc(sizeof(*this) );

   GetNodesRespMsg_initFromNodeData(this, rootNumID, nodeList);

   return this;
}

void GetNodesRespMsg_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void GetNodesRespMsg_destruct(NetMessage* this)
{
   GetNodesRespMsg_uninit(this);

   os_kfree(this);
}

void GetNodesRespMsg_parseNodeList(App* app, GetNodesRespMsg* this, NodeList* outNodeList)
{
   Serialization_deserializeNodeList(app, this->nodeListElemNum, this->nodeListStart, outNodeList);
}

uint16_t GetNodesRespMsg_getRootNumID(GetNodesRespMsg* this)
{
   return this->rootNumID;
}


#endif /* GETNODESRESPMSG_H_ */
