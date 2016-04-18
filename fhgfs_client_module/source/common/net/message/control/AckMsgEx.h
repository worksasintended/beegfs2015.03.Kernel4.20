#ifndef ACKMSGEX_H_
#define ACKMSGEX_H_

#include "../SimpleStringMsg.h"


struct AckMsgEx;
typedef struct AckMsgEx AckMsgEx;

static inline void AckMsgEx_init(AckMsgEx* this);
static inline void AckMsgEx_initFromValue(AckMsgEx* this,
   const char* value);
static inline AckMsgEx* AckMsgEx_construct(void);
static inline AckMsgEx* AckMsgEx_constructFromValue(
   const char* value);
static inline void AckMsgEx_uninit(NetMessage* this);
static inline void AckMsgEx_destruct(NetMessage* this);

// virtual functions
extern fhgfs_bool __AckMsgEx_processIncoming(NetMessage* this, struct App* app,
   fhgfs_sockaddr_in* fromAddr, struct Socket* sock, char* respBuf, size_t bufLen);

// getters & setters
static inline const char* AckMsgEx_getValue(AckMsgEx* this);


struct AckMsgEx
{
   SimpleStringMsg simpleStringMsg;
};


void AckMsgEx_init(AckMsgEx* this)
{
   SimpleStringMsg_init( (SimpleStringMsg*)this, NETMSGTYPE_Ack);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = AckMsgEx_uninit;

   ( (NetMessage*)this)->processIncoming = __AckMsgEx_processIncoming;
}
   
void AckMsgEx_initFromValue(AckMsgEx* this, const char* value)
{
   SimpleStringMsg_initFromValue( (SimpleStringMsg*)this, NETMSGTYPE_Ack, value);

   // assign virtual functions
   ( (NetMessage*)this)->uninit = AckMsgEx_uninit;
}

AckMsgEx* AckMsgEx_construct(void)
{
   struct AckMsgEx* this = os_kmalloc(sizeof(*this) );
   
   AckMsgEx_init(this);
   
   return this;
}

AckMsgEx* AckMsgEx_constructFromValue(const char* value)
{
   struct AckMsgEx* this = os_kmalloc(sizeof(*this) );
   
   AckMsgEx_initFromValue(this, value);
   
   return this;
}

void AckMsgEx_uninit(NetMessage* this)
{
   SimpleStringMsg_uninit( (NetMessage*)this);
}

void AckMsgEx_destruct(NetMessage* this)
{
   AckMsgEx_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


const char* AckMsgEx_getValue(AckMsgEx* this)
{
   return SimpleStringMsg_getValue( (SimpleStringMsg*)this);
}


#endif /* ACKMSGEX_H_ */
