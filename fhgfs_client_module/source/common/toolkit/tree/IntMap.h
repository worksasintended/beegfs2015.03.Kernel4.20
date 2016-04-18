#ifndef INTMAP_H_
#define INTMAP_H_

#include "PointerRBTree.h"
#include "PointerRBTreeIter.h"


/**
 * We assign the integer keys directly to the key pointers (i.e. no extra allocation involved,
 * but a lot of casting to convince gcc that we know what we're doing).
 * Values are just arbitrary pointers.
 */


struct IntMapElem;
typedef struct IntMapElem IntMapElem;
struct IntMap;
typedef struct IntMap IntMap;

struct IntMapIter; // forward declaration of the iterator

static inline void IntMap_init(IntMap* this);
static inline IntMap* IntMap_construct(void);
static inline void IntMap_uninit(IntMap* this);
static inline void IntMap_destruct(IntMap* this);

static inline fhgfs_bool IntMap_insert(IntMap* this, int newKey, char* newValue);
static inline fhgfs_bool IntMap_erase(IntMap* this, const int eraseKey);
static inline size_t IntMap_length(IntMap* this);

static inline void IntMap_clear(IntMap* this);

extern struct IntMapIter IntMap_find(IntMap* this, const int searchKey);
extern struct IntMapIter IntMap_begin(IntMap* this);

extern int compareIntMapElems(const void* key1, const void* key2);


struct IntMapElem
{
   RBTreeElem rbTreeElem;
};

struct IntMap
{
   RBTree rbTree;
};


void IntMap_init(IntMap* this)
{
   PointerRBTree_init( (RBTree*)this, compareIntMapElems);
}

IntMap* IntMap_construct(void)
{
   IntMap* this = (IntMap*)os_kmalloc(sizeof(*this) );

   IntMap_init(this);

   return this;
}

void IntMap_uninit(IntMap* this)
{
   // (nothing alloc'ed by this sub map type => nothing special here to be free'd)

   PointerRBTree_uninit( (RBTree*)this);
}

void IntMap_destruct(IntMap* this)
{
   IntMap_uninit(this);

   os_kfree(this);
}


/**
 * @param newValue just assigned, not copied.
 * @return fhgfs_true if element was inserted, fhgfs_false if key already existed (in which case
 * nothing is changed)
 */
fhgfs_bool IntMap_insert(IntMap* this, int newKey, char* newValue)
{
   return PointerRBTree_insert( (RBTree*)this, (void*)(size_t)newKey, newValue);
}

/**
 * @return fhgfs_false if no element with the given key existed
 */
fhgfs_bool IntMap_erase(IntMap* this, const int eraseKey)
{
   return PointerRBTree_erase( (RBTree*)this, (const void*)(const size_t)eraseKey);
}

size_t IntMap_length(IntMap* this)
{
   return PointerRBTree_length( (RBTree*)this);
}


void IntMap_clear(IntMap* this)
{
   while(this->rbTree.length)
   {
      IntMap_erase(this, (int)(size_t)this->rbTree.treeroot.rootnode->key);
   }
}

#endif /* INTMAP_H_ */
