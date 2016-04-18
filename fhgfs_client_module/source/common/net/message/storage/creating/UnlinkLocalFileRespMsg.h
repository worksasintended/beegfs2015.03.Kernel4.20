#ifndef UNLINKLOCALFILERESPMSG_H_
#define UNLINKLOCALFILERESPMSG_H_

#include <common/net/message/SimpleIntMsg.h>


struct UnlinkLocalFileRespMsg;
typedef struct UnlinkLocalFileRespMsg UnlinkLocalFileRespMsg;

static inline void UnlinkLocalFileRespMsg_init(UnlinkLocalFileRespMsg* this);
static inline void UnlinkLocalFileRespMsg_initFromValue(UnlinkLocalFileRespMsg* this, int value);
static inline UnlinkLocalFileRespMsg* UnlinkLocalFileRespMsg_construct(void);
static inline UnlinkLocalFileRespMsg* UnlinkLocalFileRespMsg_constructFromValue(int value);
static inline void UnlinkLocalFileRespMsg_uninit(NetMessage* this);
static inline void UnlinkLocalFileRespMsg_destruct(NetMessage* this);

// getters & setters
static inline int UnlinkLocalFileRespMsg_getValue(UnlinkLocalFileRespMsg* this);

struct UnlinkLocalFileRespMsg
{
   SimpleIntMsg simpleIntMsg;
};


void UnlinkLocalFileRespMsg_init(UnlinkLocalFileRespMsg* this)
{
   SimpleIntMsg_init( (SimpleIntMsg*)this, NETMSGTYPE_UnlinkLocalFileResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = UnlinkLocalFileRespMsg_uninit;
}
   
void UnlinkLocalFileRespMsg_initFromValue(UnlinkLocalFileRespMsg* this, int value)
{
   SimpleIntMsg_initFromValue( (SimpleIntMsg*)this, NETMSGTYPE_UnlinkLocalFileResp, value);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = UnlinkLocalFileRespMsg_uninit;
}

UnlinkLocalFileRespMsg* UnlinkLocalFileRespMsg_construct(void)
{
   struct UnlinkLocalFileRespMsg* this = os_kmalloc(sizeof(struct UnlinkLocalFileRespMsg) );
   
   UnlinkLocalFileRespMsg_init(this);
   
   return this;
}

UnlinkLocalFileRespMsg* UnlinkLocalFileRespMsg_constructFromValue(int value)
{
   struct UnlinkLocalFileRespMsg* this = os_kmalloc(sizeof(struct UnlinkLocalFileRespMsg) );
   
   UnlinkLocalFileRespMsg_initFromValue(this, value);
   
   return this;
}

void UnlinkLocalFileRespMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit( (NetMessage*)this);
}

void UnlinkLocalFileRespMsg_destruct(NetMessage* this)
{
   UnlinkLocalFileRespMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


int UnlinkLocalFileRespMsg_getValue(UnlinkLocalFileRespMsg* this)
{
   return SimpleIntMsg_getValue( (SimpleIntMsg*)this);
}



#endif /*UNLINKLOCALFILERESPMSG_H_*/
