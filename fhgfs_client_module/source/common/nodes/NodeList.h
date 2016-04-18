#ifndef NODELIST_H_
#define NODELIST_H_

#include <common/toolkit/list/PointerList.h>
#include <common/Common.h>
#include <common/nodes/Node.h>

struct NodeList;
typedef struct NodeList NodeList;

static inline void NodeList_init(NodeList* this);
static inline NodeList* NodeList_construct(void);
static inline void NodeList_uninit(NodeList* this);
static inline void NodeList_destruct(NodeList* this);
static inline void NodeList_append(NodeList* this, Node* node);
static inline void NodeList_removeHead(NodeList* this);
static inline size_t NodeList_length(NodeList* this);

struct NodeList
{
   struct PointerList pointerList;
};


void NodeList_init(NodeList* this)
{
   PointerList_init( (PointerList*)this);
}

struct NodeList* NodeList_construct(void)
{
   struct NodeList* this = (NodeList*)os_kmalloc(sizeof(*this) );

   NodeList_init(this);

   return this;
}

void NodeList_uninit(NodeList* this)
{
   PointerList_uninit( (PointerList*)this);
}

void NodeList_destruct(NodeList* this)
{
   NodeList_uninit(this);

   os_kfree(this);
}

void NodeList_append(NodeList* this, Node* node)
{
   PointerList_append( (PointerList*)this, node);
}

void NodeList_removeHead(NodeList* this)
{
   PointerList_removeHead( (PointerList*)this);
}

static inline size_t NodeList_length(NodeList* this)
{
   return PointerList_length( (PointerList*)this);
}

#endif /* NODELIST_H_ */
