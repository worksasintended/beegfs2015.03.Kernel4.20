#ifndef POINTERRBTREEITER_H_
#define POINTERRBTREEITER_H_

#include "PointerRBTree.h"

struct RBTreeIter;
typedef struct RBTreeIter RBTreeIter;

static inline void PointerRBTreeIter_init(
   RBTreeIter* this, RBTree* tree, RBTreeElem* treeElem);
static inline struct RBTreeIter* PointerRBTreeIter_construct(
   RBTree* tree, RBTreeElem* treeElem);
static inline void PointerRBTreeIter_uninit(RBTreeIter* this);
static inline void PointerRBTreeIter_destruct(RBTreeIter* this);

static inline fhgfs_bool PointerRBTreeIter_hasNext(RBTreeIter* this);
static inline void* PointerRBTreeIter_next(RBTreeIter* this);
static inline void* PointerRBTreeIter_key(RBTreeIter* this);
static inline void* PointerRBTreeIter_value(RBTreeIter* this);
static inline fhgfs_bool PointerRBTreeIter_end(RBTreeIter* this);

struct RBTreeIter
{
   RBTree* tree;
   RBTreeElem* treeElem;
};


void PointerRBTreeIter_init(RBTreeIter* this, RBTree* tree, RBTreeElem* treeElem)
{
   this->tree = tree;
   this->treeElem = treeElem;
}

RBTreeIter* PointerRBTreeIter_construct(RBTree* tree, RBTreeElem* treeElem)
{
   RBTreeIter* this = (RBTreeIter*)os_kmalloc(sizeof(RBTreeIter) );
   
   PointerRBTreeIter_init(this, tree, treeElem);
   
   return this;
}


void PointerRBTreeIter_uninit(RBTreeIter* this)
{
}

void PointerRBTreeIter_destruct(RBTreeIter* this)
{
   PointerRBTreeIter_uninit(this);
   
   os_kfree(this);
}

fhgfs_bool PointerRBTreeIter_hasNext(RBTreeIter* this)
{
   return (this->treeElem && fhgfs_RBTree_next( (RBNode*)this->treeElem) );
}

void* PointerRBTreeIter_next(RBTreeIter* this)
{
   this->treeElem = (RBTreeElem*)fhgfs_RBTree_next( (RBNode*)this->treeElem);
   
   return this->treeElem ? this->treeElem->value : NULL;
}

void* PointerRBTreeIter_key(RBTreeIter* this)
{
   return this->treeElem->key;
}

void* PointerRBTreeIter_value(RBTreeIter* this)
{
   return this->treeElem->value;
}

/**
 * Return true if the end of the iterator was reached
 */
fhgfs_bool PointerRBTreeIter_end(RBTreeIter* this)
{
   return (this->treeElem == NULL);
}

#endif /*POINTERRBTREEITER_H_*/
