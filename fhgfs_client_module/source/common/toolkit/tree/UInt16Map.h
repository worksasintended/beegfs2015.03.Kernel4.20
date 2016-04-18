#ifndef UINT16MAP_H_
#define UINT16MAP_H_

#include "PointerRBTree.h"
#include "PointerRBTreeIter.h"


/**
 * We assign the integer keys directly to the key pointers (i.e. no extra allocation involved,
 * but a lot of casting to convince gcc that we know what we're doing).
 * Same for the uint16_t values.
 */


struct UInt16MapElem;
typedef struct UInt16MapElem UInt16MapElem;
struct UInt16Map;
typedef struct UInt16Map UInt16Map;

struct UInt16MapIter; // forward declaration of the iterator

static inline void UInt16Map_init(UInt16Map* this);
static inline UInt16Map* UInt16Map_construct(void);
static inline void UInt16Map_uninit(UInt16Map* this);
static inline void UInt16Map_destruct(UInt16Map* this);

static inline fhgfs_bool UInt16Map_insert(UInt16Map* this, uint16_t newKey, uint16_t newValue);
static inline fhgfs_bool UInt16Map_erase(UInt16Map* this, const uint16_t eraseKey);
static inline size_t UInt16Map_length(UInt16Map* this);

static inline void UInt16Map_clear(UInt16Map* this);

extern struct UInt16MapIter UInt16Map_find(UInt16Map* this, const uint16_t searchKey);
extern struct UInt16MapIter UInt16Map_begin(UInt16Map* this);

extern int compareUInt16MapElems(const void* key1, const void* key2);


struct UInt16MapElem
{
   RBTreeElem rbTreeElem;
};

/**
 * keys: uint16_t; values: uint16_t
 */
struct UInt16Map
{
   RBTree rbTree;
};


void UInt16Map_init(UInt16Map* this)
{
   PointerRBTree_init( (RBTree*)this, compareUInt16MapElems);
}

UInt16Map* UInt16Map_construct(void)
{
   UInt16Map* this = (UInt16Map*)os_kmalloc(sizeof(*this) );

   UInt16Map_init(this);

   return this;
}

void UInt16Map_uninit(UInt16Map* this)
{
   // (nothing alloc'ed by this sub map type => nothing special here to be free'd)

   PointerRBTree_uninit( (RBTree*)this);
}

void UInt16Map_destruct(UInt16Map* this)
{
   UInt16Map_uninit(this);

   os_kfree(this);
}


/**
 * @return fhgfs_true if element was inserted, fhgfs_false if key already existed (in which case
 * nothing is changed)
 */
fhgfs_bool UInt16Map_insert(UInt16Map* this, uint16_t newKey, uint16_t newValue)
{
   return PointerRBTree_insert( (RBTree*)this, (void*)(size_t)newKey, (void*)(size_t)newValue);
}

/**
 * @return fhgfs_false if no element with the given key existed
 */
fhgfs_bool UInt16Map_erase(UInt16Map* this, const uint16_t eraseKey)
{
   return PointerRBTree_erase( (RBTree*)this, (const void*)(const size_t)eraseKey);
}

size_t UInt16Map_length(UInt16Map* this)
{
   return PointerRBTree_length( (RBTree*)this);
}


void UInt16Map_clear(UInt16Map* this)
{
   while(this->rbTree.length)
   {
      UInt16Map_erase(this, (uint16_t)(size_t)this->rbTree.treeroot.rootnode->key);
   }
}

#endif /* UINT16MAP_H_ */
