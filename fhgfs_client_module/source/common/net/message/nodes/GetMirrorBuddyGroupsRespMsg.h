#ifndef GETMIRRORBUDDYGROUPSRESPMSG_H
#define GETMIRRORBUDDYGROUPSRESPMSG_H

#include <common/net/message/NetMessage.h>
#include <common/Common.h>

/**
 * Note: This message can only be received/deserialized (send/serialization not implemented)
 */

struct GetMirrorBuddyGroupsRespMsg;
typedef struct GetMirrorBuddyGroupsRespMsg GetMirrorBuddyGroupsRespMsg;

static inline void GetMirrorBuddyGroupsRespMsg_init(GetMirrorBuddyGroupsRespMsg* this);
static inline GetMirrorBuddyGroupsRespMsg* GetMirrorBuddyGroupsRespMsg_construct(void);
static inline void GetMirrorBuddyGroupsRespMsg_uninit(NetMessage* this);
static inline void GetMirrorBuddyGroupsRespMsg_destruct(NetMessage* this);

// virtual functions
extern void GetMirrorBuddyGroupsRespMsg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool GetMirrorBuddyGroupsRespMsg_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen);
extern unsigned GetMirrorBuddyGroupsRespMsg_calcMessageLength(NetMessage* this);

// inliners
static inline void GetMirrorBuddyGroupsRespMsg_parseBuddyGroupIDs(GetMirrorBuddyGroupsRespMsg* this,
   UInt16List* outGroupIDs);
static inline void GetMirrorBuddyGroupsRespMsg_parsePrimaryTargetIDs(
   GetMirrorBuddyGroupsRespMsg* this, UInt16List* outPrimaryTargetIDs);
static inline void GetMirrorBuddyGroupsRespMsg_parseSecondaryTargetIDs(
   GetMirrorBuddyGroupsRespMsg* this, UInt16List* outSecondaryTargetIDs);

struct GetMirrorBuddyGroupsRespMsg
{
   NetMessage netMessage;

   // for serialization
   UInt16List* mirrorBuddyGroupIDs; // not owned by this object!
   UInt16List* primaryTargetIDs; // not owned by this object!
   UInt16List* secondaryTargetIDs; // not owned by this object!

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
};


void GetMirrorBuddyGroupsRespMsg_init(GetMirrorBuddyGroupsRespMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_GetMirrorBuddyGroupsResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = GetMirrorBuddyGroupsRespMsg_uninit;

   ( (NetMessage*)this)->serializePayload = GetMirrorBuddyGroupsRespMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = GetMirrorBuddyGroupsRespMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = GetMirrorBuddyGroupsRespMsg_calcMessageLength;
}

GetMirrorBuddyGroupsRespMsg* GetMirrorBuddyGroupsRespMsg_construct(void)
{
   struct GetMirrorBuddyGroupsRespMsg* this = os_kmalloc(sizeof(*this) );

   GetMirrorBuddyGroupsRespMsg_init(this);

   return this;
}

void GetMirrorBuddyGroupsRespMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void GetMirrorBuddyGroupsRespMsg_destruct(NetMessage* this)
{
   GetMirrorBuddyGroupsRespMsg_uninit( (NetMessage*)this);

   os_kfree(this);
}

void GetMirrorBuddyGroupsRespMsg_parseBuddyGroupIDs(GetMirrorBuddyGroupsRespMsg* this,
   UInt16List* outGroupIDs)
{
   Serialization_deserializeUInt16List( this->buddyGroupIDsBufLen, this->buddyGroupIDsElemNum,
      this->buddyGroupIDsListStart, outGroupIDs);
}

void GetMirrorBuddyGroupsRespMsg_parsePrimaryTargetIDs(GetMirrorBuddyGroupsRespMsg* this,
   UInt16List* outPrimaryTargetIDs)
{
   Serialization_deserializeUInt16List(this->primaryTargetIDsBufLen, this->primaryTargetIDsElemNum,
      this->primaryTargetIDsListStart, outPrimaryTargetIDs);
}

void GetMirrorBuddyGroupsRespMsg_parseSecondaryTargetIDs(GetMirrorBuddyGroupsRespMsg* this,
   UInt16List* outSecondaryTargetIDs)
{
   Serialization_deserializeUInt16List( this->secondaryTargetIDsBufLen,
      this->secondaryTargetIDsElemNum, this->secondaryTargetIDsListStart, outSecondaryTargetIDs);
}

#endif /* GETMIRRORBUDDYGROUPSRESPMSG_H */
