#ifndef GETSTORAGETARGETINFORESPMSG_H
#define GETSTORAGETARGETINFORESPMSG_H

#include <common/net/message/NetMessage.h>

class GetStorageTargetInfoRespMsg: public NetMessage
{
   public:
      GetStorageTargetInfoRespMsg(StorageTargetInfoList *targetInfoList) :
         NetMessage(NETMSGTYPE_GetStorageTargetInfoResp)
      {
         this->targetInfoList = targetInfoList;
      }

      GetStorageTargetInfoRespMsg() : NetMessage(NETMSGTYPE_GetStorageTargetInfoResp)
      {
      }

   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenStorageTargetInfoList(targetInfoList);
      }

   private:
      StorageTargetInfoList* targetInfoList; // not owned by this object!

      // for deserializaztion
      const char* targetInfoListStart;
      unsigned targetInfoListElemNum;

   public:
      void parseStorageTargetInfos(StorageTargetInfoList *outList)
      {
         Serialization::deserializeStorageTargetInfoList(targetInfoListElemNum,
            targetInfoListStart, outList);
      }
};

#endif /*GETSTORAGETARGETINFORESPMSG_H*/
