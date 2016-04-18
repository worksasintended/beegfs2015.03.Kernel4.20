#ifndef MIRRORMETADATAWORK_H_
#define MIRRORMETADATAWORK_H_

#include <common/components/worker/Work.h>
#include <common/Common.h>
#include <components/metadatamirrorer/MirrorerTask.h>


class MirrorMetadataWork : public Work
{
   public:
      /**
       * @param taskList just a reference, so do not free/modify as long as this work packet
       * is using it.
       * @param taskListNumElems number of all elements in list
       * @param taskListSerialLen serial length of all elements in list
       */
      MirrorMetadataWork(uint16_t mirrorNodeID, MirrorerTaskList* taskList,
         unsigned taskListNumElems, unsigned taskListSerialLen)
      {
         this->nodeID = mirrorNodeID;
         this->taskList = taskList;
         this->taskListNumElems = taskListNumElems;
         this->taskListSerialLen = taskListSerialLen;
      }

      virtual ~MirrorMetadataWork() {}


      virtual void process(char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen);


   private:
      uint16_t nodeID;
      MirrorerTaskList* taskList;
      unsigned taskListNumElems;
      unsigned taskListSerialLen;

      FhgfsOpsErr communicate();

};


#endif /* MIRRORMETADATAWORK_H_ */
