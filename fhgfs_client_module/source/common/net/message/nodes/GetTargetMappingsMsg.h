#ifndef GETTARGETMAPPINGSMSG_H_
#define GETTARGETMAPPINGSMSG_H_

#include "../SimpleMsg.h"


struct GetTargetMappingsMsg;
typedef struct GetTargetMappingsMsg GetTargetMappingsMsg;

static inline void GetTargetMappingsMsg_init(GetTargetMappingsMsg* this);
static inline GetTargetMappingsMsg* GetTargetMappingsMsg_construct(void);
static inline void GetTargetMappingsMsg_uninit(NetMessage* this);
static inline void GetTargetMappingsMsg_destruct(NetMessage* this);


struct GetTargetMappingsMsg
{
   SimpleMsg simpleMsg;
};


void GetTargetMappingsMsg_init(GetTargetMappingsMsg* this)
{
   SimpleMsg_init( (SimpleMsg*)this, NETMSGTYPE_GetTargetMappings);

   // virtual functions
   ( (NetMessage*)this)->uninit = GetTargetMappingsMsg_uninit;
}

GetTargetMappingsMsg* GetTargetMappingsMsg_construct(void)
{
   struct GetTargetMappingsMsg* this = os_kmalloc(sizeof(*this) );

   GetTargetMappingsMsg_init(this);

   return this;
}

void GetTargetMappingsMsg_uninit(NetMessage* this)
{
   SimpleMsg_uninit(this);
}

void GetTargetMappingsMsg_destruct(NetMessage* this)
{
   GetTargetMappingsMsg_uninit(this);

   os_kfree(this);
}

#endif /* GETTARGETMAPPINGSMSG_H_ */
