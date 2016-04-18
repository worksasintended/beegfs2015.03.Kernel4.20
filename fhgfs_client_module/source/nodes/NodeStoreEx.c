#include <common/net/sock/NetworkInterfaceCard.h>
#include <common/nodes/NodeListIter.h>
#include <common/system/System.h>
#include <common/toolkit/VersionTk.h>
#include "NodeStoreEx.h"

#define NODESTORE_WARN_REFNUM 2000

/**
 * @param storeType will be applied to nodes on addOrUpdate()
 */
void NodeStoreEx_init(NodeStoreEx* this, App* app, NodeType storeType)
{
   this->storeValid = fhgfs_true;

   this->app = app;

   this->mutex = Mutex_construct();
   this->newNodeCond = Condition_construct();

   NodeMap_init(&this->activeNodes);
   NodePointerMap_init(&this->deletedNodes);

   this->rootNodeID = 0; // 0 means undefined/invalid

   this->storeType = storeType;
}

NodeStoreEx* NodeStoreEx_construct(App* app, NodeType storeType)
{
   NodeStoreEx* this = (NodeStoreEx*)os_kmalloc(sizeof(*this) );

   NodeStoreEx_init(this, app, storeType);

   return this;
}

void NodeStoreEx_uninit(NodeStoreEx* this)
{
   NodeMapIter iterActive;
   NodePointerMapIter iterDeleted;

   // delete activeNodes
   for(iterActive = NodeMap_begin(&this->activeNodes);
       !NodeMapIter_end(&iterActive);
       NodeMapIter_next(&iterActive) )
   {
      NodeReferencer_destruct(NodeMapIter_value(&iterActive) );
   }

   NodeMapIter_uninit(&iterActive);
   NodeMap_clear(&this->activeNodes);

   // delete deletedNodes
   for(iterDeleted = NodePointerMap_begin(&this->deletedNodes);
       !NodePointerMapIter_end(&iterDeleted);
       NodePointerMapIter_next(&iterDeleted) )
   {
      NodeReferencer_destruct(NodePointerMapIter_value(&iterDeleted) );
   }

   NodePointerMapIter_uninit(&iterDeleted);
   NodePointerMap_clear(&this->deletedNodes);


   NodeMap_uninit(&this->activeNodes);
   NodePointerMap_uninit(&this->deletedNodes);

   Condition_destruct(this->newNodeCond);
   Mutex_destruct(this->mutex);
}

void NodeStoreEx_destruct(NodeStoreEx* this)
{
   NodeStoreEx_uninit(this);

   os_kfree(this);
}


/**
 * @param node belongs to the store after calling this method; this method will set (*node=NULL);
 * so do not free it and don't use it any more afterwards (reference it from this store if you need
 * it)
 * @return fhgfs_true if the node was not in the store yet, fhgfs_false otherwise
 */
