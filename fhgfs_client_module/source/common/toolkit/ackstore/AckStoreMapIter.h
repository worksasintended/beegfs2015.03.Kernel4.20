#ifndef ACKSTOREMAPITER_H_
#define ACKSTOREMAPITER_H_

#include "AckStoreMap.h"

struct AckStoreMapIter;
typedef struct AckStoreMapIter AckStoreMapIter;

static inline void AckStoreMapIter_init(AckStoreMapIter* this, AckStoreMap* map, RBTreeElem* treeElem);
static inline struct AckStoreMapIter* AckStoreMapIter_construct(AckStoreMap* map, RBTreeElem* treeElem);
static inline void AckStoreMapIter_uninit(AckStoreMapIter* this);
static inline void AckStoreMapIter_destruct(AckStoreMapIter* this);
static inline fhgfs_bool AckStoreMapIter_hasNext(AckStoreMapIter* this);
static inline AckStoreEntry* AckStoreMapIter_next(AckStoreMapIter* this);
static inline char* AckStoreMapIter_key(AckStoreMapIter* this);
static inline AckStoreEntry* AckStoreMapIter_value(AckStoreMapIter* this);
static inline fhgfs_bool AckStoreMapIter_end(AckStoreMapIter* this);

struct AckStoreMapIter
{
   RBTreeIter rbTreeIter;
};

void AckStoreMapIter_init(AckStoreMapIter* this, AckStoreMap* map, RBTreeElem* treeElem)
{
   PointerRBTreeIter_init( (RBTreeIter*)this, (RBTree*)map, (RBTreeElem*)treeElem);
}

AckStoreMapIter* AckStoreMapIter_construct(AckStoreMap* map, RBTreeElem* treeElem)
{
   AckStoreMapIter* this = (AckStoreMapIter*)os_kmalloc(sizeof(*this) );
   
   AckStoreMapIter_init(this, map, treeElem);
   
   return this;
}

void AckStoreMapIter_uninit(AckStoreMapIter* this)
{
   PointerRBTreeIter_uninit( (RBTreeIter*)this);
}

void AckStoreMapIter_destruct(AckStoreMapIter* this)
{
   AckStoreMapIter_uninit(this);
   
   os_kfree(this);
}

fhgfs_bool AckStoreMapIter_hasNext(AckStoreMapIter* this)
{
   return PointerRBTreeIter_hasNext( (RBTreeIter*)this);
}

AckStoreEntry* AckStoreMapIter_next(AckStoreMapIter* this)
{
   return (AckStoreEntry*)PointerRBTreeIter_next( (RBTreeIter*)this);
}

char* AckStoreMapIter_key(AckStoreMapIter* this)
{
   return (char*)PointerRBTreeIter_key( (RBTreeIter*)this);
}

AckStoreEntry* AckStoreMapIter_value(AckStoreMapIter* this)
{
   return (AckStoreEntry*)PointerRBTreeIter_value( (RBTreeIter*)this);
}

fhgfs_bool AckStoreMapIter_end(AckStoreMapIter* this)
{
   return PointerRBTreeIter_end( (RBTreeIter*)this);
}


#endif /* ACKSTOREMAPITER_H_ */
