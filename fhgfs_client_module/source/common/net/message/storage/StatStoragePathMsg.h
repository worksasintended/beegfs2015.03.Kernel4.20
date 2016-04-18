#ifndef STATSTORAGEPATHMSG_H_
#define STATSTORAGEPATHMSG_H_

#include "../SimpleUInt16Msg.h"


struct StatStoragePathMsg;
typedef struct StatStoragePathMsg StatStoragePathMsg;

static inline void StatStoragePathMsg_init(StatStoragePathMsg* this);
static inline void StatStoragePathMsg_initFromTarget(StatStoragePathMsg* this,
   uint16_t targetID);
static inline StatStoragePathMsg* StatStoragePathMsg_construct(void);
static inline StatStoragePathMsg* StatStoragePathMsg_constructFromTarget(uint16_t targetID);
static inline void StatStoragePathMsg_uninit(NetMessage* this);
static inline void StatStoragePathMsg_destruct(NetMessage* this);

// getters & setters
static inline uint16_t StatStoragePathMsg_getTargetID(StatStoragePathMsg* this);


struct StatStoragePathMsg
{
   SimpleUInt16Msg simpleUInt16Msg;
};


void StatStoragePathMsg_init(StatStoragePathMsg* this)
{
   SimpleUInt16Msg_init( (SimpleUInt16Msg*)this, NETMSGTYPE_StatStoragePath);

   // virtual functions
   ( (NetMessage*)this)->uninit = StatStoragePathMsg_uninit;
}

/**
 * @param targetID only used for storage servers, ignored for other nodes (but may not be NULL!)
 */
void StatStoragePathMsg_initFromTarget(StatStoragePathMsg* this, uint16_t targetID)
{
   SimpleUInt16Msg_initFromValue( (SimpleUInt16Msg*)this, NETMSGTYPE_StatStoragePath, targetID);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = StatStoragePathMsg_uninit;
}

StatStoragePathMsg* StatStoragePathMsg_construct(void)
{
   struct StatStoragePathMsg* this = os_kmalloc(sizeof(*this) );
   
   StatStoragePathMsg_init(this);
   
   return this;
}

/**
 * @param targetID only used for storage servers, ignored for other nodes (but may not be NULL!)
 */
StatStoragePathMsg* StatStoragePathMsg_constructFromTarget(uint16_t targetID)
{
   struct StatStoragePathMsg* this = os_kmalloc(sizeof(*this) );

   StatStoragePathMsg_initFromTarget(this, targetID);

   return this;
}

void StatStoragePathMsg_uninit(NetMessage* this)
{
   SimpleUInt16Msg_uninit(this);
}

void StatStoragePathMsg_destruct(NetMessage* this)
{
   StatStoragePathMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}

uint16_t StatStoragePathMsg_getTargetID(StatStoragePathMsg* this)
{
   return SimpleUInt16Msg_getValue( (SimpleUInt16Msg*)this);
}

#endif /*STATSTORAGEPATHMSG_H_*/
