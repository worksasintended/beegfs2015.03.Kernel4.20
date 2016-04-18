#ifndef MIRRORBUDDYGROUPMAPITER_H_
#define MIRRORBUDDYGROUPMAPITER_H_

#include "MirrorBuddyGroupMap.h"

struct MirrorBuddyGroupMapIter;
typedef struct MirrorBuddyGroupMapIter MirrorBuddyGroupMapIter;

static inline void MirrorBuddyGroupMapIter_init(MirrorBuddyGroupMapIter* this,
   MirrorBuddyGroupMap* map, RBTreeElem* treeElem);
static inline struct MirrorBuddyGroupMapIter* MirrorBuddyGroupMapIter_construct(
   MirrorBuddyGroupMap* map, RBTreeElem* treeElem);
static inline void MirrorBuddyGroupMapIter_uninit(MirrorBuddyGroupMapIter* this);
static inline void MirrorBuddyGroupMapIter_destruct(MirrorBuddyGroupMapIter* this);
static inline fhgfs_bool MirrorBuddyGroupMapIter_hasNext(MirrorBuddyGroupMapIter* this);
static inline void MirrorBuddyGroupMapIter_next(MirrorBuddyGroupMapIter* this);
static inline uint16_t MirrorBuddyGroupMapIter_key(MirrorBuddyGroupMapIter* this);
static inline MirrorBuddyGroup* MirrorBuddyGroupMapIter_value(MirrorBuddyGroupMapIter* this);
static inline fhgfs_bool MirrorBuddyGroupMapIter_end(MirrorBuddyGroupMapIter* this);

struct MirrorBuddyGroupMapIter
{
   RBTreeIter rbTreeIter;
};

void MirrorBuddyGroupMapIter_init(MirrorBuddyGroupMapIter* this, MirrorBuddyGroupMap* map,
   RBTreeElem* treeElem)
{
   PointerRBTreeIter_init((RBTreeIter*) this, (RBTree*) map, (RBTreeElem*) treeElem);
}

MirrorBuddyGroupMapIter* MirrorBuddyGroupMapIter_construct(MirrorBuddyGroupMap* map,
   RBTreeElem* treeElem)
{
   MirrorBuddyGroupMapIter* this = (MirrorBuddyGroupMapIter*) os_kmalloc(sizeof(*this));

   MirrorBuddyGroupMapIter_init(this, map, treeElem);

   return this;
}

void MirrorBuddyGroupMapIter_uninit(MirrorBuddyGroupMapIter* this)
{
   PointerRBTreeIter_uninit((RBTreeIter*) this);
}

void MirrorBuddyGroupMapIter_destruct(MirrorBuddyGroupMapIter* this)
{
   MirrorBuddyGroupMapIter_uninit(this);

   os_kfree(this);
}

fhgfs_bool MirrorBuddyGroupMapIter_hasNext(MirrorBuddyGroupMapIter* this)
{
   return PointerRBTreeIter_hasNext((RBTreeIter*) this);
}

void MirrorBuddyGroupMapIter_next(MirrorBuddyGroupMapIter* this)
{
   PointerRBTreeIter_next((RBTreeIter*) this);
}

uint16_t MirrorBuddyGroupMapIter_key(MirrorBuddyGroupMapIter* this)
{
   return (uint16_t) (size_t) PointerRBTreeIter_key((RBTreeIter*) this);
}

MirrorBuddyGroup* MirrorBuddyGroupMapIter_value(MirrorBuddyGroupMapIter* this)
{
   return (MirrorBuddyGroup*) (size_t) PointerRBTreeIter_value((RBTreeIter*) this);
}

fhgfs_bool MirrorBuddyGroupMapIter_end(MirrorBuddyGroupMapIter* this)
{
   return PointerRBTreeIter_end((RBTreeIter*) this);
}

#endif /* MIRRORBUDDYGROUPMAPITER_H_ */
