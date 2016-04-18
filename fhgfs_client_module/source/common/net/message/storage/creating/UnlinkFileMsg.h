#ifndef UNLINKFILEMSG_H_
#define UNLINKFILEMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/EntryInfo.h>


struct UnlinkFileMsg;
typedef struct UnlinkFileMsg UnlinkFileMsg;

static inline void UnlinkFileMsg_init(UnlinkFileMsg* this);
static inline void UnlinkFileMsg_initFromEntryInfo(UnlinkFileMsg* this,
   const EntryInfo* parentInfo, const char* delFileName);
static inline UnlinkFileMsg* UnlinkFileMsg_construct(void);
static inline void UnlinkFileMsg_uninit(NetMessage* this);
static inline void UnlinkFileMsg_destruct(NetMessage* this);

// virtual functions
extern void UnlinkFileMsg_serializePayload(NetMessage* this, char* buf);
extern unsigned UnlinkFileMsg_calcMessageLength(NetMessage* this);


struct UnlinkFileMsg
{
   NetMessage netMessage;
   
   // for serialization
   const EntryInfo* parentInfoPtr; // not owned by this object!
   const char* delFileName;        // file name to be delete, not owned by this object
   unsigned delFileNameLen;

};


void UnlinkFileMsg_init(UnlinkFileMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_UnlinkFile);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = UnlinkFileMsg_uninit;

   ( (NetMessage*)this)->serializePayload = UnlinkFileMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength = UnlinkFileMsg_calcMessageLength;
}
   
/**
 * @param path just a reference, so do not free it as long as you use this object!
 */
void UnlinkFileMsg_initFromEntryInfo(UnlinkFileMsg* this, const EntryInfo* parentInfo,
   const char* delFileName)
{
   UnlinkFileMsg_init(this);
   
   this->parentInfoPtr  = parentInfo;
   this->delFileName    = delFileName;
   this->delFileNameLen = os_strlen(delFileName);
}

UnlinkFileMsg* UnlinkFileMsg_construct(void)
{
   struct UnlinkFileMsg* this = os_kmalloc(sizeof(struct UnlinkFileMsg) );
   
   UnlinkFileMsg_init(this);
   
   return this;
}

void UnlinkFileMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void UnlinkFileMsg_destruct(NetMessage* this)
{
   UnlinkFileMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


#endif /*UNLINKFILEMSG_H_*/
