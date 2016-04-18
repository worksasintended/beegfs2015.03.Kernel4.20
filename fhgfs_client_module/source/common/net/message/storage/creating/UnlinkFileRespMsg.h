#ifndef UNLINKFILERESPMSG_H_
#define UNLINKFILERESPMSG_H_

#include <common/net/message/SimpleIntMsg.h>


struct UnlinkFileRespMsg;
typedef struct UnlinkFileRespMsg UnlinkFileRespMsg;

static inline void UnlinkFileRespMsg_init(UnlinkFileRespMsg* this);
static inline void UnlinkFileRespMsg_initFromValue(UnlinkFileRespMsg* this, int value);
static inline UnlinkFileRespMsg* UnlinkFileRespMsg_construct(void);
static inline UnlinkFileRespMsg* UnlinkFileRespMsg_constructFromValue(int value);
static inline void UnlinkFileRespMsg_uninit(NetMessage* this);
static inline void UnlinkFileRespMsg_destruct(NetMessage* this);

// getters & setters
static inline int UnlinkFileRespMsg_getValue(UnlinkFileRespMsg* this);

struct UnlinkFileRespMsg
{
   SimpleIntMsg simpleIntMsg;
};


void UnlinkFileRespMsg_init(UnlinkFileRespMsg* this)
{
   SimpleIntMsg_init( (SimpleIntMsg*)this, NETMSGTYPE_UnlinkFileResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = UnlinkFileRespMsg_uninit;
}
   
void UnlinkFileRespMsg_initFromValue(UnlinkFileRespMsg* this, int value)
{
   SimpleIntMsg_initFromValue( (SimpleIntMsg*)this, NETMSGTYPE_UnlinkFileResp, value);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = UnlinkFileRespMsg_uninit;
}

UnlinkFileRespMsg* UnlinkFileRespMsg_construct(void)
{
   struct UnlinkFileRespMsg* this = os_kmalloc(sizeof(struct UnlinkFileRespMsg) );
   
   UnlinkFileRespMsg_init(this);
   
   return this;
}

UnlinkFileRespMsg* UnlinkFileRespMsg_constructFromValue(int value)
{
   struct UnlinkFileRespMsg* this = os_kmalloc(sizeof(struct UnlinkFileRespMsg) );
   
   UnlinkFileRespMsg_initFromValue(this, value);
   
   return this;
}

void UnlinkFileRespMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit( (NetMessage*)this);
}

void UnlinkFileRespMsg_destruct(NetMessage* this)
{
   UnlinkFileRespMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


int UnlinkFileRespMsg_getValue(UnlinkFileRespMsg* this)
{
   return SimpleIntMsg_getValue( (SimpleIntMsg*)this);
}



#endif /*UNLINKFILERESPMSG_H_*/
