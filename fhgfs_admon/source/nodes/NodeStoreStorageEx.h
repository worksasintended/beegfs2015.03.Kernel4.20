#ifndef NODESTORESTORAGEEX_H_
#define NODESTORESTORAGEEX_H_

/*
 * NodeStoreStorageEx extends NodeStore from beegfs_common to support StorageNodeEx objects
 *
 * The node stores are derived in admon for each of the different node classes to handle their
 * special data structures in the addNode and addOrUpdateNode methods
 */

#include <common/nodes/NodeStore.h>
#include <nodes/StorageNodeEx.h>


class NodeStoreStorageEx : public NodeStoreServers
{
   public:
      NodeStoreStorageEx() throw(InvalidConfigException);

     bool addOrUpdateNode(StorageNodeEx** node);
     virtual bool addOrUpdateNode(Node** node);
     virtual bool addOrUpdateNodeEx(Node** node, uint16_t* outNodeNumID);

     void releaseStorageNode(StorageNodeEx** node);
};

#endif /*NODESTORESTORAGEEX_H_*/
