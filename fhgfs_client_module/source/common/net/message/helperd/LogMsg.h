#ifndef LOGMSG_H_
#define LOGMSG_H_

#include <common/net/message/NetMessage.h>


struct LogMsg;
typedef struct LogMsg LogMsg;

static inline void LogMsg_init(LogMsg* this);
static inline void LogMsg_initFromEntry(LogMsg* this, int level,
   int threadID, const char* threadName, const char* context, const char* logMsg);
static inline LogMsg* LogMsg_construct(void);
static inline LogMsg* LogMsg_constructFromEntry(int level,
   int threadID, const char* threadName, const char* context, const char* logMsg);
static inline void LogMsg_uninit(NetMessage* this);
static inline void LogMsg_destruct(NetMessage* this);

// virtual functions
extern void LogMsg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool LogMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen);
extern unsigned LogMsg_calcMessageLength(NetMessage* this);

// getters & setters
static inline int LogMsg_getLevel(LogMsg* this);
static inline const char* LogMsg_getThreadName(LogMsg* this);
static inline const char* LogMsg_getContext(LogMsg* this);
static inline const char* LogMsg_getLogMsg(LogMsg* this);


struct LogMsg
{
   NetMessage netMessage;

   int level;
   int threadID;
   unsigned threadNameLen;
   const char* threadName;
   unsigned contextLen;
   const char* context;
   unsigned logMsgLen;
   const char* logMsg;
};


void LogMsg_init(LogMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_Log);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = LogMsg_uninit;

   ( (NetMessage*)this)->serializePayload = LogMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = LogMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = LogMsg_calcMessageLength;
}
   
/**
 * @param context just a reference, so do not free it as long as you use this object!
 * @param logMsg just a reference, so do not free it as long as you use this object!
 */
void LogMsg_initFromEntry(LogMsg* this, int level, int threadID, const char* threadName,
   const char* context, const char* logMsg)
{
   LogMsg_init(this);
   
   this->level = level;
   this->threadID = threadID;
   
   this->threadName = threadName;
   this->threadNameLen = os_strlen(threadName);

   this->context = context;
   this->contextLen = os_strlen(context);
   
   this->logMsg = logMsg;
   this->logMsgLen = os_strlen(logMsg);
}

LogMsg* LogMsg_construct(void)
{
   struct LogMsg* this = os_kmalloc(sizeof(struct LogMsg) );
   
   LogMsg_init(this);
   
   return this;
}

/**
 * @param context just a reference, so do not free it as long as you use this object!
 * @param logMsg just a reference, so do not free it as long as you use this object!
 */
LogMsg* LogMsg_constructFromEntry(int level, int threadID, const char* threadName,
   const char* context, const char* logMsg)
{
   struct LogMsg* this = os_kmalloc(sizeof(struct LogMsg) );
   
   LogMsg_initFromEntry(this, level, threadID, threadName, context, logMsg);
   
   return this;
}

void LogMsg_uninit(NetMessage* this)
{
   NetMessage_uninit(this);
}

void LogMsg_destruct(NetMessage* this)
{
   LogMsg_uninit(this);
   
   os_kfree(this);
}

int LogMsg_getLevel(LogMsg* this)
{
   return this->level;
}

const char* LogMsg_getThreadName(LogMsg* this)
{
   return this->threadName;
}

const char* LogMsg_getContext(LogMsg* this)
{
   return this->context;
}

const char* LogMsg_getLogMsg(LogMsg* this)
{
   return this->logMsg;
}

#endif /*LOGMSG_H_*/