fhgfs_bool NodeStoreEx_addOrUpdateNode(NodeStoreEx* this, Node** node)
{
   const char* logContext = __func__;
   Logger* log = App_getLogger(this->app);

   uint16_t nodeNumID = Node_getNumID(*node);
   char* nodeID = Node_getID(*node);

   NodeMapIter iterActive;
   fhgfs_bool isNodeActive;

   // check if numeric ID is defined

   if(unlikely(!nodeNumID) )
   { // undefined numeric ID should never happen
      Logger_logErrFormatted(log, logContext,
         "Rejecting node with undefined numeric ID: %s; Type: %s",
         nodeID, Node_nodeTypeToStr(this->storeType) );

      Node_destruct(*node);
      *node = NULL;
      return fhgfs_false;
   }

   // sanity check: test if bit index 0 (dummy feature flag) is set
   BEEGFS_BUG_ON_DEBUG(!BitStore_getBit(Node_getNodeFeatures(*node), 0),
      "dummy feature flag not set");

   Mutex_lock(this->mutex); // L O C K

   // is node in any of the stores already?

   iterActive = NodeMap_find(&this->activeNodes, nodeNumID);

   isNodeActive = !NodeMapIter_end(&iterActive);

   if(isNodeActive)
   { // node was in the store already => update it
      NodeReferencer* nodeRefer = NodeMapIter_value(&iterActive);
      Node* nodeRef = NodeReferencer_getReferencedObject(nodeRefer);

      if(unlikely(os_strcmp(nodeID, Node_getID(nodeRef) ) ) )
      { // bad: numeric ID collision for two different node string IDs
         Logger_logErrFormatted(log, logContext,
            "Numeric ID collision for two different string IDs: %s / %s; Type: %s",
            nodeID, Node_getID(nodeRef), Node_nodeTypeToStr(this->storeType) );
      }
      else
      { // update heartbeat time of existing node
         Node_updateLastHeartbeatT(nodeRef);
         Node_setFhgfsVersion(nodeRef, Node_getFhgfsVersion(*node) );
         Node_updateInterfaces(nodeRef, Node_getPortUDP(*node), Node_getPortTCP(*node),
            Node_getNicList(*node) );
         Node_updateFeatureFlagsThreadSafe(nodeRef, Node_getNodeFeatures(*node) );
      }

      Node_destruct(*node);
   }
   else
   { // node is not currently active => insert it
      NodeReferencer* nodeRefer = NodeReferencer_construct(*node, fhgfs_true);

      NodeMap_insert(&this->activeNodes, nodeNumID, nodeRefer);

      #ifdef BEEGFS_DEBUG
      // check whether this node type and store type differ
      if( (Node_getNodeType(*node) != NODETYPE_Invalid) &&
          (Node_getNodeType(*node) != this->storeType) )
      {
         Logger_logErrFormatted(log, logContext,
            "Node type and store type differ. Node: %s %s; Store: %s",
            Node_getNodeTypeStr(*node), nodeID, Node_nodeTypeToStr(this->storeType) );
      }
      #endif // BEEGFS_DEBUG

      Node_setIsActive(*node, fhgfs_true);
      Node_setNodeType(*node, this->storeType);

      __NodeStoreEx_handleNodeVersion(this, *node);

      Condition_broadcast(this->newNodeCond);
   }

   NodeMapIter_uninit(&iterActive);

   Mutex_unlock(this->mutex); // U N L O C K

   *node = NULL;

   return !isNodeActive;
}

/**
 * Note: remember to call releaseNode()
 *
 * @return NULL if no such node exists
 */
Node* NodeStoreEx_referenceNode(NodeStoreEx* this, uint16_t id)
{
   Logger* log = App_getLogger(this->app);
   Node* node = NULL;
   NodeMapIter iter;

   // check for invalid id 0
   #ifdef BEEGFS_DEBUG
   if(!id)
   {
      Logger_log(log, Log_CRITICAL, __func__, "BUG?: Attempt to reference numeric node ID '0'");
      dump_stack();
   }
   #endif // BEEGFS_DEBUG

   IGNORE_UNUSED_VARIABLE(log);


   Mutex_lock(this->mutex); // L O C K

   iter = NodeMap_find(&this->activeNodes, id);
   if(likely(!NodeMapIter_end(&iter) ) )
   { // found it
      NodeReferencer* nodeRefer = NodeMapIter_value(&iter);
      node = NodeReferencer_reference(nodeRefer);

      // check for unusually high reference count
      #ifdef BEEGFS_DEBUG
      if(NodeReferencer_getRefCount(nodeRefer) > NODESTORE_WARN_REFNUM)
         Logger_logFormatted(log, Log_CRITICAL, __func__,
            "WARNING: Lots of references to node (=> leak?): %s %s; ref count: %d",
            Node_getNodeTypeStr(node), Node_getID(node),
            (int)NodeReferencer_getRefCount(nodeRefer) );
      #endif // BEEGFS_DEBUG
   }

   NodeMapIter_uninit(&iter);

   Mutex_unlock(this->mutex); // U N L O C K

   return node;
}

