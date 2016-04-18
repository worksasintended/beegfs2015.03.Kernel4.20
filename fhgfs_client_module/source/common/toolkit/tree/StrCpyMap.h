#ifndef STRCPYMAP_H_
#define STRCPYMAP_H_

#include <common/toolkit/tree/PointerRBTree.h>
#include <common/toolkit/tree/PointerRBTreeIter.h>

struct StrCpyMapElem;
typedef struct StrCpyMapElem StrCpyMapElem;
struct StrCpyMap;
typedef struct StrCpyMap StrCpyMap;

struct StrCpyMapIter; // forward declaration of the iterator

static inline void StrCpyMap_init(StrCpyMap* this);
static inline StrCpyMap* StrCpyMap_construct(void);
static inline void StrCpyMap_uninit(StrCpyMap* this);
static inline void StrCpyMap_destruct(StrCpyMap* this);

static inline fhgfs_bool StrCpyMap_insert(StrCpyMap* this, const char* newKey,
   const char* newValue);
static inline fhgfs_bool StrCpyMap_erase(StrCpyMap* this, const char* eraseKey);
static inline size_t StrCpyMap_length(StrCpyMap* this);

static inline void StrCpyMap_clear(StrCpyMap* this);

extern struct StrCpyMapIter StrCpyMap_find(StrCpyMap* this, const char* searchKey);
extern struct StrCpyMapIter StrCpyMap_begin(StrCpyMap* this);

extern int compareStrCpyMapElems(const void* key1, const void* key2);


struct StrCpyMapElem
{
   RBTreeElem rbTreeElem;
};

struct StrCpyMap
{
   RBTree rbTree;
};


void StrCpyMap_init(StrCpyMap* this)
{
   PointerRBTree_init( (RBTree*)this, compareStrCpyMapElems);

   //this->rbTree.compareTreeElems = compareStrCpyMapElems;
}

StrCpyMap* StrCpyMap_construct(void)
{
   StrCpyMap* this = (StrCpyMap*)os_kmalloc(sizeof(*this) );

   StrCpyMap_init(this);

   return this;
}

void StrCpyMap_uninit(StrCpyMap* this)
{
   StrCpyMap_clear(this);

   PointerRBTree_uninit( (RBTree*)this);
}

void StrCpyMap_destruct(StrCpyMap* this)
{
   StrCpyMap_uninit(this);

   os_kfree(this);
}



/**
 * @return fhgfs_true if element was inserted, fhgfs_false if key already existed (in which case
 * nothing is changed)
 */
fhgfs_bool StrCpyMap_insert(StrCpyMap* this, const char* newKey, const char* newValue)
{
   size_t keyLen;
   char* keyCopy;
   size_t valueLen;
   char* valueCopy;
   fhgfs_bool insRes;

   // copy key
   keyLen = os_strlen(newKey)+1;
   keyCopy = (char*)os_kmalloc(keyLen);
   os_memcpy(keyCopy, newKey, keyLen);

   // copy value
   valueLen = os_strlen(newValue)+1;
   valueCopy = (char*)os_kmalloc(valueLen);
   os_memcpy(valueCopy, newValue, valueLen);

   insRes = PointerRBTree_insert( (RBTree*)this, keyCopy, valueCopy);
   if(!insRes)
   {
      // not inserted because the key already existed => free up the copies
      os_kfree(keyCopy);
      os_kfree(valueCopy);
   }

   return insRes;
}

/**
 * @return fhgfs_false if no element with the given key existed
 */
fhgfs_bool StrCpyMap_erase(StrCpyMap* this, const char* eraseKey)
{
   fhgfs_bool eraseRes;
   void* elemKey;
   void* elemValue;

   RBTreeElem* treeElem = _PointerRBTree_findElem( (RBTree*)this, eraseKey);
   if(!treeElem)
   { // element not found
      return fhgfs_false;
   }

   elemKey = treeElem->key;
   elemValue = treeElem->value;

   eraseRes = PointerRBTree_erase( (RBTree*)this, eraseKey);
   if(eraseRes)
   { // treeElem has been erased
      os_kfree(elemKey);
      os_kfree(elemValue);
   }

   return eraseRes;
}

size_t StrCpyMap_length(StrCpyMap* this)
{
   return PointerRBTree_length( (RBTree*)this);
}


void StrCpyMap_clear(StrCpyMap* this)
{
   while(this->rbTree.length)
   {
      StrCpyMap_erase(this, this->rbTree.treeroot.rootnode->key);
   }
}

#endif /*STRCPYMAP_H_*/
