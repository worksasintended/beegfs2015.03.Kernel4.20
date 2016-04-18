#ifndef WAITACKMAP_H_
#define WAITACKMAP_H_

#include <common/nodes/Node.h>
#include <common/toolkit/tree/PointerRBTree.h>
#include <common/toolkit/tree/PointerRBTreeIter.h>
#include <common/threading/Mutex.h>
#include <common/threading/Condition.h>


struct WaitAckNotification;
typedef struct WaitAckNotification WaitAckNotification;

struct WaitAck;
typedef struct WaitAck WaitAck;


struct WaitAckMapElem;
typedef struct WaitAckMapElem WaitAckMapElem;
struct WaitAckMap;
typedef struct WaitAckMap WaitAckMap;

struct WaitAckMapIter; // forward declaration of the iterator


static inline void WaitAckNotification_init(WaitAckNotification* this);
static inline void WaitAckNotification_uninit(WaitAckNotification* this);

static inline void WaitAck_init(WaitAck* this, char* ackID, void* privateData);
static inline WaitAck* WaitAck_construct(char* ackID, void* privateData);
static inline void WaitAck_uninit(WaitAck* this);
static inline void WaitAck_destruct(WaitAck* this);


static inline void WaitAckMap_init(WaitAckMap* this);
static inline WaitAckMap* WaitAckMap_construct(void);
static inline void WaitAckMap_uninit(WaitAckMap* this);
static inline void WaitAckMap_destruct(WaitAckMap* this);

static inline fhgfs_bool WaitAckMap_insert(WaitAckMap* this, char* newKey,
   WaitAck* newValue);
static fhgfs_bool WaitAckMap_erase(WaitAckMap* this, const char* eraseKey);
static inline size_t WaitAckMap_length(WaitAckMap* this);

static inline void WaitAckMap_clear(WaitAckMap* this);

extern struct WaitAckMapIter WaitAckMap_find(WaitAckMap* this, const char* searchKey);
extern struct WaitAckMapIter WaitAckMap_begin(WaitAckMap* this);

extern int compareWaitAckMapElems(const void* key1, const void* key2);


struct WaitAckNotification
{
   Mutex* waitAcksMutex; // this mutex also syncs access to the waitMap/receivedMap during the
      // wait phase (which is between registration and deregistration)
   Condition* waitAcksCompleteCond; // in case all WaitAcks have been received
};

struct WaitAck
{
      char* ackID;
      void* privateData; // caller's private data
};



struct WaitAckMapElem
{
   RBTreeElem rbTreeElem;
};

struct WaitAckMap
{
   RBTree rbTree;
};


void WaitAckNotification_init(WaitAckNotification* this)
{
   this->waitAcksMutex = Mutex_construct();
   this->waitAcksCompleteCond = Condition_construct();
}

void WaitAckNotification_uninit(WaitAckNotification* this)
{
   Condition_destruct(this->waitAcksCompleteCond);
   Mutex_destruct(this->waitAcksMutex);
}

/**
 * @param ackID is not copied, so don't free or touch it while you use this object
 * @param privateData any private data that helps the caller to identify to ack later
 */
void WaitAck_init(WaitAck* this, char* ackID, void* privateData)
{
   this->ackID = ackID;
   this->privateData = privateData;
}

/**
 * @param ackID is not copied, so don't free or touch it while you use this object
 * @param privateData any private data that helps the caller to identify to ack later
 */
WaitAck* WaitAck_construct(char* ackID, void* privateData)
{
   WaitAck* this = (WaitAck*)os_kmalloc(sizeof(*this) );
   
   WaitAck_init(this, ackID, privateData);
   
   return this;
}

void WaitAck_uninit(WaitAck* this)
{
   // nothing to do here
}

void WaitAck_destruct(WaitAck* this)
{
   WaitAck_uninit(this);
   
   os_kfree(this);
}


void WaitAckMap_init(WaitAckMap* this)
{
   PointerRBTree_init( (RBTree*)this, compareWaitAckMapElems);
}

WaitAckMap* WaitAckMap_construct(void)
{
   WaitAckMap* this = (WaitAckMap*)os_kmalloc(sizeof(*this) );
   
   WaitAckMap_init(this);
   
   return this;
}

void WaitAckMap_uninit(WaitAckMap* this)
{
   while(this->rbTree.length)
   {
      WaitAckMap_erase(this, this->rbTree.treeroot.rootnode->key);
   }
   
   PointerRBTree_uninit( (RBTree*)this);
}

void WaitAckMap_destruct(WaitAckMap* this)
{
   WaitAckMap_uninit(this);
   
   os_kfree(this);
}


fhgfs_bool WaitAckMap_insert(WaitAckMap* this, char* newKey, WaitAck* newValue)
{
   fhgfs_bool insRes;
   
   insRes = PointerRBTree_insert( (RBTree*)this, newKey, newValue);
   if(!insRes)
   {
      // not inserted because the key already existed
   }
   
   return insRes;
}

fhgfs_bool WaitAckMap_erase(WaitAckMap* this, const char* eraseKey)
{
   RBTreeElem* treeElem = _PointerRBTree_findElem( (RBTree*)this, eraseKey);
   if(!treeElem)
   { // element not found
      return fhgfs_false;
   }
   
   return PointerRBTree_erase( (RBTree*)this, eraseKey);
}

size_t WaitAckMap_length(WaitAckMap* this)
{
   return PointerRBTree_length( (RBTree*)this);
}


void WaitAckMap_clear(WaitAckMap* this)
{
   while(this->rbTree.length)
   {
      WaitAckMap_erase(this, this->rbTree.treeroot.rootnode->key);
   }
}


#endif /* WAITACKMAP_H_ */
