#ifndef GETENTRYINFOWORK_H_
#define GETENTRYINFOWORK_H_

/*
 * GetEntryInfoWork is intended to be used to fetch information on a specific entry in BeeGFS
 * (stripe size, default number of targets)
 */

#include <common/net/message/storage/attribs/GetEntryInfoMsg.h>
#include <common/net/message/storage/attribs/GetEntryInfoRespMsg.h>
#include <common/toolkit/MetadataTk.h>
#include <common/toolkit/NodesTk.h>
#include <common/net/message/NetMessage.h>
#include <nodes/NodeStoreMetaEx.h>
#include <common/components/worker/Work.h>

class GetEntryInfoWork: public Work
{
   public:
      /*
       * @param pathStr the path to retrieve information for
       * @param metaNodes Pointer to the global metadata node store
       * @param outChunkSize Pointer to a variable, in which the chunkSize will be saved
       * @param outDefaultNumTargets Pointer to a variable, in which the default no. of targets will be saved
       * @param waitCondition Pointer to a condition for synchronization (calling function can wait for the work to finish)
       */
      GetEntryInfoWork(std::string pathStr, NodeStoreMetaEx *metaNodes, uint *outChunkSize,
         uint *outDefaultNumTargets, Condition *waitCond)
      {
         this->pathStr = pathStr;
         this->metaNodes = metaNodes;
         this->outChunkSize = outChunkSize;
         this->outDefaultNumTargets = outDefaultNumTargets;
         this->waitCond = waitCond;
         log.setContext("GetEntryInfoWork");
      }

      virtual ~GetEntryInfoWork() {}

      void process(char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen);

   private:
      std::string pathStr;
      NodeStoreMetaEx *metaNodes;
      uint *outChunkSize;
      uint *outDefaultNumTargets;
      Condition *waitCond;
      LogContext log;
};

#endif /*GETENTRYINFOWORK_H_*/
