#ifndef LISTDIRFROMOFFSETRESPMSG_H_
#define LISTDIRFROMOFFSETRESPMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/Common.h>


struct ListDirFromOffsetRespMsg;
typedef struct ListDirFromOffsetRespMsg ListDirFromOffsetRespMsg;

static inline void ListDirFromOffsetRespMsg_init(ListDirFromOffsetRespMsg* this);
static inline ListDirFromOffsetRespMsg* ListDirFromOffsetRespMsg_construct(void);
static inline void ListDirFromOffsetRespMsg_uninit(NetMessage* this);
static inline void ListDirFromOffsetRespMsg_destruct(NetMessage* this);

// virtual functions
extern void ListDirFromOffsetRespMsg_serializePayload(NetMessage* this, char* buf);
extern fhgfs_bool ListDirFromOffsetRespMsg_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen);
extern unsigned ListDirFromOffsetRespMsg_calcMessageLength(NetMessage* this);

// inliners
static inline void ListDirFromOffsetRespMsg_parseNames(ListDirFromOffsetRespMsg* this,
   StrCpyVec* outNames);
static inline void ListDirFromOffsetRespMsg_parseEntryIDs(ListDirFromOffsetRespMsg* this,
   StrCpyVec* outEntryIDs);
static inline void ListDirFromOffsetRespMsg_parseEntryTypes(ListDirFromOffsetRespMsg* this,
   UInt8Vec* outEntryTypes);
static inline void ListDirFromOffsetRespMsg_parseServerOffsets(ListDirFromOffsetRespMsg* this,
   Int64CpyVec* outServerOffsets);

// getters & setters
static inline int ListDirFromOffsetRespMsg_getResult(ListDirFromOffsetRespMsg* this);
static inline uint64_t ListDirFromOffsetRespMsg_getNewServerOffset(ListDirFromOffsetRespMsg* this);


/**
 * Note: Only deserialization supported, no serialization.
 */
struct ListDirFromOffsetRespMsg
{
   NetMessage netMessage;
   
   int result;
   
   int64_t newServerOffset;

   // for deserialization
   unsigned namesElemNum;
   const char* namesListStart;
   unsigned namesBufLen;

   unsigned entryTypesElemNum;
   const char* entryTypesListStart;
   unsigned entryTypesBufLen;

   unsigned entryIDsElemNum;
   const char* entryIDsListStart;
   unsigned entryIDsBufLen;

   unsigned serverOffsetsElemNum;
   const char* serverOffsetsListStart;
   unsigned serverOffsetsBufLen;
};


void ListDirFromOffsetRespMsg_init(ListDirFromOffsetRespMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_ListDirFromOffsetResp);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = ListDirFromOffsetRespMsg_uninit;

   ( (NetMessage*)this)->serializePayload = _NetMessage_serializeDummy;
   ( (NetMessage*)this)->deserializePayload = ListDirFromOffsetRespMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength = _NetMessage_calcMessageLengthDummy;
}
   
ListDirFromOffsetRespMsg* ListDirFromOffsetRespMsg_construct(void)
{
   struct ListDirFromOffsetRespMsg* this = os_kmalloc(sizeof(*this) );

   if(likely(this) )
      ListDirFromOffsetRespMsg_init(this);
   
   return this;
}

void ListDirFromOffsetRespMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void ListDirFromOffsetRespMsg_destruct(NetMessage* this)
{
   ListDirFromOffsetRespMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


void ListDirFromOffsetRespMsg_parseEntryIDs(ListDirFromOffsetRespMsg* this, StrCpyVec* outEntryIDs)
{
   Serialization_deserializeStrCpyVec(
      this->entryIDsBufLen, this->entryIDsElemNum, this->entryIDsListStart, outEntryIDs);
}

void ListDirFromOffsetRespMsg_parseNames(ListDirFromOffsetRespMsg* this, StrCpyVec* outNames)
{
   Serialization_deserializeStrCpyVec(
      this->namesBufLen, this->namesElemNum, this->namesListStart, outNames);
}

void ListDirFromOffsetRespMsg_parseEntryTypes(ListDirFromOffsetRespMsg* this,
   UInt8Vec* outEntryTypes)
{
   Serialization_deserializeUInt8Vec(
      this->entryTypesBufLen, this->entryTypesElemNum, this->entryTypesListStart, outEntryTypes);
}

void ListDirFromOffsetRespMsg_parseServerOffsets(ListDirFromOffsetRespMsg* this,
   Int64CpyVec* outServerOffsets)
{
   Serialization_deserializeInt64CpyVec(
      this->serverOffsetsBufLen, this->serverOffsetsElemNum, this->serverOffsetsListStart,
      outServerOffsets);
}

int ListDirFromOffsetRespMsg_getResult(ListDirFromOffsetRespMsg* this)
{
   return this->result;
}

uint64_t ListDirFromOffsetRespMsg_getNewServerOffset(ListDirFromOffsetRespMsg* this)
{
   return this->newServerOffset;
}


#endif /*LISTDIRFROMOFFSETRESPMSG_H_*/
