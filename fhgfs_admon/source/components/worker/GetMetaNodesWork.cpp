#include <program/Program.h>
#include "GetMetaNodesWork.h"

void GetMetaNodesWork::process(char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen)
{
   NodeList metaNodesList;
   UInt16List addedMetaNodes;
   UInt16List removedMetaNodes;
   uint16_t rootNodeID;

   // get an updated list of metadata nodes
   if(NodesTk::downloadNodes(mgmtdNode, NODETYPE_Meta, &metaNodesList, false, &rootNodeID))
   {
      // sync the downloaded list with the node store
      metaNodes->syncNodes(&metaNodesList, &addedMetaNodes, &removedMetaNodes, true);

      metaNodes->setRootNodeNumID(rootNodeID, false);

      if(addedMetaNodes.size())
         log.log(Log_WARNING, "Nodes added (sync results): "
            + StringTk::uintToStr(addedMetaNodes.size())
            + " (Type: " + Node::nodeTypeToStr(NODETYPE_Meta) + ")");

      if(removedMetaNodes.size())
         log.log(Log_WARNING, "Nodes removed (sync results): "
            + StringTk::uintToStr(removedMetaNodes.size())
            + " (Type: " + Node::nodeTypeToStr(NODETYPE_Meta) + ")");

      // for each node added, get some general information
      for(UInt16ListIter iter = addedMetaNodes.begin(); iter != addedMetaNodes.end(); iter++)
      {
         Program::getApp()->getWorkQueue()->addIndirectWork(
            new GetNodeInfoWork(*iter, NODETYPE_Meta));
      }
   }
   else
   {
      log.log(Log_ERR, "Couldn't download metadata server list from management daemon.");
   }
}
