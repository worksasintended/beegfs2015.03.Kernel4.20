#ifndef NODEMAPITER_H_
#define NODEMAPITER_H_

#include "NodeMap.h"


struct NodeMapIter;
typedef struct NodeMapIter NodeMapIter;


static inline void NodeMapIter_init(NodeMapIter* this, NodeMap* map, RBTreeElem* treeElem);
static inline struct NodeMapIter* NodeMapIter_construct(NodeMap* map, RBTreeElem* treeElem);
static inline void NodeMapIter_uninit(NodeMapIter* this);
static inline void NodeMapIter_destruct(NodeMapIter* this);
static inline fhgfs_bool NodeMapIter_hasNext(NodeMapIter* this);
static inline NodeReferencer* NodeMapIter_next(NodeMapIter* this);
static inline uint16_t NodeMapIter_key(NodeMapIter* this);
static inline NodeReferencer* NodeMapIter_value(NodeMapIter* this);
static inline fhgfs_bool NodeMapIter_end(NodeMapIter* this);


struct NodeMapIter
{
   RBTreeIter rbTreeIter;
};


void NodeMapIter_init(NodeMapIter* this, NodeMap* map, RBTreeElem* treeElem)
{
   PointerRBTreeIter_init( (RBTreeIter*)this, (RBTree*)map, (RBTreeElem*)treeElem);
}

NodeMapIter* NodeMapIter_construct(NodeMap* map, RBTreeElem* treeElem)
{
   NodeMapIter* this = (NodeMapIter*)os_kmalloc(sizeof(*this) );
   
   NodeMapIter_init(this, map, treeElem);
   
   return this;
}

void NodeMapIter_uninit(NodeMapIter* this)
{
   PointerRBTreeIter_uninit( (RBTreeIter*)this);
}

void NodeMapIter_destruct(NodeMapIter* this)
{
   NodeMapIter_uninit(this);
   
   os_kfree(this);
}

fhgfs_bool NodeMapIter_hasNext(NodeMapIter* this)
{
   return PointerRBTreeIter_hasNext( (RBTreeIter*)this);
}

NodeReferencer* NodeMapIter_next(NodeMapIter* this)
{
   return (NodeReferencer*)PointerRBTreeIter_next( (RBTreeIter*)this);
}

uint16_t NodeMapIter_key(NodeMapIter* this)
{
   return (uint16_t)(size_t)PointerRBTreeIter_key( (RBTreeIter*)this);
}

NodeReferencer* NodeMapIter_value(NodeMapIter* this)
{
   return (NodeReferencer*)PointerRBTreeIter_value( (RBTreeIter*)this);
}

fhgfs_bool NodeMapIter_end(NodeMapIter* this)
{
   return PointerRBTreeIter_end( (RBTreeIter*)this);
}


#endif /*NODEMAPITER_H_*/
