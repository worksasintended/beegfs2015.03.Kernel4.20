#ifndef REQUESTSTORAGEDATAWORK_H_
#define REQUESTSTORAGEDATAWORK_H_

/*
 * RequestStorageDataWork sends a message to a specific storage node to request
 * the statistics of that node
 */

#include <common/components/worker/Work.h>
#include <common/net/message/admon/RequestStorageDataMsg.h>
#include <common/net/message/admon/RequestStorageDataRespMsg.h>
#include <common/toolkit/StringTk.h>
#include <nodes/StorageNodeEx.h>
#include <nodes/NodeStoreStorageEx.h>

class RequestStorageDataWork: public Work
{
   public:
      /*
       * @param storageNodes The storage node store
       * @param node storage node, which is contacted
       */
      RequestStorageDataWork(NodeStoreStorageEx *storageNodes,
         StorageNodeEx *node)
      {
         this->node = node;
         this->storageNodes = storageNodes;
         log.setContext("Request Storage Data Work");
      }

      virtual ~RequestStorageDataWork() {}

      void process(char* bufIn, unsigned bufInLen, char* bufOut,
         unsigned bufOutLen);

   private:
      StorageNodeEx *node;
      NodeStoreStorageEx *storageNodes;
      LogContext log;
};

#endif /*REQUESTSTORAGEDATAWORK_H_*/