/**
 * Note: remember to call releaseNode()
 *
 * @return NULL if no such node exists
 */
Node* NodeStoreEx_referenceRootNode(NodeStoreEx* this)
{
   Node* node = NULL;
   NodeMapIter iter;

   Mutex_lock(this->mutex); // L O C K

   iter = NodeMap_find(&this->activeNodes, this->rootNodeID);
   if(likely(!NodeMapIter_end(&iter) ) )
   { // found it
      NodeReferencer* nodeRefer = NodeMapIter_value(&iter);
      node = NodeReferencer_reference(nodeRefer);
   }

   NodeMapIter_uninit(&iter);

   Mutex_unlock(this->mutex); // U N L O C K

   return node;
}

/**
 * This is a helper to have only one call for the typical targetMapper->getNodeID() and following
 * referenceNode() calls.
 *
 * Note: remember to call releaseNode().
 *
 * @param targetMapper where to resolve the given targetID
 * @param outErr will be set to FhgfsOpsErr_UNKNOWNNODE, _UNKNOWNTARGET, _SUCCESS (may be NULL)
 * @return NULL if targetID is not mapped or if the mapped node does not exist in the store.
 */
Node* NodeStoreEx_referenceNodeByTargetID(NodeStoreEx* this, uint16_t targetID,
   TargetMapper* targetMapper, FhgfsOpsErr* outErr)
{
   uint16_t nodeID;
   Node* node;

   nodeID = TargetMapper_getNodeID(targetMapper, targetID);
   if(!nodeID)
   {
      SAFE_ASSIGN(outErr, FhgfsOpsErr_UNKNOWNTARGET);
      return NULL;
   }

   node = NodeStoreEx_referenceNode(this, nodeID);

   if(!node)
   {
      SAFE_ASSIGN(outErr, FhgfsOpsErr_UNKNOWNNODE);
      return NULL;
   }

   SAFE_ASSIGN(outErr, FhgfsOpsErr_SUCCESS);
   return node;
}

/**
 * @param node double-pointer, because this method sets *node=NULL to prevent further usage of
 * of the pointer after release
 */
void NodeStoreEx_releaseNode(NodeStoreEx* this, Node** node)
{
   uint16_t nodeID = Node_getNumID(*node);
   NodePointerMapIter iterDeleted;
   NodeMapIter iterActive;

   Mutex_lock(this->mutex); // L O C K

   /* note: we must check deletedNodes here first, in case the deleted numID has been re-used
      by the mgmtd already (in which case we could release the wrong node otherwise) */

   // search in deleted map

   iterDeleted = NodePointerMap_find(&this->deletedNodes, *node);
   if(unlikely(!NodePointerMapIter_end(&iterDeleted) ) )
   { // node marked for deletion => check refCount
      NodeReferencer* nodeRefer = NodePointerMapIter_value(&iterDeleted);

      int refCount = NodeReferencer_release(nodeRefer, this->app);
      if(!refCount)
      { // execute delayed deletion
         NodePointerMap_erase(&this->deletedNodes, *node);
         NodeReferencer_destruct(nodeRefer);
      }

      NodePointerMapIter_uninit(&iterDeleted);

      goto unlock_and_exit;
   }

   NodePointerMapIter_uninit(&iterDeleted);

   // search in active map

   iterActive = NodeMap_find(&this->activeNodes, nodeID);
   if(likely(!NodeMapIter_end(&iterActive) ) )
   { // node active => just decrease refCount
      NodeReferencer* nodeRefer = NodeMapIter_value(&iterActive);
      NodeReferencer_release(nodeRefer, this->app);
   }

   NodeMapIter_uninit(&iterActive);


unlock_and_exit:

   Mutex_unlock(this->mutex); // U N L O C K

   /* make sure noone can access the given node pointer anymore after release (just to avoid
      human errors) */
   *node = NULL;
}

/**
 * @return true if node existed as active node
 */
