#ifndef TRUNCFILEMSG_H_
#define TRUNCFILEMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/EntryInfo.h>


#define TRUNCFILEMSG_FLAG_USE_QUOTA       1 /* if the message contains quota informations */


struct TruncFileMsg;
typedef struct TruncFileMsg TruncFileMsg;

static inline void TruncFileMsg_init(TruncFileMsg* this);
static inline void TruncFileMsg_initFromEntryInfo(TruncFileMsg* this, int64_t filesize,
   const EntryInfo* entryInfo);
static inline TruncFileMsg* TruncFileMsg_construct(void);
static inline TruncFileMsg* TruncFileMsg_constructFromEntryInfo(int64_t filesize,
   const EntryInfo* entryInfo);
static inline void TruncFileMsg_uninit(NetMessage* this);
static inline void TruncFileMsg_destruct(NetMessage* this);

// virtual functions
extern void TruncFileMsg_serializePayload(NetMessage* this, char* buf);
extern unsigned TruncFileMsg_calcMessageLength(NetMessage* this);



struct TruncFileMsg
{
   NetMessage netMessage;
   
   int64_t filesize;

   // for serialization
   const EntryInfo* entryInfoPtr;  // not owned by this object!
};


void TruncFileMsg_init(TruncFileMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_TruncFile);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = TruncFileMsg_uninit;

   ( (NetMessage*)this)->serializePayload = TruncFileMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength = TruncFileMsg_calcMessageLength;
}
   
/**
 * @param entryInfo just a reference, so do not free it as long as you use this object!
 */
void TruncFileMsg_initFromEntryInfo(TruncFileMsg* this, int64_t filesize,
   const EntryInfo* entryInfo)
{
   TruncFileMsg_init(this);
   
   this->filesize = filesize;
   
   this->entryInfoPtr = entryInfo;
}

TruncFileMsg* TruncFileMsg_construct(void)
{
   struct TruncFileMsg* this = os_kmalloc(sizeof(*this) );
   
   TruncFileMsg_init(this);
   
   return this;
}

/**
 * @param entryID just a reference, so do not free it as long as you use this object!
 */
TruncFileMsg* TruncFileMsg_constructFromEntryInfo(int64_t filesize, const EntryInfo* entryInfo)
{
   struct TruncFileMsg* this = os_kmalloc(sizeof(*this) );
   
   TruncFileMsg_initFromEntryInfo(this, filesize, entryInfo);
   
   return this;
}

void TruncFileMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void TruncFileMsg_destruct(NetMessage* this)
{
   TruncFileMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}



#endif /*TRUNCFILEMSG_H_*/
