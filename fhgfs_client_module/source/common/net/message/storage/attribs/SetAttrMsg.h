#ifndef SETATTRMSG_H_
#define SETATTRMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/StorageDefinitions.h>
#include <common/storage/EntryInfo.h>


#define SETATTRMSG_FLAG_USE_QUOTA            1 /* if the message contains quota informations */


struct SetAttrMsg;
typedef struct SetAttrMsg SetAttrMsg;

static inline void SetAttrMsg_init(SetAttrMsg* this);
static inline void SetAttrMsg_initFromEntryInfo(SetAttrMsg* this, const EntryInfo* entryInfo,
   int validAttribs, SettableFileAttribs* attribs);
static inline SetAttrMsg* SetAttrMsg_construct(void);
static inline SetAttrMsg* SetAttrMsg_constructFromEntryInfo(const EntryInfo* entryInfo,
   int validAttribs, SettableFileAttribs* attribs);
static inline void SetAttrMsg_uninit(NetMessage* this);
static inline void SetAttrMsg_destruct(NetMessage* this);

// virtual functions
extern void SetAttrMsg_serializePayload(NetMessage* this, char* buf);
extern unsigned SetAttrMsg_calcMessageLength(NetMessage* this);

// getters & setters
static inline int SetAttrMsg_getValidAttribs(SetAttrMsg* this);
static inline SettableFileAttribs* SetAttrMsg_getAttribs(SetAttrMsg* this);


struct SetAttrMsg
{
   NetMessage netMessage;

   int validAttribs;
   SettableFileAttribs attribs;
   
   // for serialization
   const EntryInfo* entryInfoPtr;

};


void SetAttrMsg_init(SetAttrMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_SetAttr);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = SetAttrMsg_uninit;

   ( (NetMessage*)this)->serializePayload = SetAttrMsg_serializePayload;
   ( (NetMessage*)this)->deserializePayload = _NetMessage_deserializeDummy;
   ( (NetMessage*)this)->calcMessageLength = SetAttrMsg_calcMessageLength;
}
   
/**
 * @param entryInfo just a reference, so do not free it as long as you use this object!
 * @param validAttribs a combination of SETATTR_CHANGE_...-Flags
 */
void SetAttrMsg_initFromEntryInfo(SetAttrMsg* this, const EntryInfo* entryInfo, int validAttribs,
   SettableFileAttribs* attribs)
{
   SetAttrMsg_init(this);
   
   this->entryInfoPtr = entryInfo;
   this->validAttribs = validAttribs;
   this->attribs = *attribs;
}

SetAttrMsg* SetAttrMsg_construct(void)
{
   struct SetAttrMsg* this = os_kmalloc(sizeof(struct SetAttrMsg) );
   
   SetAttrMsg_init(this);
   
   return this;
}

/**
 * @param entryInfo just a reference, so do not free it as long as you use this object!
 * @param validAttribs a combination of SETATTR_CHANGE_...-Flags
 */
SetAttrMsg* SetAttrMsg_constructFromEntryInfo(const EntryInfo* entryInfo, int validAttribs,
   SettableFileAttribs* attribs)
{
   struct SetAttrMsg* this = os_kmalloc(sizeof(struct SetAttrMsg) );
   
   SetAttrMsg_initFromEntryInfo(this, entryInfo, validAttribs, attribs);
   
   return this;
}

void SetAttrMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void SetAttrMsg_destruct(NetMessage* this)
{
   SetAttrMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}

int SetAttrMsg_getValidAttribs(SetAttrMsg* this)
{
   return this->validAttribs;
}

SettableFileAttribs* SetAttrMsg_getAttribs(SetAttrMsg* this)
{
   return &this->attribs;
}


#endif /*SETATTRMSG_H_*/