fhgfs_bool NodeStoreEx_deleteNode(NodeStoreEx* this, uint16_t nodeID)
{
   const char* logContext = __func__;
   Logger* log = App_getLogger(this->app);

   fhgfs_bool nodeWasActive = fhgfs_false;

   NodeMapIter iter;

   #ifdef BEEGFS_DEBUG
   if(unlikely(!nodeID) ) // should never happen
      Logger_logFormatted(log, Log_CRITICAL, logContext, "Called with invalid node ID '0'");
   #endif // BEEGFS_DEBUG

   IGNORE_UNUSED_VARIABLE(logContext);
   IGNORE_UNUSED_VARIABLE(log);


   Mutex_lock(this->mutex); // L O C K

   iter = NodeMap_find(&this->activeNodes, nodeID);
   if(!NodeMapIter_end(&iter) )
   {
      NodeReferencer* nodeRefer = NodeMapIter_value(&iter);
      Node* node = NodeReferencer_getReferencedObject(nodeRefer);

      if(NodeReferencer_getRefCount(nodeRefer) )
      { // node is referenced => move to deletedNodes
         NodePointerMap_insert(&this->deletedNodes, node, nodeRefer);
         NodeMap_erase(&this->activeNodes, nodeID);

         Node_setIsActive(node, fhgfs_false);
      }
      else
      { // no references => delete immediately
         NodeMap_erase(&this->activeNodes, nodeID);
         NodeReferencer_destruct(nodeRefer);
      }

      nodeWasActive = fhgfs_true;
   }

   NodeMapIter_uninit(&iter);

   Mutex_unlock(this->mutex); // U N L O C K

   return nodeWasActive;
}

unsigned NodeStoreEx_getSize(NodeStoreEx* this)
{
   unsigned nodesSize;

   Mutex_lock(this->mutex);

   nodesSize = NodeMap_length(&this->activeNodes);

   Mutex_unlock(this->mutex);

   return nodesSize;
}

/**
 * This is used to iterate over all stored nodes.
 * Start with this and then use referenceNextNode() until it returns NULL.
 *
 * Note: remember to call releaseNode()
 *
 * @return can be NULL
 */
Node* NodeStoreEx_referenceFirstNode(NodeStoreEx* this)
{
   Node* resultNode = NULL;
   NodeMapIter iter;

   Mutex_lock(this->mutex); // L O C K

   iter = NodeMap_begin(&this->activeNodes);
   if(!NodeMapIter_end(&iter) )
   {
      NodeReferencer* nodeRefer = NodeMapIter_value(&iter);
      resultNode = NodeReferencer_reference(nodeRefer);
   }

   NodeMapIter_uninit(&iter);

   Mutex_unlock(this->mutex); // U N L O C K

   return resultNode;
}

/**
 * Note: remember to call releaseNode()
 *
 * @return NULL if nodeID was the last node
 */
Node* NodeStoreEx_referenceNextNode(NodeStoreEx* this, uint16_t nodeID)
{
   Node* resultNode;
   fhgfs_bool nodeIDexisted = fhgfs_true;

   NodeMapIter iter;

   Mutex_lock(this->mutex); // L O C K

   // find next by inserting nodeID param (if it didn't exist already) and moving
   // the iterator to the next node after nodeID

   iter = NodeMap_find(&this->activeNodes, nodeID);
   if(NodeMapIter_end(&iter) )
   { // nodeID doesn't exist => insert it
      NodeMap_insert(&this->activeNodes, nodeID, NULL);
      nodeIDexisted = fhgfs_false;

      // find inserted node
      NodeMapIter_uninit(&iter);
      iter = NodeMap_find(&this->activeNodes, nodeID);
   }

   NodeMapIter_next(&iter); // move to next


   if(NodeMapIter_end(&iter) )
      resultNode = NULL;
   else
   { // next node exists => reference it
      NodeReferencer* nodeRefer = NodeMapIter_value(&iter);
      resultNode = NodeReferencer_reference(nodeRefer);
   }

   if(!nodeIDexisted)
      NodeMap_erase(&this->activeNodes, nodeID);


   NodeMapIter_uninit(&iter);

   Mutex_unlock(this->mutex); // U N L O C K

   return resultNode;
}

