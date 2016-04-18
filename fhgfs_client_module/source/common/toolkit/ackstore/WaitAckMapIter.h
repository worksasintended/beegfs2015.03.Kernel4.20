#ifndef WAITACKMAPITER_H_
#define WAITACKMAPITER_H_

#include "WaitAckMap.h"

struct WaitAckMapIter;
typedef struct WaitAckMapIter WaitAckMapIter;

static inline void WaitAckMapIter_init(WaitAckMapIter* this, WaitAckMap* map, RBTreeElem* treeElem);
static inline struct WaitAckMapIter* WaitAckMapIter_construct(WaitAckMap* map, RBTreeElem* treeElem);
static inline void WaitAckMapIter_uninit(WaitAckMapIter* this);
static inline void WaitAckMapIter_destruct(WaitAckMapIter* this);
static inline fhgfs_bool WaitAckMapIter_hasNext(WaitAckMapIter* this);
static inline WaitAck* WaitAckMapIter_next(WaitAckMapIter* this);
static inline char* WaitAckMapIter_key(WaitAckMapIter* this);
static inline WaitAck* WaitAckMapIter_value(WaitAckMapIter* this);
static inline fhgfs_bool WaitAckMapIter_end(WaitAckMapIter* this);

struct WaitAckMapIter
{
   RBTreeIter rbTreeIter;
};

void WaitAckMapIter_init(WaitAckMapIter* this, WaitAckMap* map, RBTreeElem* treeElem)
{
   PointerRBTreeIter_init( (RBTreeIter*)this, (RBTree*)map, (RBTreeElem*)treeElem);
}

WaitAckMapIter* WaitAckMapIter_construct(WaitAckMap* map, RBTreeElem* treeElem)
{
   WaitAckMapIter* this = (WaitAckMapIter*)os_kmalloc(sizeof(*this) );
   
   WaitAckMapIter_init(this, map, treeElem);
   
   return this;
}

void WaitAckMapIter_uninit(WaitAckMapIter* this)
{
   PointerRBTreeIter_uninit( (RBTreeIter*)this);
}

void WaitAckMapIter_destruct(WaitAckMapIter* this)
{
   WaitAckMapIter_uninit(this);
   
   os_kfree(this);
}

fhgfs_bool WaitAckMapIter_hasNext(WaitAckMapIter* this)
{
   return PointerRBTreeIter_hasNext( (RBTreeIter*)this);
}

WaitAck* WaitAckMapIter_next(WaitAckMapIter* this)
{
   return (WaitAck*)PointerRBTreeIter_next( (RBTreeIter*)this);
}

char* WaitAckMapIter_key(WaitAckMapIter* this)
{
   return (char*)PointerRBTreeIter_key( (RBTreeIter*)this);
}

WaitAck* WaitAckMapIter_value(WaitAckMapIter* this)
{
   return (WaitAck*)PointerRBTreeIter_value( (RBTreeIter*)this);
}

fhgfs_bool WaitAckMapIter_end(WaitAckMapIter* this)
{
   return PointerRBTreeIter_end( (RBTreeIter*)this);
}


#endif /* WAITACKMAPITER_H_ */
