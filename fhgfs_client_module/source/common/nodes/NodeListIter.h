#ifndef NODELISTITER_H_
#define NODELISTITER_H_

#include <common/toolkit/list/PointerListIter.h>
#include <common/Common.h>
#include <common/nodes/Node.h>
#include "NodeList.h"

struct NodeListIter;
typedef struct NodeListIter NodeListIter;

static inline void NodeListIter_init(NodeListIter* this, NodeList* list);
static inline NodeListIter* NodeListIter_construct(NodeList* list);
static inline void NodeListIter_uninit(NodeListIter* this);
static inline void NodeListIter_destruct(NodeListIter* this);
static inline fhgfs_bool NodeListIter_hasNext(NodeListIter* this);
static inline void NodeListIter_next(NodeListIter* this);
static inline Node* NodeListIter_value(NodeListIter* this);
static inline fhgfs_bool NodeListIter_end(NodeListIter* this);


struct NodeListIter
{
   PointerListIter pointerListIter;
};


void NodeListIter_init(NodeListIter* this, NodeList* list)
{
   PointerListIter_init( (PointerListIter*)this, (PointerList*)list); 
}

NodeListIter* NodeListIter_construct(NodeList* list)
{
   NodeListIter* this = (NodeListIter*)os_kmalloc(
      sizeof(NodeListIter) );
   
   NodeListIter_init(this, list);
   
   return this;
}

void NodeListIter_uninit(NodeListIter* this)
{
   PointerListIter_uninit( (PointerListIter*)this);
}

void NodeListIter_destruct(NodeListIter* this)
{
   NodeListIter_uninit(this);
   
   os_kfree(this);
}

fhgfs_bool NodeListIter_hasNext(NodeListIter* this)
{
   return PointerListIter_hasNext( (PointerListIter*)this);
}

void NodeListIter_next(NodeListIter* this)
{
   PointerListIter_next( (PointerListIter*)this);
}

Node* NodeListIter_value(NodeListIter* this)
{
   return (Node*)PointerListIter_value( (PointerListIter*)this);
}

fhgfs_bool NodeListIter_end(NodeListIter* this)
{
   return PointerListIter_end( (PointerListIter*)this);
}



#endif /* NODELISTITER_H_ */