/**
 * Releases the old node and then references the next node.
 *
 * Note: Make sure to call this until it returns NULL (so the last node is released properly) or
 * remember to release the node yourself if you don't iterate to the end.
 *
 * @return NULL if oldNode was the last node
 */
Node* NodeStoreEx_referenceNextNodeAndReleaseOld(NodeStoreEx* this, Node* oldNode)
{
   uint16_t oldNodeID = Node_getNumID(oldNode);

   NodeStoreEx_releaseNode(this, &oldNode);

   return NodeStoreEx_referenceNextNode(this, oldNodeID);
}

/**
 * @return 0 if no root node is known
 */
uint16_t NodeStoreEx_getRootNodeNumID(NodeStoreEx* this)
{
   uint16_t rootNodeID;

   Mutex_lock(this->mutex); // L O C K

   rootNodeID = this->rootNodeID;

   Mutex_unlock(this->mutex); // U N L O C K

   return rootNodeID;
}

/**
 * Set internal root node ID.
 *
 * @return fhgfs_false if the new ID was rejected (e.g. because we already had an id set and
 * ignoreExistingRoot was false).
 */
fhgfs_bool NodeStoreEx_setRootNodeNumID(NodeStoreEx* this, uint16_t nodeID,
   fhgfs_bool ignoreExistingRoot)
{
   fhgfs_bool setRootRes = fhgfs_true;

   // don't allow invalid id 0 (if not forced to do so)
   if(!nodeID && !ignoreExistingRoot)
      return fhgfs_false;

   Mutex_lock(this->mutex); // L O C K

   if(!this->rootNodeID)
   { // rootID empty => set the new root
      this->rootNodeID = nodeID;
   }
   else
   if(!ignoreExistingRoot)
   { // root defined already, reject new root
      setRootRes = fhgfs_false;
   }
   else
   { // root defined already, but shall be ignored
      this->rootNodeID = nodeID;
   }

   Mutex_unlock(this->mutex); // U N L O C K

   return setRootRes;
}

/**
 * Waits for the first node that is added to the store.
 *
 * @return true when a new node was added to the store before the timeout expired
 */
fhgfs_bool NodeStoreEx_waitForFirstNode(NodeStoreEx* this, int waitTimeoutMS)
{
   fhgfs_bool retVal;
   int elapsedMS = 0;

   Time startT;

   Time_init(&startT);

   Mutex_lock(this->mutex); // L O C K

   while(!NodeMap_length(&this->activeNodes) && (elapsedMS < waitTimeoutMS) )
   {
      Condition_timedwait(this->newNodeCond, this->mutex, waitTimeoutMS - elapsedMS);

      elapsedMS = Time_elapsedMS(&startT);
   }

   retVal = (NodeMap_length(&this->activeNodes) > 0);

   Mutex_unlock(this->mutex); // U N L O C K

   Time_uninit(&startT);

   return retVal;
}


/**
 * @param masterList must be ordered; contained nodes will be removed and may no longer be
 * accessed after calling this method.
 * @param appLocalNode just what you get from app->getLocalNode(), to determine NIC capabilities
 */
