#include <common/threading/SafeMutexLock.h>
#include <common/toolkit/Random.h>
#include "NodeStoreStorageEx.h"


NodeStoreStorageEx::NodeStoreStorageEx() throw(InvalidConfigException) :
   NodeStoreServers(NODETYPE_Storage, false)
{
   // nothing to be done here
}


bool NodeStoreStorageEx::addOrUpdateNode(StorageNodeEx** node)
{
   std::string nodeID( (*node)->getID() );
   uint16_t nodeNumID = (*node)->getNumID();
   Node* tmpNodeCast = *node; // to allow usage as Node** below

   // sanity check: don't allow nodeNumID==0 (only mgmtd allows this)
   bool precheckRes = addOrUpdateNodePrecheck(&tmpNodeCast);
   if(!precheckRes)
   {
      *node = NULL;
      return false; // precheck automatically deletes *node
   }

   SafeMutexLock mutexLock(&mutex); // L O C K

   // check if this is a new node or an update of an existing node

   NodeMapIter iter = activeNodes.find(nodeNumID);
   if(iter != activeNodes.end() )
   { // node exists already => update
      // update node
      NodeReferencer* nodeRefer = iter->second;
      StorageNodeEx *oldNode = (StorageNodeEx *) nodeRefer->getReferencedObject();

      oldNode->update(*node);
   }
   else
   { // new node
      (*node)->initializeDBData();
   }

   bool retVal = addOrUpdateNodeUnlocked(&tmpNodeCast, NULL);

   mutexLock.unlock(); // U N L O C K

   *node = NULL; // to prevent accidental usage after calling this method

   return retVal;
}

/**
 * this function should only be called by sync nodes, all others use the typed version.
 *
 * it is important here that we only insert the specialized node objects into the store, not the
 * generic "class Node" objects.
 */
bool NodeStoreStorageEx::addOrUpdateNode(Node** node)
{
   LogContext context("add/update node");
   context.logErr("Wrong method used!");
   context.logBacktrace();

   return false;
}

bool NodeStoreStorageEx::addOrUpdateNodeEx(Node** node, uint16_t* outNodeNumID)
{
   std::string nodeID( (*node)->getID() );
   uint16_t nodeNumID = (*node)->getNumID();

   // sanity check: don't allow nodeNumID==0 (only mgmtd allows this)
   bool precheckRes = addOrUpdateNodePrecheck(node);
   if(!precheckRes)
      return false; // precheck automatically deletes *node and sets it to NULL

   SafeMutexLock mutexLock(&mutex); // L O C K

   // check if this is a new node

   NodeMapIter iter = activeNodes.find(nodeNumID);
   if(iter == activeNodes.end() )
   { // new node
      // new node which will be kept => do the DB operations
      NicAddressList nicList = (*node)->getNicList();
      StorageNodeEx* storageNode = new StorageNodeEx(
         nodeID, nodeNumID, (*node)->getPortUDP(), (*node)->getPortTCP(), nicList);

      delete(*node);
      *node = storageNode;

      storageNode->initializeDBData();
   }

   bool retVal = addOrUpdateNodeUnlocked(node, outNodeNumID);

   mutexLock.unlock(); // U N L O C K

   return retVal;
}

void NodeStoreStorageEx::releaseStorageNode(StorageNodeEx** node)
{
   Node* tmpNodeCast = *node; // to allow using as Node** below

   releaseNode(&tmpNodeCast);

   *node = NULL;
}
