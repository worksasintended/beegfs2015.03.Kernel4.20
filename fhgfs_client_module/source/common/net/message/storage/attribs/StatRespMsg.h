#ifndef STATRESPMSG_H_
#define STATRESPMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/StatData.h>


#define STATRESPMSG_COMPAT_FLAG_HAS_PARENTOWNERNODEID    1 /* deprecated, no longer used
                                                              (due to missing parentEntryID) */

#define STATRESPMSG_FLAG_HAS_PARENTINFO                  1 /* msg includes parentOwnerNodeID and
                                                              parentEntryID */


struct StatRespMsg;
typedef struct StatRespMsg StatRespMsg;

static inline void StatRespMsg_init(StatRespMsg* this);
static inline StatRespMsg* StatRespMsg_construct(void);
static inline void StatRespMsg_uninit(NetMessage* this);
static inline void StatRespMsg_destruct(NetMessage* this);
static inline void StatRespMsg_getParentInfo(StatRespMsg* this,
   uint16_t* outParentNodeID, char** outParentEntryID);

// virtual functions
extern fhgfs_bool StatRespMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen);
extern unsigned StatRespMsg_getSupportedHeaderFeatureFlagsMask(NetMessage* this);

// getters & setters
static inline int StatRespMsg_getResult(StatRespMsg* this);
static inline StatData* StatRespMsg_getStatData(StatRespMsg* this);

struct StatRespMsg
{
   NetMessage netMessage;
   
   int result;
   StatData statData;

   const char* parentEntryID;
   uint16_t parentNodeID;
};


void StatRespMsg_init(StatRespMsg* this)
{
   NetMessage_init( (NetMessage*)this, NETMSGTYPE_StatResp);
   
   // assign virtual functions
   ( (NetMessage*)this)->uninit = StatRespMsg_uninit;

   ( (NetMessage*)this)->serializePayload   = _NetMessage_serializeDummy;
   ( (NetMessage*)this)->deserializePayload = StatRespMsg_deserializePayload;
   ( (NetMessage*)this)->calcMessageLength  = _NetMessage_calcMessageLengthDummy;

   ( (NetMessage*)this)->getSupportedHeaderFeatureFlagsMask =
      StatRespMsg_getSupportedHeaderFeatureFlagsMask;

   this->parentNodeID = 0;
   this->parentEntryID = NULL;
}
   
StatRespMsg* StatRespMsg_construct(void)
{
   struct StatRespMsg* this = os_kmalloc(sizeof(*this) );
   
   StatRespMsg_init(this);
   
   return this;
}

void StatRespMsg_uninit(NetMessage* this)
{
   NetMessage_uninit( (NetMessage*)this);
}

void StatRespMsg_destruct(NetMessage* this)
{
   StatRespMsg_uninit( (NetMessage*)this);
   
   os_kfree(this);
}


int StatRespMsg_getResult(StatRespMsg* this)
{
   return this->result;
}


StatData* StatRespMsg_getStatData(StatRespMsg* this)
{
   return &this->statData;
}

/**
 * Get parentInfo
 *
 * Note: outParentEntryID is a string copy
 */
void StatRespMsg_getParentInfo(StatRespMsg* this,
   uint16_t* outParentNodeID, char** outParentEntryID)
{
   if (outParentNodeID)
   {
      *outParentNodeID = this->parentNodeID;

      if (likely(outParentEntryID) )
      *outParentEntryID = StringTk_strDup(this->parentEntryID);
   }
}

#endif /*STATRESPMSG_H_*/
