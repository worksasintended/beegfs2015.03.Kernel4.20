#ifndef GETSTATESANDBUDDYGROUPSRESPMSG_H_
#define GETSTATESANDBUDDYGROUPSRESPMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/Common.h>


struct GetStatesAndBuddyGroupsRespMsg;
typedef struct GetStatesAndBuddyGroupsRespMsg GetStatesAndBuddyGroupsRespMsg;

static inline void GetStatesAndBuddyGroupsRespMsg_init(GetStatesAndBuddyGroupsRespMsg* this);
static inline GetStatesAndBuddyGroupsRespMsg* GetStatesAndBuddyGroupsRespMsg_construct(void);
static inline void GetStatesAndBuddyGroupsRespMsg_uninit(NetMessage* this);
static inline void GetStatesAndBuddyGroupsRespMsg_destruct(NetMessage* this);

// virtual functions
extern fhgfs_bool GetStatesAndBuddyGroupsRespMsg_deserializePayload(NetMessage* this,
   const char* buf, size_t bufLen);

// inliners
static inline void GetStatesAndBuddyGroupsRespMsg_parseBuddyGroupIDs(
   GetStatesAndBuddyGroupsRespMsg* this, UInt16List* outGroupIDs);
static inline void GetStatesAndBuddyGroupsRespMsg_parsePrimaryTargetIDs(
   GetStatesAndBuddyGroupsRespMsg* this, UInt16List* outPrimaryTargetIDs);
static inline void GetStatesAndBuddyGroupsRespMsg_parseSecondaryTargetIDs(
   GetStatesAndBuddyGroupsRespMsg* this, UInt16List* outSecondaryTargetIDs);
static inline void GetStatesAndBuddyGroupsRespMsg_parseTargetIDs(
   GetStatesAndBuddyGroupsRespMsg* this, UInt16List* outTargetIDs);
static inline void GetStatesAndBuddyGroupsRespMsg_parseReachabilityStates(
   GetStatesAndBuddyGroupsRespMsg* this, UInt8List* outReachabilityStates);
static inline void GetStatesAndBuddyGroupsRespMsg_parseConsistencyStates(
   GetStatesAndBuddyGroupsRespMsg* this, UInt8List* outConsistencyStates);

/**
 * This message carries two maps:
 *    1) buddyGroupID -> primaryTarget, secondaryTarget
 *    2) targetID -> targetReachabilityState, targetConsistencyState
 *
 * Note: This message can only be received/deserialized (send/serialization not implemented).
 */
struct GetStatesAndBuddyGroupsRespMsg
{
   NetMessage netMessage;

   // for deserialization

   unsigned buddyGroupIDsElemNum;
   const char* buddyGroupIDsListStart;
   unsigned buddyGroupIDsBufLen;

   unsigned primaryTargetIDsElemNum;
   const char* primaryTargetIDsListStart;
   unsigned primaryTargetIDsBufLen;

   unsigned secondaryTargetIDsElemNum;
   const char* secondaryTargetIDsListStart;
   unsigned secondaryTargetIDsBufLen;

   unsigned targetIDsElemNum;
   const char* targetIDsListStart;
   unsigned targetIDsBufLen;

   unsigned targetReachabilityStatesElemNum;
   const char* targetReachabilityStatesListStart;
   unsigned targetReachabilityStatesBufLen;

   unsigned targetConsistencyStatesElemNum;
   const char* targetConsistencyStatesListStart;
   unsigned targetConsistencyStatesBufLen;
};

void GetStatesAndBuddyGroupsRespMsg_init(GetStatesAndBuddyGroupsRespMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_GetStatesAndBuddyGroupsResp);

   // Assign virtual functions.
   ( (NetMessage*)this)->uninit = GetStatesAndBuddyGroupsRespMsg_uninit;

   ( (NetMessage*)this)->serializePayload = _NetMessage_serializeDummy;
   ( (NetMessage*)this)->deserializePayload = GetStatesAndBuddyGroupsRespMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = _NetMessage_calcMessageLengthDummy;
}

GetStatesAndBuddyGroupsRespMsg* GetStatesAndBuddyGroupsRespMsg_construct(void)
{
   struct GetStatesAndBuddyGroupsRespMsg* this = os_kmalloc(sizeof(*this) );

   if(likely(this) )
      GetStatesAndBuddyGroupsRespMsg_init(this);

   return this;
}

void GetStatesAndBuddyGroupsRespMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void GetStatesAndBuddyGroupsRespMsg_destruct(NetMessage* this)
{
   GetStatesAndBuddyGroupsRespMsg_uninit( (NetMessage*)this);

   os_kfree(this);
}

void GetStatesAndBuddyGroupsRespMsg_parseBuddyGroupIDs(GetStatesAndBuddyGroupsRespMsg* this,
   UInt16List* outGroupIDs)
{
   Serialization_deserializeUInt16List( this->buddyGroupIDsBufLen, this->buddyGroupIDsElemNum,
      this->buddyGroupIDsListStart, outGroupIDs);
}

void GetStatesAndBuddyGroupsRespMsg_parsePrimaryTargetIDs(GetStatesAndBuddyGroupsRespMsg* this,
   UInt16List* outPrimaryTargetIDs)
{
   Serialization_deserializeUInt16List(this->primaryTargetIDsBufLen, this->primaryTargetIDsElemNum,
      this->primaryTargetIDsListStart, outPrimaryTargetIDs);
}

void GetStatesAndBuddyGroupsRespMsg_parseSecondaryTargetIDs(GetStatesAndBuddyGroupsRespMsg* this,
   UInt16List* outSecondaryTargetIDs)
{
   Serialization_deserializeUInt16List( this->secondaryTargetIDsBufLen,
      this->secondaryTargetIDsElemNum, this->secondaryTargetIDsListStart, outSecondaryTargetIDs);
}

void GetStatesAndBuddyGroupsRespMsg_parseTargetIDs(GetStatesAndBuddyGroupsRespMsg* this,
   UInt16List* outTargetIDs)
{
   Serialization_deserializeUInt16List(this->targetIDsBufLen, this->targetIDsElemNum,
      this->targetIDsListStart, outTargetIDs);
}

void GetStatesAndBuddyGroupsRespMsg_parseReachabilityStates(GetStatesAndBuddyGroupsRespMsg* this,
   UInt8List* outReachabilityStates)
{
   Serialization_deserializeUInt8List(this->targetReachabilityStatesBufLen,
      this->targetReachabilityStatesElemNum, this->targetReachabilityStatesListStart,
      outReachabilityStates);
}

void GetStatesAndBuddyGroupsRespMsg_parseConsistencyStates(GetStatesAndBuddyGroupsRespMsg* this,
   UInt8List* outConsistencyStates)
{
   Serialization_deserializeUInt8List(this->targetConsistencyStatesBufLen,
      this->targetConsistencyStatesElemNum, this->targetConsistencyStatesListStart,
      outConsistencyStates);
}

#endif /* GETSTATESANDBUDDYGROUPSRESPMSG_H_ */
