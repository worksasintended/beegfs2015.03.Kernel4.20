#ifndef SETMIRRORBUDDYGROUPMSGEX_H_
#define SETMIRRORBUDDYGROUPMSGEX_H_

#include <common/net/message/NetMessage.h>


struct SetMirrorBuddyGroupMsgEx;
typedef struct SetMirrorBuddyGroupMsgEx SetMirrorBuddyGroupMsgEx;

static inline void SetMirrorBuddyGroupMsgEx_init(SetMirrorBuddyGroupMsgEx* this);
static inline SetMirrorBuddyGroupMsgEx* SetMirrorBuddyGroupMsgEx_construct(void);
static inline void SetMirrorBuddyGroupMsgEx_uninit(NetMessage* this);
static inline void SetMirrorBuddyGroupMsgEx_destruct(NetMessage* this);

// virtual functions
extern fhgfs_bool SetMirrorBuddyGroupMsgEx_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen);
extern unsigned SetMirrorBuddyGroupMsgEx_calcMessageLength(NetMessage* this);
extern fhgfs_bool __SetMirrorBuddyGroupMsgEx_processIncoming(NetMessage* this, struct App* app,
   fhgfs_sockaddr_in* fromAddr, struct Socket* sock, char* respBuf, size_t bufLen);

// getters and setters
static inline uint16_t SetMirrorBuddyGroupMsgEx_getPrimaryTargetID(SetMirrorBuddyGroupMsgEx* this);
static inline uint16_t SetMirrorBuddyGroupMsgEx_getSecondaryTargetID(SetMirrorBuddyGroupMsgEx* this);
static inline uint16_t SetMirrorBuddyGroupMsgEx_getBuddyGroupID(SetMirrorBuddyGroupMsgEx* this);
static inline bool SetMirrorBuddyGroupMsgEx_getAllowUpdate(SetMirrorBuddyGroupMsgEx* this);
static inline const char* SetMirrorBuddyGroupMsgEx_getAckID(SetMirrorBuddyGroupMsgEx* this);

struct SetMirrorBuddyGroupMsgEx
{
   NetMessage netMessage;

   uint16_t primaryTargetID;
   uint16_t secondaryTargetID;
   uint16_t buddyGroupID;
   fhgfs_bool allowUpdate;

   unsigned ackIDLen;
   const char* ackID;
};

void SetMirrorBuddyGroupMsgEx_init(SetMirrorBuddyGroupMsgEx* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_SetMirrorBuddyGroup);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = SetMirrorBuddyGroupMsgEx_uninit;

   // ( (NetMessage*)this)->serializePayload - not implemented, msg is only received in client.
   ( (NetMessage*)this)->deserializePayload = SetMirrorBuddyGroupMsgEx_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = SetMirrorBuddyGroupMsgEx_calcMessageLength;

   ( (NetMessage*)this)->processIncoming = __SetMirrorBuddyGroupMsgEx_processIncoming;
}

SetMirrorBuddyGroupMsgEx* SetMirrorBuddyGroupMsgEx_construct(void)
{
   struct SetMirrorBuddyGroupMsgEx* this = os_kmalloc(sizeof(*this) );

   if(likely(this) )
      SetMirrorBuddyGroupMsgEx_init(this);

   return this;
}

void SetMirrorBuddyGroupMsgEx_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void SetMirrorBuddyGroupMsgEx_destruct(NetMessage* this)
{
   SetMirrorBuddyGroupMsgEx_uninit(this);

   os_kfree(this);
}



static inline uint16_t SetMirrorBuddyGroupMsgEx_getPrimaryTargetID(SetMirrorBuddyGroupMsgEx* this)
{
   return this->primaryTargetID;
}

static inline uint16_t SetMirrorBuddyGroupMsgEx_getSecondaryTargetID(SetMirrorBuddyGroupMsgEx* this)
{
   return this->secondaryTargetID;
}

static inline uint16_t SetMirrorBuddyGroupMsgEx_getBuddyGroupID(SetMirrorBuddyGroupMsgEx* this)
{
   return this->buddyGroupID;
}

static inline bool SetMirrorBuddyGroupMsgEx_getAllowUpdate(SetMirrorBuddyGroupMsgEx* this)
{
   return this->allowUpdate;
}

const char* SetMirrorBuddyGroupMsgEx_getAckID(SetMirrorBuddyGroupMsgEx* this)
{
   return this->ackID;
}

#endif /* SETMIRRORBUDDYGROUPMSGEX_H_ */
