#ifndef NODESTOREMGMTDEX_H_
#define NODESTOREMGMTDEX_H_

/*
 * NodeStoreMgmtEx extends NodeStore from beegfs_common to support MgmtNodeEx objects
 *
 * The node stores are derived in admon for each of the different node classes to handle their
 * special data structures in the addNode and addOrUpdateNode methods
 */

#include <common/nodes/NodeStore.h>
#include <nodes/MgmtNodeEx.h>


class NodeStoreMgmtEx : public NodeStoreServers
{
   public:
      NodeStoreMgmtEx() throw(InvalidConfigException);

      bool addOrUpdateNode(MgmtNodeEx** node);
      virtual bool addOrUpdateNode(Node** node);
      virtual bool addOrUpdateNodeEx(Node** node, uint16_t* outNodeNumID);

      void deleteAllNodes();

      void releaseMgmtNode(MgmtNodeEx** node);
};

#endif /*NODESTOREMGMTDEX_H_*/
