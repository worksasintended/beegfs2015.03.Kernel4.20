#ifndef NODESTOREMETAEX_H_
#define NODESTOREMETAEX_H_

/*
 * NodeStoreMetaEx extends NodeStore from beegfs_common to support MetaNodeEx objects
 *
 * The node stores are derived in admon for each of the different node classes to handle their
 * special data structures in the addNode and addOrUpdateNode methods
 */

#include <common/nodes/NodeStore.h>
#include <nodes/MetaNodeEx.h>


class NodeStoreMetaEx : public NodeStoreServers
{
   public:
      NodeStoreMetaEx() throw(InvalidConfigException);

     bool addOrUpdateNode(MetaNodeEx** node);
     virtual bool addOrUpdateNode(Node** node);
     virtual bool addOrUpdateNodeEx(Node** node, uint16_t* outNodeNumID);

     void releaseMetaNode(MetaNodeEx** node);
};

#endif /*NODESTOREMETAEX_H_*/
