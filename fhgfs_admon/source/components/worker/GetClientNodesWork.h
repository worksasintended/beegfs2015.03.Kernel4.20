#ifndef GETCLIENTNODESWORK_H_
#define GETCLIENTNODESWORK_H_

/*
 * GetClientNodesWork is a work packet to download the client nodes from the mgmtd
 */

#include <common/nodes/Node.h>
#include <common/components/worker/Work.h>
#include <common/net/message/nodes/GetNodesMsg.h>
#include <common/net/message/nodes/GetNodesRespMsg.h>
#include <common/net/message/NetMessage.h>
#include <app/App.h>
#include <components/worker/GetNodeInfoWork.h>

class GetClientNodesWork: public Work
{
   public:
      /*
       * @param mgmtdNode Pointer to the management node
       * @param clients The global client node store
       */
      GetClientNodesWork(Node *mgmtdNode, NodeStoreClients *clients)
      {
         this->mgmtdNode = mgmtdNode;
         this->clients = clients;
         log.setContext("GetClientsWork");
      }

      void process(char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen);

   private:
      Node *mgmtdNode;
      NodeStoreClients *clients;
      LogContext log;
};

#endif /*GETCLIENTNODESWORK_H_*/
