#ifndef ACKSTOREMAP_H_
#define ACKSTOREMAP_H_

#include <common/toolkit/tree/PointerRBTree.h>
#include <common/toolkit/tree/PointerRBTreeIter.h>
#include "WaitAckMap.h"


struct AckStoreEntry;
typedef struct AckStoreEntry AckStoreEntry;


struct AckStoreMapElem;
typedef struct AckStoreMapElem AckStoreMapElem;
struct AckStoreMap;
typedef struct AckStoreMap AckStoreMap;

struct AckStoreMapIter; // forward declaration of the iterator


static inline void AckStoreEntry_init(AckStoreEntry* this, char* ackID,
   WaitAckMap* waitMap, WaitAckMap* receivedMap, WaitAckNotification* notifier);
static inline AckStoreEntry* AckStoreEntry_construct(char* ackID,
   WaitAckMap* waitMap, WaitAckMap* receivedMap, WaitAckNotification* notifier);
static inline void AckStoreEntry_uninit(AckStoreEntry* this);
static inline void AckStoreEntry_destruct(AckStoreEntry* this);


static inline void AckStoreMap_init(AckStoreMap* this);
static inline AckStoreMap* AckStoreMap_construct(void);
static inline void AckStoreMap_uninit(AckStoreMap* this);
static inline void AckStoreMap_destruct(AckStoreMap* this);

static inline fhgfs_bool AckStoreMap_insert(AckStoreMap* this, char* newKey,
   AckStoreEntry* newValue);
static fhgfs_bool AckStoreMap_erase(AckStoreMap* this, const char* eraseKey);
static inline size_t AckStoreMap_length(AckStoreMap* this);

static inline void AckStoreMap_clear(AckStoreMap* this);

extern struct AckStoreMapIter AckStoreMap_find(AckStoreMap* this, const char* searchKey);
extern struct AckStoreMapIter AckStoreMap_begin(AckStoreMap* this);

extern int compareAckStoreMapElems(const void* key1, const void* key2);


struct AckStoreEntry
{
      char* ackID;
   
      WaitAckMap* waitMap; // ack will be removed from this map if it is received
      WaitAckMap* receivedMap; // ack will be added to this map if it is received
      
      WaitAckNotification* notifier;
};



struct AckStoreMapElem
{
   RBTreeElem rbTreeElem;
};

struct AckStoreMap
{
   RBTree rbTree;
};


void AckStoreEntry_init(AckStoreEntry* this, char* ackID,
   WaitAckMap* waitMap, WaitAckMap* receivedMap, WaitAckNotification* notifier)
{
   this->ackID = ackID;
   this->waitMap = waitMap;
   this->receivedMap = receivedMap;
   this->notifier = notifier;
}

AckStoreEntry* AckStoreEntry_construct(char* ackID,
   WaitAckMap* waitMap, WaitAckMap* receivedMap, WaitAckNotification* notifier)
{
   AckStoreEntry* this = (AckStoreEntry*)os_kmalloc(sizeof(*this) );
   
   AckStoreEntry_init(this, ackID, waitMap, receivedMap, notifier);
   
   return this;
}

void AckStoreEntry_uninit(AckStoreEntry* this)
{
   // nothing to do here
}

void AckStoreEntry_destruct(AckStoreEntry* this)
{
   AckStoreEntry_uninit(this);
   
   os_kfree(this);
}



void AckStoreMap_init(AckStoreMap* this)
{
   PointerRBTree_init( (RBTree*)this, compareAckStoreMapElems);
}

AckStoreMap* AckStoreMap_construct(void)
{
   AckStoreMap* this = (AckStoreMap*)os_kmalloc(sizeof(*this) );
   
   AckStoreMap_init(this);
   
   return this;
}

void AckStoreMap_uninit(AckStoreMap* this)
{
   while(this->rbTree.length)
   {
      AckStoreMap_erase(this, this->rbTree.treeroot.rootnode->key);
   }
   
   PointerRBTree_uninit( (RBTree*)this);
}

void AckStoreMap_destruct(AckStoreMap* this)
{
   AckStoreMap_uninit(this);
   
   os_kfree(this);
}


fhgfs_bool AckStoreMap_insert(AckStoreMap* this, char* newKey, AckStoreEntry* newValue)
{
   fhgfs_bool insRes;
   
   insRes = PointerRBTree_insert( (RBTree*)this, newKey, newValue);
   if(!insRes)
   {
      // not inserted because the key already existed
   }
   
   return insRes;
}

fhgfs_bool AckStoreMap_erase(AckStoreMap* this, const char* eraseKey)
{
   RBTreeElem* treeElem = _PointerRBTree_findElem( (RBTree*)this, eraseKey);
   if(!treeElem)
   { // element not found
      return fhgfs_false;
   }
   
   return PointerRBTree_erase( (RBTree*)this, eraseKey);
}

size_t AckStoreMap_length(AckStoreMap* this)
{
   return PointerRBTree_length( (RBTree*)this);
}


void AckStoreMap_clear(AckStoreMap* this)
{
   while(this->rbTree.length)
   {
      AckStoreMap_erase(this, this->rbTree.treeroot.rootnode->key);
   }
}


#endif /* ACKSTOREMAP_H_ */
