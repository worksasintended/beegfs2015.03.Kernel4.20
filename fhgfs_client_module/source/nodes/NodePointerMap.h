#ifndef NODEPOINTERMAP_H_
#define NODEPOINTERMAP_H_

#include <common/toolkit/tree/PointerRBTree.h>
#include <common/toolkit/tree/PointerRBTreeIter.h>
#include <nodes/NodeReferencer.h>


// type definitions

struct NodePointerMapElem;
typedef struct NodePointerMapElem NodePointerMapElem;
struct NodePointerMap;
typedef struct NodePointerMap NodePointerMap;


// forward declarations

struct NodePointerMapIter;


static inline void NodePointerMap_init(NodePointerMap* this);
static inline NodePointerMap* NodePointerMap_construct(void);
static inline void NodePointerMap_uninit(NodePointerMap* this);
static inline void NodePointerMap_destruct(NodePointerMap* this);

static inline fhgfs_bool NodePointerMap_insert(NodePointerMap* this, Node* newKey,
   NodeReferencer* newValue);
static fhgfs_bool NodePointerMap_erase(NodePointerMap* this, Node* eraseKey);
static inline size_t NodePointerMap_length(NodePointerMap* this);

static inline void NodePointerMap_clear(NodePointerMap* this);

extern struct NodePointerMapIter NodePointerMap_find(NodePointerMap* this, Node* searchKey);
extern struct NodePointerMapIter NodePointerMap_begin(NodePointerMap* this);

extern int compareNodePointerMapElems(const void* key1, const void* key2);


struct NodePointerMapElem
{
   RBTreeElem rbTreeElem;
};

/**
 * Keys: Node*, values: NodeReferencer*
 */
struct NodePointerMap
{
   RBTree rbTree;
};


void NodePointerMap_init(NodePointerMap* this)
{
   PointerRBTree_init( (RBTree*)this, compareNodePointerMapElems);
}

NodePointerMap* NodePointerMap_construct(void)
{
   NodePointerMap* this = (NodePointerMap*)os_kmalloc(sizeof(*this) );

   NodePointerMap_init(this);

   return this;
}

void NodePointerMap_uninit(NodePointerMap* this)
{
   while(this->rbTree.length)
   {
      NodePointerMap_erase(this, this->rbTree.treeroot.rootnode->key);
   }

   PointerRBTree_uninit( (RBTree*)this);
}

void NodePointerMap_destruct(NodePointerMap* this)
{
   NodePointerMap_uninit(this);

   os_kfree(this);
}


fhgfs_bool NodePointerMap_insert(NodePointerMap* this, Node* newKey, NodeReferencer* newValue)
{
   fhgfs_bool insRes;

   insRes = PointerRBTree_insert( (RBTree*)this, newKey, newValue);
   if(!insRes)
   {
      // not inserted because the key already existed
   }

   return insRes;
}

fhgfs_bool NodePointerMap_erase(NodePointerMap* this, Node* eraseKey)
{
   RBTreeElem* treeElem = _PointerRBTree_findElem( (RBTree*)this, eraseKey);
   if(!treeElem)
   { // element not found
      return fhgfs_false;
   }

   return PointerRBTree_erase( (RBTree*)this, eraseKey);
}

size_t NodePointerMap_length(NodePointerMap* this)
{
   return PointerRBTree_length( (RBTree*)this);
}


void NodePointerMap_clear(NodePointerMap* this)
{
   while(this->rbTree.length)
   {
      NodePointerMap_erase(this, this->rbTree.treeroot.rootnode->key);
   }
}


#endif /*NODEPOINTERMAP_H_*/
