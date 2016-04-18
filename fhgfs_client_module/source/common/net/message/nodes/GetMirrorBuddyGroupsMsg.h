#ifndef GETMIRRORBUDDYGROUPSMSG_H
#define GETMIRRORBUDDYGROUPSMSG_H

#include <common/net/message/SimpleIntMsg.h>

struct GetMirrorBuddyGroupsMsg;
typedef struct GetMirrorBuddyGroupsMsg GetMirrorBuddyGroupsMsg;

static inline void GetMirrorBuddyGroupsMsg_init(GetMirrorBuddyGroupsMsg* this, NodeType nodeType);
static inline GetMirrorBuddyGroupsMsg* GetMirrorBuddyGroupsMsg_construct(NodeType nodeType);
static inline void GetMirrorBuddyGroupsMsg_uninit(NetMessage* this);
static inline void GetMirrorBuddyGroupsMsg_destruct(NetMessage* this);

struct GetMirrorBuddyGroupsMsg
{
   SimpleIntMsg simpleIntMsg;
};

void GetMirrorBuddyGroupsMsg_init(GetMirrorBuddyGroupsMsg* this, NodeType nodeType)
{
   SimpleIntMsg_initFromValue( (SimpleIntMsg*)this, NETMSGTYPE_GetMirrorBuddyGroups, nodeType);

   // virtual functions
   ( (NetMessage*)this)->uninit = GetMirrorBuddyGroupsMsg_uninit;
}

GetMirrorBuddyGroupsMsg* GetMirrorBuddyGroupsMsg_construct(NodeType nodeType)
{
   struct GetMirrorBuddyGroupsMsg* this = os_kmalloc(sizeof(*this) );

   if(likely(this) )
      GetMirrorBuddyGroupsMsg_init(this, nodeType);

   return this;
}

void GetMirrorBuddyGroupsMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit(this);
}

void GetMirrorBuddyGroupsMsg_destruct(NetMessage* this)
{
   GetMirrorBuddyGroupsMsg_uninit(this);

   os_kfree(this);
}

#endif /* GETMIRRORBUDDYGROUPSMSG_H */
