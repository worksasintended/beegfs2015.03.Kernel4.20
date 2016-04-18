#ifndef MAPTARGETSMSGEX_H_
#define MAPTARGETSMSGEX_H_

#include <common/net/message/NetMessage.h>
#include <common/Common.h>

/**
 * Note: Only the receive/deserialize part of this message is implemented (so it cannot be sent).
 * Note: Processing only sends response when ackID is set (no MapTargetsRespMsg implemented).
 */

struct MapTargetsMsgEx;
typedef struct MapTargetsMsgEx MapTargetsMsgEx;

static inline void MapTargetsMsgEx_init(MapTargetsMsgEx* this);
static inline MapTargetsMsgEx* MapTargetsMsgEx_construct(void);
static inline void MapTargetsMsgEx_uninit(NetMessage* this);
static inline void MapTargetsMsgEx_destruct(NetMessage* this);

// virtual functions
extern fhgfs_bool MapTargetsMsgEx_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen);
extern unsigned MapTargetsMsgEx_calcMessageLength(NetMessage* this);
extern fhgfs_bool __MapTargetsMsgEx_processIncoming(NetMessage* this, struct App* app,
   fhgfs_sockaddr_in* fromAddr, struct Socket* sock, char* respBuf, size_t bufLen);

// inliners
static inline void MapTargetsMsgEx_parseTargetIDs(MapTargetsMsgEx* this,
   UInt16List* outTargetIDs);

// getters & setters
static inline uint16_t MapTargetsMsgEx_getNodeID(MapTargetsMsgEx* this);
static inline const char* MapTargetsMsgEx_getAckID(MapTargetsMsgEx* this);


struct MapTargetsMsgEx
{
   NetMessage netMessage;

   uint16_t nodeID;
   unsigned ackIDLen;
   const char* ackID;

   // for serialization
   UInt16List* targetIDs;

   // for deserialization
   unsigned targetIDsElemNum;
   const char* targetIDsListStart;
   unsigned targetIDsBufLen;
};


void MapTargetsMsgEx_init(MapTargetsMsgEx* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_MapTargets);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = MapTargetsMsgEx_uninit;

   //( (NetMessage*)this)->serializePayload = MapTargetsMsgEx_serializePayload; // not impl'ed
   ( (NetMessage*)this)->deserializePayload = MapTargetsMsgEx_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = MapTargetsMsgEx_calcMessageLength;

   ( (NetMessage*)this)->processIncoming = __MapTargetsMsgEx_processIncoming;
}


MapTargetsMsgEx* MapTargetsMsgEx_construct(void)
{
   struct MapTargetsMsgEx* this = os_kmalloc(sizeof(*this) );

   MapTargetsMsgEx_init(this);

   return this;
}

void MapTargetsMsgEx_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void MapTargetsMsgEx_destruct(NetMessage* this)
{
   MapTargetsMsgEx_uninit(this);

   os_kfree(this);
}

void MapTargetsMsgEx_parseTargetIDs(MapTargetsMsgEx* this,
   UInt16List* outTargetIDs)
{
   Serialization_deserializeUInt16List(
      this->targetIDsBufLen, this->targetIDsElemNum, this->targetIDsListStart, outTargetIDs);
}

uint16_t MapTargetsMsgEx_getNodeID(MapTargetsMsgEx* this)
{
   return this->nodeID;
}

const char* MapTargetsMsgEx_getAckID(MapTargetsMsgEx* this)
{
   return this->ackID;
}



#endif /* MAPTARGETSMSGEX_H_ */
