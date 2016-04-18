#ifndef MODEHELPERGETNODES_H_
#define MODEHELPERGETNODES_H_

#include <common/Common.h>
#include <common/nodes/NodeStore.h>

class ModeHelperGetNodes
{
   public:
      static void checkReachability(NodeType nodeType, NodeList* nodeList,
         StringSet* outUnreachableNodes, unsigned numRetries, unsigned retryTimeoutMS);
      static void pingNodes(NodeType nodeType, NodeList* nodeList, unsigned numPingsPerNode);
      static void connTest(NodeType nodeType, NodeList* nodeList, unsigned numConns);


   private:
      ModeHelperGetNodes() {}
};

#endif /* MODEHELPERGETNODES_H_ */
