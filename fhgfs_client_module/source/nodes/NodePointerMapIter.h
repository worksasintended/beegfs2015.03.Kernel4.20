#ifndef NODEPOINTERMAPITER_H_
#define NODEPOINTERMAPITER_H_

#include "NodePointerMap.h"


struct NodePointerMapIter;
typedef struct NodePointerMapIter NodePointerMapIter;


static inline void NodePointerMapIter_init(NodePointerMapIter* this, NodePointerMap* map,
   RBTreeElem* treeElem);
static inline struct NodePointerMapIter* NodePointerMapIter_construct(NodePointerMap* map,
   RBTreeElem* treeElem);
static inline void NodePointerMapIter_uninit(NodePointerMapIter* this);
static inline void NodePointerMapIter_destruct(NodePointerMapIter* this);
static inline fhgfs_bool NodePointerMapIter_hasNext(NodePointerMapIter* this);
static inline NodeReferencer* NodePointerMapIter_next(NodePointerMapIter* this);
static inline Node* NodePointerMapIter_key(NodePointerMapIter* this);
static inline NodeReferencer* NodePointerMapIter_value(NodePointerMapIter* this);
static inline fhgfs_bool NodePointerMapIter_end(NodePointerMapIter* this);


struct NodePointerMapIter
{
   RBTreeIter rbTreeIter;
};


void NodePointerMapIter_init(NodePointerMapIter* this, NodePointerMap* map, RBTreeElem* treeElem)
{
   PointerRBTreeIter_init( (RBTreeIter*)this, (RBTree*)map, (RBTreeElem*)treeElem);
}

NodePointerMapIter* NodePointerMapIter_construct(NodePointerMap* map, RBTreeElem* treeElem)
{
   NodePointerMapIter* this = (NodePointerMapIter*)os_kmalloc(sizeof(*this) );
   
   NodePointerMapIter_init(this, map, treeElem);
   
   return this;
}

void NodePointerMapIter_uninit(NodePointerMapIter* this)
{
   PointerRBTreeIter_uninit( (RBTreeIter*)this);
}

void NodePointerMapIter_destruct(NodePointerMapIter* this)
{
   NodePointerMapIter_uninit(this);
   
   os_kfree(this);
}

fhgfs_bool NodePointerMapIter_hasNext(NodePointerMapIter* this)
{
   return PointerRBTreeIter_hasNext( (RBTreeIter*)this);
}

NodeReferencer* NodePointerMapIter_next(NodePointerMapIter* this)
{
   return (NodeReferencer*)PointerRBTreeIter_next( (RBTreeIter*)this);
}

Node* NodePointerMapIter_key(NodePointerMapIter* this)
{
   return (Node*)PointerRBTreeIter_key( (RBTreeIter*)this);
}

NodeReferencer* NodePointerMapIter_value(NodePointerMapIter* this)
{
   return (NodeReferencer*)PointerRBTreeIter_value( (RBTreeIter*)this);
}

fhgfs_bool NodePointerMapIter_end(NodePointerMapIter* this)
{
   return PointerRBTreeIter_end( (RBTreeIter*)this);
}


#endif /*NODEPOINTERMAPITER_H_*/
