#ifndef MIRRORMETADATAMSGEX_H_
#define MIRRORMETADATAMSGEX_H_

#include <common/net/message/storage/mirroring/MirrorMetadataMsg.h>
#include <components/metadatamirrorer/MirrorerTask.h>


class MirrorMetadataMsgEx : public MirrorMetadataMsg
{
   public:
      /**
       * @param taskList just a reference, so do not free it as long as you use this object
       * @param taskListNumElems number of all elements in list
       * @param taskListSerialLen serial length of all elements in list
       */
      MirrorMetadataMsgEx(MirrorerTaskList* taskList, unsigned taskListNumElems,
         unsigned taskListSerialLen) :
         MirrorMetadataMsg(taskList, taskListNumElems, taskListSerialLen)
      {
      }

      /**
       * For deserialization only
       */
      MirrorMetadataMsgEx() : MirrorMetadataMsg() {}


      virtual ~MirrorMetadataMsgEx()
      {
         // cleanup all tasks
         for(MirrorerTaskListIter iter=deserTaskList.begin(); iter != deserTaskList.end(); iter++)
            delete(*iter);
      }


      virtual bool processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats);


   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      virtual unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenUInt() + // taskListNumElems
            taskListSerialLen; // taskList
      }

   private:
      // for deserialization
      unsigned deserTaskListNumElems;
      MirrorerTaskList deserTaskList;


   public:
      unsigned getTaskListNumElems() const
      {
         return deserTaskListNumElems;
      }

      MirrorerTaskList* getTaskList()
      {
         return &deserTaskList;
      }
};

#endif /* MIRRORMETADATAMSGEX_H_ */
