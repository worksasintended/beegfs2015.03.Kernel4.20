#ifndef NODEREFERENCER_H_
#define NODEREFERENCER_H_

#include <common/Common.h>
#include <common/toolkit/ObjectReferencer.h>
#include <common/nodes/Node.h>

/*
 * Note: Be careful with sub-classing NodeReferencer, because it deletes the referencedObject
 * by calling Node_destruct. You better derive directly from ObjectReferencer.
 */

struct NodeReferencer;
typedef struct NodeReferencer NodeReferencer;

static inline void NodeReferencer_init(NodeReferencer* this, Node* referencedObject,
   fhgfs_bool ownReferencedObject);
static inline NodeReferencer* NodeReferencer_construct(Node* referencedObject,
   fhgfs_bool ownReferencedObject);
static inline void NodeReferencer_uninit(NodeReferencer* this);
static inline void NodeReferencer_destruct(NodeReferencer* this);

static inline Node* NodeReferencer_reference(NodeReferencer* this);
static inline int NodeReferencer_release(NodeReferencer* this, App* app);
static inline Node* NodeReferencer_getReferencedObject(NodeReferencer* this);

// getters & setters
static inline int NodeReferencer_getRefCount(NodeReferencer* this);


struct NodeReferencer
{
   ObjectReferencer objectReferencer;
   
   fhgfs_bool ownReferencedObject;
};


/**
 * @param ownReferencedObject if set to fhgfs_true, this object owns the referencedObject
 * (so it will destruct the referencedObject when itself is being destructed)
 */
void NodeReferencer_init(NodeReferencer* this, Node* referencedObject, fhgfs_bool ownReferencedObject)
{
   ObjectReferencer_init( (ObjectReferencer*)this, referencedObject);
   
   this->ownReferencedObject = ownReferencedObject;
}

/**
 * @param ownReferencedObject if set to fhgfs_true, this object owns the referencedObject
 * (so it will destruct the referencedObject when itself is being destructed)
 */
NodeReferencer* NodeReferencer_construct(Node* referencedObject, fhgfs_bool ownReferencedObject)
{
   NodeReferencer* this = (NodeReferencer*)os_kmalloc(sizeof(NodeReferencer) );
   
   NodeReferencer_init(this, referencedObject, ownReferencedObject);
   
   return this;
}

void NodeReferencer_uninit(NodeReferencer* this)
{
   if(this->ownReferencedObject)
   {
      void* referencedObject = ObjectReferencer_getReferencedObject( (ObjectReferencer*)this);
      Node_destruct( (Node*)referencedObject);
   }
   
   
   ObjectReferencer_uninit( (ObjectReferencer*)this);
}

void NodeReferencer_destruct(NodeReferencer* this)
{
   NodeReferencer_uninit(this);
   
   os_kfree(this);
}

Node* NodeReferencer_reference(NodeReferencer* this)
{
   return (Node*)ObjectReferencer_reference( (ObjectReferencer*)this);
}

int NodeReferencer_release(NodeReferencer* this, App* app)
{
   return ObjectReferencer_release( (ObjectReferencer*)this, app);
}

Node* NodeReferencer_getReferencedObject(NodeReferencer* this)
{
   return (Node*)ObjectReferencer_getReferencedObject( (ObjectReferencer*)this);
}

int NodeReferencer_getRefCount(NodeReferencer* this)
{
   return ObjectReferencer_getRefCount( (ObjectReferencer*)this);
}

#endif /*NODEREFERENCER_H_*/
