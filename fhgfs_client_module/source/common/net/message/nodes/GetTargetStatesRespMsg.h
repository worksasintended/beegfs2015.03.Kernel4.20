#ifndef GETTARGETSTATESRESPMSG_H
#define GETTARGETSTATESRESPMSG_H

#include <common/net/message/NetMessage.h>
#include <common/Common.h>

/**
 * Note: This message can only be received/deserialized (send/serialization not implemented)
 */

struct GetTargetStatesRespMsg;
typedef struct GetTargetStatesRespMsg GetTargetStatesRespMsg;

static inline void GetTargetStatesRespMsg_init(GetTargetStatesRespMsg* this);
static inline GetTargetStatesRespMsg* GetTargetStatesRespMsg_construct(void);
static inline void GetTargetStatesRespMsg_uninit(NetMessage* this);
static inline void GetTargetStatesRespMsg_destruct(NetMessage* this);

// virtual functions
extern void GetTargetStatesRespMsg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool GetTargetStatesRespMsg_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen);
extern unsigned GetTargetStatesRespMsg_calcMessageLength(NetMessage* this);

// inliners
static inline void GetTargetStatesRespMsg_parseTargetIDs(GetTargetStatesRespMsg* this,
   UInt16List* outTargetIDs);
static inline void GetTargetStatesRespMsg_parseReachabilityStates(
   GetTargetStatesRespMsg* this, UInt8List* outReachabilityStates);
static inline void GetTargetStatesRespMsg_parseConsistencyStates(
   GetTargetStatesRespMsg* this, UInt8List* outConsistencyStates);

struct GetTargetStatesRespMsg
{
   NetMessage netMessage;

   /*
    * NOTE: UInt8List is used here for target states; must be consistent with userspace common lib
    */

   // for serialization
   UInt16List* targetIDs; // not owned by this object!
   UInt8List* reachabilityStates; // not owned by this object!
   UInt8List* consistencyStates; // not owned by this object!

   // for deserialization
   unsigned targetIDsElemNum;
   const char* targetIDsListStart;
   unsigned targetIDsBufLen;

   unsigned reachabilityStatesElemNum;
   const char* reachabilityStatesListStart;
   unsigned reachabilityStatesBufLen;

   unsigned consistencyStatesElemNum;
   const char* consistencyStatesListStart;
   unsigned consistencyStatesBufLen;
};


void GetTargetStatesRespMsg_init(GetTargetStatesRespMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_GetTargetStatesResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = GetTargetStatesRespMsg_uninit;

   ( (NetMessage*)this)->serializePayload = GetTargetStatesRespMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = GetTargetStatesRespMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = GetTargetStatesRespMsg_calcMessageLength;
}

GetTargetStatesRespMsg* GetTargetStatesRespMsg_construct(void)
{
   struct GetTargetStatesRespMsg* this = os_kmalloc(sizeof(*this) );

   GetTargetStatesRespMsg_init(this);

   return this;
}

void GetTargetStatesRespMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void GetTargetStatesRespMsg_destruct(NetMessage* this)
{
   GetTargetStatesRespMsg_uninit( (NetMessage*)this);

   os_kfree(this);
}

void GetTargetStatesRespMsg_parseTargetIDs(GetTargetStatesRespMsg* this, UInt16List* outTargetIDs)
{
   Serialization_deserializeUInt16List(this->targetIDsBufLen, this->targetIDsElemNum,
      this->targetIDsListStart, outTargetIDs);
}

void GetTargetStatesRespMsg_parseReachabilityStates(GetTargetStatesRespMsg* this,
   UInt8List* outReachabilityStates)
{
   Serialization_deserializeUInt8List(this->reachabilityStatesBufLen,
      this->reachabilityStatesElemNum, this->reachabilityStatesListStart, outReachabilityStates);
}

void GetTargetStatesRespMsg_parseConsistencyStates(GetTargetStatesRespMsg* this,
   UInt8List* outConsistencyStates)
{
   Serialization_deserializeUInt8List(this->consistencyStatesBufLen, this->consistencyStatesElemNum,
      this->consistencyStatesListStart, outConsistencyStates);
}

#endif /* GETTARGETSTATESRESPMSG_H */
