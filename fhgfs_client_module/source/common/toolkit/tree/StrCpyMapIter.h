#ifndef STRCPYMAPITER_H_
#define STRCPYMAPITER_H_

#include "StrCpyMap.h"

struct StrCpyMapIter;
typedef struct StrCpyMapIter StrCpyMapIter;

static inline void StrCpyMapIter_init(StrCpyMapIter* this, StrCpyMap* map, RBTreeElem* treeElem);
static inline struct StrCpyMapIter* StrCpyMapIter_construct(StrCpyMap* map, RBTreeElem* treeElem);
static inline void StrCpyMapIter_uninit(StrCpyMapIter* this);
static inline void StrCpyMapIter_destruct(StrCpyMapIter* this);
static inline fhgfs_bool StrCpyMapIter_hasNext(StrCpyMapIter* this);
static inline char* StrCpyMapIter_next(StrCpyMapIter* this);
static inline char* StrCpyMapIter_key(StrCpyMapIter* this);
static inline char* StrCpyMapIter_value(StrCpyMapIter* this);
static inline fhgfs_bool StrCpyMapIter_end(StrCpyMapIter* this);

struct StrCpyMapIter
{
   RBTreeIter rbTreeIter;
};

void StrCpyMapIter_init(StrCpyMapIter* this, StrCpyMap* map, RBTreeElem* treeElem)
{
   PointerRBTreeIter_init( (RBTreeIter*)this, (RBTree*)map, (RBTreeElem*)treeElem);
}

StrCpyMapIter* StrCpyMapIter_construct(StrCpyMap* map, RBTreeElem* treeElem)
{
   StrCpyMapIter* this = (StrCpyMapIter*)os_kmalloc(sizeof(StrCpyMapIter) );
   
   StrCpyMapIter_init(this, map, treeElem);
   
   return this;
}

void StrCpyMapIter_uninit(StrCpyMapIter* this)
{
   PointerRBTreeIter_uninit( (RBTreeIter*)this);
}

void StrCpyMapIter_destruct(StrCpyMapIter* this)
{
   StrCpyMapIter_uninit(this);
   
   os_kfree(this);
}

fhgfs_bool StrCpyMapIter_hasNext(StrCpyMapIter* this)
{
   return PointerRBTreeIter_hasNext( (RBTreeIter*)this);
}

char* StrCpyMapIter_next(StrCpyMapIter* this)
{
   return (char*)PointerRBTreeIter_next( (RBTreeIter*)this);
}

char* StrCpyMapIter_key(StrCpyMapIter* this)
{
   return (char*)PointerRBTreeIter_key( (RBTreeIter*)this);
}

char* StrCpyMapIter_value(StrCpyMapIter* this)
{
   return (char*)PointerRBTreeIter_value( (RBTreeIter*)this);
}

fhgfs_bool StrCpyMapIter_end(StrCpyMapIter* this)
{
   return PointerRBTreeIter_end( (RBTreeIter*)this);
}

#endif /*STRCPYMAPITER_H_*/
