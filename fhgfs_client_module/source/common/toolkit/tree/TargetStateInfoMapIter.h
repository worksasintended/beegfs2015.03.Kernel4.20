#ifndef TARGETSTATEINFOMAPITER_H_
#define TARGETSTATEINFOMAPITER_H_

#include "TargetStateInfoMap.h"

struct TargetStateInfoMapIter;
typedef struct TargetStateInfoMapIter TargetStateInfoMapIter;

static inline void TargetStateInfoMapIter_init(TargetStateInfoMapIter* this,
   TargetStateInfoMap* map, RBTreeElem* treeElem);
static inline struct TargetStateInfoMapIter* TargetStateInfoMapIter_construct(
   TargetStateInfoMap* map, RBTreeElem* treeElem);
static inline void TargetStateInfoMapIter_uninit(TargetStateInfoMapIter* this);
static inline void TargetStateInfoMapIter_destruct(TargetStateInfoMapIter* this);
static inline fhgfs_bool TargetStateInfoMapIter_hasNext(TargetStateInfoMapIter* this);
static inline void TargetStateInfoMapIter_next(TargetStateInfoMapIter* this);
static inline uint16_t TargetStateInfoMapIter_key(TargetStateInfoMapIter* this);
static inline TargetStateInfo* TargetStateInfoMapIter_value(TargetStateInfoMapIter* this);
static inline fhgfs_bool TargetStateInfoMapIter_end(TargetStateInfoMapIter* this);

struct TargetStateInfoMapIter
{
   RBTreeIter rbTreeIter;
};

void TargetStateInfoMapIter_init(TargetStateInfoMapIter* this, TargetStateInfoMap* map,
   RBTreeElem* treeElem)
{
   PointerRBTreeIter_init((RBTreeIter*) this, (RBTree*) map, (RBTreeElem*) treeElem);
}

TargetStateInfoMapIter* TargetStateInfoMapIter_construct(TargetStateInfoMap* map,
   RBTreeElem* treeElem)
{
   TargetStateInfoMapIter* this = (TargetStateInfoMapIter*) os_kmalloc(sizeof(*this));

   TargetStateInfoMapIter_init(this, map, treeElem);

   return this;
}

void TargetStateInfoMapIter_uninit(TargetStateInfoMapIter* this)
{
   PointerRBTreeIter_uninit((RBTreeIter*) this);
}

void TargetStateInfoMapIter_destruct(TargetStateInfoMapIter* this)
{
   TargetStateInfoMapIter_uninit(this);

   os_kfree(this);
}

fhgfs_bool TargetStateInfoMapIter_hasNext(TargetStateInfoMapIter* this)
{
   return PointerRBTreeIter_hasNext((RBTreeIter*) this);
}

void TargetStateInfoMapIter_next(TargetStateInfoMapIter* this)
{
   PointerRBTreeIter_next((RBTreeIter*) this);
}

uint16_t TargetStateInfoMapIter_key(TargetStateInfoMapIter* this)
{
   return (uint16_t) (size_t) PointerRBTreeIter_key((RBTreeIter*) this);
}

TargetStateInfo* TargetStateInfoMapIter_value(TargetStateInfoMapIter* this)
{
   return (TargetStateInfo*) (size_t) PointerRBTreeIter_value((RBTreeIter*) this);
}

fhgfs_bool TargetStateInfoMapIter_end(TargetStateInfoMapIter* this)
{
   return PointerRBTreeIter_end((RBTreeIter*) this);
}

#endif /* TARGETSTATEINFOMAPITER_H_ */
