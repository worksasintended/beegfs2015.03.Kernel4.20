#ifndef NODEMAP_H_
#define NODEMAP_H_

#include <common/toolkit/tree/PointerRBTree.h>
#include <common/toolkit/tree/PointerRBTreeIter.h>
#include <nodes/NodeReferencer.h>


// type definitions

struct NodeMapElem;
typedef struct NodeMapElem NodeMapElem;
struct NodeMap;
typedef struct NodeMap NodeMap;


// forward declarations

struct NodeMapIter;


static inline void NodeMap_init(NodeMap* this);
static inline NodeMap* NodeMap_construct(void);
static inline void NodeMap_uninit(NodeMap* this);
static inline void NodeMap_destruct(NodeMap* this);

static inline fhgfs_bool NodeMap_insert(NodeMap* this, uint16_t newKey, NodeReferencer* newValue);
static fhgfs_bool NodeMap_erase(NodeMap* this, uint16_t eraseKey);
static inline size_t NodeMap_length(NodeMap* this);

static inline void NodeMap_clear(NodeMap* this);

extern struct NodeMapIter NodeMap_find(NodeMap* this, uint16_t searchKey);
extern struct NodeMapIter NodeMap_begin(NodeMap* this);

extern int compareNodeMapElems(const void* key1, const void* key2);


struct NodeMapElem
{
   RBTreeElem rbTreeElem;
};

/**
 * Keys: uint16_t nodeNumIDs, values: NodeReferencer*
 */
struct NodeMap
{
   RBTree rbTree;
};


void NodeMap_init(NodeMap* this)
{
   PointerRBTree_init( (RBTree*)this, compareNodeMapElems);
}

NodeMap* NodeMap_construct(void)
{
   NodeMap* this = (NodeMap*)os_kmalloc(sizeof(*this) );

   NodeMap_init(this);

   return this;
}

void NodeMap_uninit(NodeMap* this)
{
   while(this->rbTree.length)
   {
      NodeMap_erase(this, (uint16_t)(size_t)this->rbTree.treeroot.rootnode->key);
   }

   PointerRBTree_uninit( (RBTree*)this);
}

void NodeMap_destruct(NodeMap* this)
{
   NodeMap_uninit(this);

   os_kfree(this);
}


fhgfs_bool NodeMap_insert(NodeMap* this, uint16_t newKey, NodeReferencer* newValue)
{
   fhgfs_bool insRes;

   insRes = PointerRBTree_insert( (RBTree*)this, (void*)(size_t)newKey, newValue);
   if(!insRes)
   {
      // not inserted because the key already existed
   }

   return insRes;
}

fhgfs_bool NodeMap_erase(NodeMap* this, uint16_t eraseKey)
{
   RBTreeElem* treeElem = _PointerRBTree_findElem( (RBTree*)this, (void*)(size_t)eraseKey);
   if(!treeElem)
   { // element not found
      return fhgfs_false;
   }

   return PointerRBTree_erase( (RBTree*)this, (void*)(size_t)eraseKey);
}

size_t NodeMap_length(NodeMap* this)
{
   return PointerRBTree_length( (RBTree*)this);
}


void NodeMap_clear(NodeMap* this)
{
   while(this->rbTree.length)
   {
      NodeMap_erase(this, (uint16_t)(size_t)this->rbTree.treeroot.rootnode->key);
   }
}


#endif /*NODEMAP_H_*/
