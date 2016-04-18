#include "DataRequestorIOStats.h"
#include <common/toolkit/StringTk.h>
#include <program/Program.h>

void DataRequestorIOStats::run()
{
   try
   {
      log.log(Log_DEBUG, std::string("Component started."));

      registerSignalHandler();
      requestLoop();

      log.log(Log_DEBUG, std::string("Component stopped."));
   }
   catch (std::exception& e)
   {
      Program::getApp()->handleComponentException(e);
   }
}

void DataRequestorIOStats::requestLoop()
{
   Node *node;

   // do loop until terminate is set
   while (!getSelfTerminate())
   {
      // create work packets for the meta nodes; for each metadata node a RequestMetaDataWork is
      // put into the work queue, where it will be fetched by a worker
      node = metaNodes->referenceFirstNode();
      while (node != NULL)
      {
         uint16_t nodeID = node->getNumID();

         this->workQueue->addIndirectWork(new RequestMetaDataWork(metaNodes, (MetaNodeEx*) node));
         node = metaNodes->referenceNextNode(nodeID);
      }

      // same for storage nodes
      node = storageNodes->referenceFirstNode();
      while (node != NULL)
      {
         uint16_t nodeID = node->getNumID();

         this->workQueue->addIndirectWork(new RequestStorageDataWork(storageNodes,
            (StorageNodeEx*) node));
         node = storageNodes->referenceNextNode(nodeID);
      }

      // if stats for clients, etc. should be gathered, this would be the right place for it

      // clean up database once an hour to make sure it does not grow to much
      if (this->t.elapsedMS() > 3600000) // 1 hour
      {
         Program::getApp()->getDB()->cleanUp();
         this->t.setToNow();
      }

      // do nothing but wait for the time of queryInterval
      if (PThread::waitForSelfTerminateOrder(this->queryInterval))
         break;
   }
}
