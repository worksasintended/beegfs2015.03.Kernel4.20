#ifndef REQUESTMETADATAWORK_H_
#define REQUESTMETADATAWORK_H_

/*
 * RequestMetaDataWork sends a message to a specific metadata node to request
 * the statistics of that node
 */

#include <common/components/worker/Work.h>
#include <common/net/message/admon/RequestMetaDataMsg.h>
#include <common/net/message/admon/RequestMetaDataRespMsg.h>
#include <nodes/NodeStoreMetaEx.h>
#include <nodes/MetaNodeEx.h>

class RequestMetaDataWork : public Work
{
   public:
      /*
       * @param metaNodes The metadata node store
       * @param node metadata node, which is contacted
       */
      RequestMetaDataWork(NodeStoreMetaEx *metaNodes, MetaNodeEx *node)
      {
         this->node = node;
         this->metaNodes = metaNodes;
         log.setContext("Request Meta Data Work");
      }

      virtual ~RequestMetaDataWork() {}

      void process(char* bufIn, unsigned bufInLen, char* bufOut,
         unsigned bufOutLen);

   private:
      MetaNodeEx *node;
      NodeStoreMetaEx *metaNodes;
      LogContext log;
};

#endif /*REQUESTMETADATAWORK_H_*/
