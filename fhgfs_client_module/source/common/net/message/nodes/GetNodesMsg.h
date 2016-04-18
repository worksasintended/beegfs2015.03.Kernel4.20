#ifndef GETNODESMSG_H_
#define GETNODESMSG_H_

#include <common/net/message/SimpleIntMsg.h>


struct GetNodesMsg;
typedef struct GetNodesMsg GetNodesMsg;

static inline void GetNodesMsg_init(GetNodesMsg* this);
static inline void GetNodesMsg_initFromValue(GetNodesMsg* this, int nodeType);
static inline GetNodesMsg* GetNodesMsg_construct(void);
static inline GetNodesMsg* GetNodesMsg_constructFromValue(int nodeType);
static inline void GetNodesMsg_uninit(NetMessage* this);
static inline void GetNodesMsg_destruct(NetMessage* this);

// getters & setters
static inline int GetNodesMsg_getValue(GetNodesMsg* this);

struct GetNodesMsg
{
   SimpleIntMsg simpleIntMsg;
};


void GetNodesMsg_init(GetNodesMsg* this)
{
   SimpleIntMsg_init( (SimpleIntMsg*)this, NETMSGTYPE_GetNodes);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = GetNodesMsg_uninit;
}
   
/**
 * @param nodeType NODETYPE_...
 */
void GetNodesMsg_initFromValue(GetNodesMsg* this, int nodeType)
{
   SimpleIntMsg_initFromValue( (SimpleIntMsg*)this, NETMSGTYPE_GetNodes, nodeType);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = GetNodesMsg_uninit;
}

GetNodesMsg* GetNodesMsg_construct(void)
{
   struct GetNodesMsg* this = os_kmalloc(sizeof(*this) );
   
   GetNodesMsg_init(this);
   
   return this;
}

/**
 * @param nodeType NODETYPE_...
 */
GetNodesMsg* GetNodesMsg_constructFromValue(int nodeType)
{
   struct GetNodesMsg* this = os_kmalloc(sizeof(*this) );
   
   GetNodesMsg_initFromValue(this, nodeType);
   
   return this;
}

void GetNodesMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit( (NetMessage*)this);
}

void GetNodesMsg_destruct(NetMessage* this)
{
   GetNodesMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


int GetNodesMsg_getValue(GetNodesMsg* this)
{
   return SimpleIntMsg_getValue( (SimpleIntMsg*)this);
}

#endif /* GETNODESMSG_H_ */
