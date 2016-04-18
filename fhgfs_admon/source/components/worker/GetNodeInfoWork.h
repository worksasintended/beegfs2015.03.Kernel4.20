#ifndef GETNODEINFOWORK_H_
#define GETNODEINFOWORK_H_

/**
 * GetNodeInfoWork is a work packet to retrieve general information of a given
 * node; works for all server types, as it requests generic information
 * (e.g. CPU, RAM, ...)
 */

#include <common/components/worker/Work.h>
#include <common/net/message/admon/GetNodeInfoMsg.h>
#include <common/net/message/admon/GetNodeInfoRespMsg.h>
#include <nodes/NodeStoreMetaEx.h>
#include <nodes/MetaNodeEx.h>
#include <nodes/NodeStoreStorageEx.h>
#include <nodes/StorageNodeEx.h>

class GetNodeInfoWork: public Work
{
   public:
      GetNodeInfoWork(uint16_t nodeID, NodeType nodeType)
      {
         this->nodeID = nodeID;
         this->nodeType = nodeType;

         log.setContext("GetNodeInfoWork");
      }

      void process(char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen);

   private:
      uint16_t nodeID;
      NodeType nodeType;
      LogContext log;

      bool sendMsg(Node *node, GeneralNodeInfo *info);
};

#endif /*GETNODEINFOWORK_H_*/
