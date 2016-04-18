#ifndef LOGRESPMSG_H_
#define LOGRESPMSG_H_

#include <common/net/message/SimpleIntMsg.h>


struct LogRespMsg;
typedef struct LogRespMsg LogRespMsg;

static inline void LogRespMsg_init(LogRespMsg* this);
static inline void LogRespMsg_initFromValue(LogRespMsg* this, int value);
static inline LogRespMsg* LogRespMsg_construct(void);
static inline LogRespMsg* LogRespMsg_constructFromValue(int value);
static inline void LogRespMsg_uninit(NetMessage* this);
static inline void LogRespMsg_destruct(NetMessage* this);

// getters & setters
static inline int LogRespMsg_getValue(LogRespMsg* this);

struct LogRespMsg
{
   SimpleIntMsg simpleIntMsg;
};


void LogRespMsg_init(LogRespMsg* this)
{
   SimpleIntMsg_init( (SimpleIntMsg*)this, NETMSGTYPE_LogResp);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = LogRespMsg_uninit;
}
   
void LogRespMsg_initFromValue(LogRespMsg* this, int value)
{
   SimpleIntMsg_initFromValue( (SimpleIntMsg*)this, NETMSGTYPE_LogResp, value);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = LogRespMsg_uninit;
}

LogRespMsg* LogRespMsg_construct(void)
{
   struct LogRespMsg* this = os_kmalloc(sizeof(struct LogRespMsg) );
   
   LogRespMsg_init(this);
   
   return this;
}

LogRespMsg* LogRespMsg_constructFromValue(int value)
{
   struct LogRespMsg* this = os_kmalloc(sizeof(struct LogRespMsg) );
   
   LogRespMsg_initFromValue(this, value);
   
   return this;
}

void LogRespMsg_uninit(NetMessage* this)
{
   SimpleIntMsg_uninit(this);
}

void LogRespMsg_destruct(NetMessage* this)
{
   LogRespMsg_uninit(this);
   
   os_kfree(this);
}


int LogRespMsg_getValue(LogRespMsg* this)
{
   return SimpleIntMsg_getValue( (SimpleIntMsg*)this);
}


#endif /*LOGRESPMSG_H_*/