void NodeStoreEx_syncNodes(NodeStoreEx* this, NodeList* masterList, UInt16List* outAddedIDs,
   UInt16List* outRemovedIDs, Node* appLocalNode)
{
   // Note: We have two phases here:
   //    Phase 1 (locked): Identify added/removed nodes.
   //    Phase 2 (unlocked): Add/remove nodes from store.
   // This separation is required to not break compatibility with virtual overwritten add/remove
   // methods in derived classes (e.g. fhgfs_mgmtd).


   // P H A S E 1 (Identify added/removed nodes.)

   NodeMapIter activeIter;
   NodeListIter masterIter;

   UInt16ListIter removedIDsIter;
   NodeList addLaterNodes; // nodes to be added in phase 2
   NodeListIter addLaterIter;

   NicListCapabilities localNicCaps;

   NodeList_init(&addLaterNodes);


   Mutex_lock(this->mutex); // L O C K

   activeIter = NodeMap_begin(&this->activeNodes);
   NodeListIter_init(&masterIter, masterList);

   while(!NodeMapIter_end(&activeIter) && !NodeListIter_end(&masterIter) )
   {
      uint16_t currentActive = NodeMapIter_key(&activeIter);
      uint16_t currentMaster = Node_getNumID(NodeListIter_value(&masterIter) );

      if(currentMaster < currentActive)
      { // currentMaster is added
         UInt16List_append(outAddedIDs, currentMaster);
         NodeList_append(&addLaterNodes, NodeListIter_value(&masterIter) );

         NodeListIter_uninit(&masterIter);
         NodeList_removeHead(masterList);
         NodeListIter_init(&masterIter, masterList);
      }
      else
      if(currentActive < currentMaster)
      { // currentActive is removed
         UInt16List_append(outRemovedIDs, currentActive);
         NodeMapIter_next(&activeIter);
      }
      else
      { // node unchanged
         NodeList_append(&addLaterNodes, NodeListIter_value(&masterIter) );

         NodeMapIter_next(&activeIter);

         NodeListIter_uninit(&masterIter);
         NodeList_removeHead(masterList);
         NodeListIter_init(&masterIter, masterList);
      }
   }

   // remaining masterList nodes are added
   while(!NodeListIter_end(&masterIter) )
   {
      uint16_t currentMaster = Node_getNumID(NodeListIter_value(&masterIter) );

      UInt16List_append(outAddedIDs, currentMaster);
      NodeList_append(&addLaterNodes, NodeListIter_value(&masterIter) );

      NodeListIter_uninit(&masterIter);
      NodeList_removeHead(masterList);
      NodeListIter_init(&masterIter, masterList);
   }

   // remaining active nodes are removed
   for( ; !NodeMapIter_end(&activeIter); NodeMapIter_next(&activeIter) )
   {
      uint16_t currentActive = NodeMapIter_key(&activeIter);

      UInt16List_append(outRemovedIDs, currentActive);
   }

   NodeMapIter_uninit(&activeIter);
   NodeListIter_uninit(&masterIter);

   Mutex_unlock(this->mutex); // U N L O C K


   // P H A S E 2 (Add/remove nodes from store.)

   // remove nodes
   UInt16ListIter_init(&removedIDsIter, outRemovedIDs);

   while(!UInt16ListIter_end(&removedIDsIter) )
   {
      uint16_t nodeID = UInt16ListIter_value(&removedIDsIter);
      UInt16ListIter_next(&removedIDsIter); // (removal invalidates iter)

      NodeStoreEx_deleteNode(this, nodeID);
   }

   UInt16ListIter_uninit(&removedIDsIter);

   // set local nic capabilities
   if(appLocalNode)
   {
      NicAddressList* localNicList = Node_getNicList(appLocalNode);
      NIC_supportedCapabilities(localNicList, &localNicCaps);
   }

   // add nodes
   NodeListIter_init(&addLaterIter, &addLaterNodes);

   for(; !NodeListIter_end(&addLaterIter); NodeListIter_next(&addLaterIter) )
   {
      Node* node = NodeListIter_value(&addLaterIter);

      if(appLocalNode)
         NodeConnPool_setLocalNicCaps(Node_getConnPool(node), &localNicCaps);

      NodeStoreEx_addOrUpdateNode(this, &node);
   }

   NodeListIter_uninit(&addLaterIter);


   NodeList_uninit(&addLaterNodes);
}

/**
 * Take special actions based on version of a (typically new) node, e.g. compat flags deactivation.
 *
 * Note: Caller must hold lock.
 */
void __NodeStoreEx_handleNodeVersion(NodeStoreEx* this, Node* node)
{
   // nothing to be done here currently
}
