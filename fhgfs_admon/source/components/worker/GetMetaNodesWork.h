#ifndef GETMETANODESWORK_H_
#define GETMETANODESWORK_H_

/*
 * GetMetaNodesWork is a work packet to download the metadata nodes from the mgmtd and fetch some
 * general information about the nodes
 */

#include <common/nodes/Node.h>
#include <common/components/worker/Work.h>
#include <common/net/message/nodes/GetNodesMsg.h>
#include <common/net/message/nodes/GetNodesRespMsg.h>
#include <common/net/message/NetMessage.h>
#include <nodes/NodeStoreMetaEx.h>
#include <app/App.h>
#include <components/worker/GetNodeInfoWork.h>

class GetMetaNodesWork: public Work
{
   public:
      /*
       * @param mgmtdNode Pointer to the management node
       * @param metaNodes The global metadata server node store
       */
      GetMetaNodesWork(Node *mgmtdNode, NodeStoreMetaEx *metaNodes)
      {
         this->mgmtdNode = mgmtdNode;
         this->metaNodes = metaNodes;
         log.setContext("GetMetaNodes Work");
      }

      virtual ~GetMetaNodesWork() {}

      void process(char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen);

   private:
      Node *mgmtdNode;
      NodeStoreMetaEx*metaNodes;
      LogContext log;
};

#endif /*GETMETANODESWORK_H_*/
