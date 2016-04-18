#ifndef SETSTORAGETARGETINFOMSG_H_
#define SETSTORAGETARGETINFOMSG_H_

#include <common/net/message/NetMessage.h>

class SetStorageTargetInfoMsg: public NetMessage
{
   public:
      SetStorageTargetInfoMsg(NodeType nodeType, StorageTargetInfoList *targetInfoList) :
         NetMessage(NETMSGTYPE_SetStorageTargetInfo)
      {
         this->nodeType = nodeType;
         this->targetInfoList = targetInfoList;
      }

      SetStorageTargetInfoMsg() : NetMessage(NETMSGTYPE_SetStorageTargetInfo)
      {
      }

   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenInt() +
            Serialization::serialLenStorageTargetInfoList(targetInfoList);
      }

   private:
      StorageTargetInfoList* targetInfoList; // not owned by this object!
      int nodeType;

      // for deserializaztion
      const char* targetInfoListStart;
      unsigned targetInfoListElemNum;

   public:
      NodeType getNodeType()
      {
         return (NodeType)nodeType;
      }

      void parseStorageTargetInfos(StorageTargetInfoList* outList)
      {
         Serialization::deserializeStorageTargetInfoList(targetInfoListElemNum,
            targetInfoListStart, outList);
      }
};

#endif /*SETSTORAGETARGETINFOMSG_H_*/
