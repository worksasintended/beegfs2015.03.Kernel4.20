#ifndef GETSTATESANDBUDDYGROUPSMSG_H_
#define GETSTATESANDBUDDYGROUPSMSG_H_

#include <common/net/message/SimpleIntMsg.h>


struct GetStatesAndBuddyGroupsMsg;
typedef struct GetStatesAndBuddyGroupsMsg GetStatesAndBuddyGroupsMsg;


static inline void GetStatesAndBuddyGroupsMsg_init(GetStatesAndBuddyGroupsMsg* this,
   NodeType nodeType);
static inline GetStatesAndBuddyGroupsMsg* GetStatesAndBuddyGroupsMsg_construct(NodeType nodeType);
static inline void GetStatesAndBuddyGroupsMsg_uninit(NetMessage* this);
static inline void GetStatesAndBuddyGroupsMsg_destruct(NetMessage* this);


struct GetStatesAndBuddyGroupsMsg
{
   SimpleIntMsg simpleIntMsg;
};


void GetStatesAndBuddyGroupsMsg_init(GetStatesAndBuddyGroupsMsg* this, NodeType nodeType)
{
   SimpleIntMsg_initFromValue( (SimpleIntMsg*)this, NETMSGTYPE_GetStatesAndBuddyGroups, nodeType);

   ( (NetMessage*)this)->uninit = GetStatesAndBuddyGroupsMsg_uninit;
}

GetStatesAndBuddyGroupsMsg* GetStatesAndBuddyGroupsMsg_construct(NodeType nodeType)
{
   struct GetStatesAndBuddyGroupsMsg* this = os_kmalloc(sizeof(*this) );

   if(likely(this) )
      GetStatesAndBuddyGroupsMsg_init(this, nodeType);

   return this;
}

void GetStatesAndBuddyGroupsMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit(this);
}

void GetStatesAndBuddyGroupsMsg_destruct(NetMessage* this)
{
   GetStatesAndBuddyGroupsMsg_uninit(this);

   os_kfree(this);
}


#endif /* GETSTATESANDBUDDYGROUPSMSG_H_ */
