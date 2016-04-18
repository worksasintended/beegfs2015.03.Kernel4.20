#ifndef NODESTOREEX_H_
#define NODESTOREEX_H_

#include <app/log/Logger.h>
#include <app/App.h>
#include <common/toolkit/list/UInt16List.h>
#include <common/storage/StorageErrors.h>
#include <common/threading/Mutex.h>
#include <common/threading/Condition.h>
#include <common/toolkit/StringTk.h>
#include <common/nodes/Node.h>
#include <common/nodes/NodeList.h>
#include <common/nodes/TargetMapper.h>
#include <common/Common.h>
#include "NodeMap.h"
#include "NodeMapIter.h"
#include "NodePointerMap.h"
#include "NodePointerMapIter.h"
#include "NodeReferencer.h"


struct NodeStoreEx;
typedef struct NodeStoreEx NodeStoreEx;


extern void NodeStoreEx_init(NodeStoreEx* this, App* app, NodeType storeType);
extern NodeStoreEx* NodeStoreEx_construct(App* app, NodeType storeType);
extern void NodeStoreEx_uninit(NodeStoreEx* this);
extern void NodeStoreEx_destruct(NodeStoreEx* this);

extern fhgfs_bool NodeStoreEx_addOrUpdateNode(NodeStoreEx* this, Node** node);
extern Node* NodeStoreEx_referenceNode(NodeStoreEx* this, uint16_t id);
extern Node* NodeStoreEx_referenceRootNode(NodeStoreEx* this);
extern Node* NodeStoreEx_referenceNodeByTargetID(NodeStoreEx* this, uint16_t targetID,
   TargetMapper* targetMapper, FhgfsOpsErr* outErr);
extern void NodeStoreEx_releaseNode(NodeStoreEx* this, Node** node);
extern fhgfs_bool NodeStoreEx_deleteNode(NodeStoreEx* this, uint16_t nodeID);
extern unsigned NodeStoreEx_getSize(NodeStoreEx* this);

extern Node* NodeStoreEx_referenceFirstNode(NodeStoreEx* this);
extern Node* NodeStoreEx_referenceNextNode(NodeStoreEx* this, uint16_t nodeID);
extern Node* NodeStoreEx_referenceNextNodeAndReleaseOld(NodeStoreEx* this, Node* oldNode);

extern uint16_t NodeStoreEx_getRootNodeNumID(NodeStoreEx* this);
extern fhgfs_bool NodeStoreEx_setRootNodeNumID(NodeStoreEx* this, uint16_t nodeID,
   fhgfs_bool ignoreExistingRoot);

extern fhgfs_bool NodeStoreEx_waitForFirstNode(NodeStoreEx* this, int waitTimeoutMS);

extern void NodeStoreEx_syncNodes(NodeStoreEx* this, NodeList* masterList, UInt16List* outAddedIDs,
   UInt16List* outRemovedIDs, Node* localNode);


extern void __NodeStoreEx_handleNodeVersion(NodeStoreEx* this, Node* node);


// getters & setters
static inline fhgfs_bool NodeStoreEx_getValid(NodeStoreEx* this);
static inline NodeType NodeStoreEx_getStoreType(NodeStoreEx* this);


/**
 * This is the corresponding class for NodeStoreServers in userspace.
 */
struct NodeStoreEx
{
   App* app;

   Mutex* mutex;
   Condition* newNodeCond; // set when a new node is added to the store (or undeleted)

   NodeMap activeNodes; // keys nodeNumID, value: NodeReferencer*
   NodePointerMap deletedNodes; // key is node pointer (to avoid collision problems on num id reuse)

   uint16_t rootNodeID; // empty string if root node is unknown

   NodeType storeType; // will be applied to nodes on addOrUpdate()

   fhgfs_bool storeValid;
};


fhgfs_bool NodeStoreEx_getValid(NodeStoreEx* this)
{
   return this->storeValid;
}

NodeType NodeStoreEx_getStoreType(NodeStoreEx* this)
{
   return this->storeType;
}


#endif /*NODESTOREEX_H_*/
