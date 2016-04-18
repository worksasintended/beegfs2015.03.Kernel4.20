#ifndef TARGETSTATEINFOMAP_H_
#define TARGETSTATEINFOMAP_H_


#include <common/nodes/TargetStateInfo.h>
#include "PointerRBTree.h"
#include "PointerRBTreeIter.h"

/**
 * We assign the uint16 keys directly to the key pointers (i.e. no extra allocation
 * involved, but a lot of casting to convince gcc that we know what we're doing).
 * Values are TargetStateInfo pointers.
 */

struct TargetStateInfoMapElem;
typedef struct TargetStateInfoMapElem TargetStateInfoMapElem;
struct TargetStateInfoMap;
typedef struct TargetStateInfoMap TargetStateInfoMap;

struct TargetStateInfoMapIter; // forward declaration of the iterator

static inline void TargetStateInfoMap_init(TargetStateInfoMap* this);
static inline TargetStateInfoMap* TargetStateInfoMap_construct(void);
static inline void TargetStateInfoMap_uninit(TargetStateInfoMap* this);
static inline void TargetStateInfoMap_destruct(TargetStateInfoMap* this);

static inline fhgfs_bool TargetStateInfoMap_insert(TargetStateInfoMap* this, uint16_t newKey,
   TargetStateInfo* newValue);
static inline fhgfs_bool TargetStateInfoMap_erase(TargetStateInfoMap* this,
   const uint16_t eraseKey);
static inline size_t TargetStateInfoMap_length(TargetStateInfoMap* this);

static inline void TargetStateInfoMap_clear(TargetStateInfoMap* this);

extern struct TargetStateInfoMapIter TargetStateInfoMap_find(TargetStateInfoMap* this,
   const uint16_t searchKey);
extern struct TargetStateInfoMapIter TargetStateInfoMap_begin(TargetStateInfoMap* this);

extern int compareTargetStateInfoMapElems(const void* key1, const void* key2);


struct TargetStateInfoMapElem
{
   RBTreeElem rbTreeElem;
};

struct TargetStateInfoMap
{
   RBTree rbTree;
};


void TargetStateInfoMap_init(TargetStateInfoMap* this)
{
   PointerRBTree_init( (RBTree*)this, compareTargetStateInfoMapElems);
}

TargetStateInfoMap* TargetStateInfoMap_construct(void)
{
   TargetStateInfoMap* this = (TargetStateInfoMap*)os_kmalloc(sizeof(*this) );

   TargetStateInfoMap_init(this);

   return this;
}

void TargetStateInfoMap_uninit(TargetStateInfoMap* this)
{
   PointerRBTree_uninit( (RBTree*)this);
}

void TargetStateInfoMap_destruct(TargetStateInfoMap* this)
{
   TargetStateInfoMap_uninit(this);

   os_kfree(this);
}


/**
 * @param newValue just assigned, not copied.
 * @return fhgfs_true if element was inserted, fhgfs_false if key already existed (in which case
 * nothing is changed)
 */
fhgfs_bool TargetStateInfoMap_insert(TargetStateInfoMap* this, uint16_t newKey,
   TargetStateInfo* newValue)
{
   return PointerRBTree_insert( (RBTree*)this, (void*)(size_t)newKey, newValue);
}

/**
 * @return fhgfs_false if no element with the given key existed
 */
fhgfs_bool TargetStateInfoMap_erase(TargetStateInfoMap* this, const uint16_t eraseKey)
{
   return PointerRBTree_erase( (RBTree*)this, (const void*)(const size_t)eraseKey);
}

size_t TargetStateInfoMap_length(TargetStateInfoMap* this)
{
   return PointerRBTree_length( (RBTree*)this);
}


void TargetStateInfoMap_clear(TargetStateInfoMap* this)
{
   while(this->rbTree.length)
   {
      TargetStateInfoMap_erase(this, (int)(size_t)this->rbTree.treeroot.rootnode->key);
   }
}

#endif /* TARGETSTATEINFOMAP_H_ */
