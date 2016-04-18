#ifndef GETTARGETMAPPINGSRESPMSG_H_
#define GETTARGETMAPPINGSRESPMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/Common.h>

/**
 * Note: This message can only be received/deserialized (send/serialization not implemented)
 */


struct GetTargetMappingsRespMsg;
typedef struct GetTargetMappingsRespMsg GetTargetMappingsRespMsg;

static inline void GetTargetMappingsRespMsg_init(GetTargetMappingsRespMsg* this);
static inline GetTargetMappingsRespMsg* GetTargetMappingsRespMsg_construct(void);
static inline void GetTargetMappingsRespMsg_uninit(NetMessage* this);
static inline void GetTargetMappingsRespMsg_destruct(NetMessage* this);

// virtual functions
extern void GetTargetMappingsRespMsg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool GetTargetMappingsRespMsg_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen);
extern unsigned GetTargetMappingsRespMsg_calcMessageLength(NetMessage* this);

// inliners
static inline void GetTargetMappingsRespMsg_parseTargetIDs(GetTargetMappingsRespMsg* this,
   UInt16List* outTargetIDs);
static inline void GetTargetMappingsRespMsg_parseNodeIDs(GetTargetMappingsRespMsg* this,
   UInt16List* outNodeIDs);


struct GetTargetMappingsRespMsg
{
   NetMessage netMessage;

   // for serialization
   UInt16List* targetIDs; // not owned by this object!
   UInt16List* nodeIDs; // not owned by this object!

   // for deserialization
   unsigned targetIDsElemNum;
   const char* targetIDsListStart;
   unsigned targetIDsBufLen;
   unsigned nodeIDsElemNum;
   const char* nodeIDsListStart;
   unsigned nodeIDsBufLen;
};


void GetTargetMappingsRespMsg_init(GetTargetMappingsRespMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_GetTargetMappingsResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = GetTargetMappingsRespMsg_uninit;

   ( (NetMessage*)this)->serializePayload = GetTargetMappingsRespMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = GetTargetMappingsRespMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = GetTargetMappingsRespMsg_calcMessageLength;
}

GetTargetMappingsRespMsg* GetTargetMappingsRespMsg_construct(void)
{
   struct GetTargetMappingsRespMsg* this = os_kmalloc(sizeof(*this) );

   GetTargetMappingsRespMsg_init(this);

   return this;
}

void GetTargetMappingsRespMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void GetTargetMappingsRespMsg_destruct(NetMessage* this)
{
   GetTargetMappingsRespMsg_uninit( (NetMessage*)this);

   os_kfree(this);
}


void GetTargetMappingsRespMsg_parseTargetIDs(GetTargetMappingsRespMsg* this,
   UInt16List* outTargetIDs)
{
   Serialization_deserializeUInt16List(
      this->targetIDsBufLen, this->targetIDsElemNum, this->targetIDsListStart, outTargetIDs);
}

void GetTargetMappingsRespMsg_parseNodeIDs(GetTargetMappingsRespMsg* this,
   UInt16List* outNodeIDs)
{
   Serialization_deserializeUInt16List(
      this->nodeIDsBufLen, this->nodeIDsElemNum, this->nodeIDsListStart, outNodeIDs);
}


#endif /* GETTARGETMAPPINGSRESPMSG_H_ */
