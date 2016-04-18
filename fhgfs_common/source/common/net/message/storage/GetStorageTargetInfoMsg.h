#ifndef GETSTORAGETARGETINFOMSG_H
#define GETSTORAGETARGETINFOMSG_H

#include <common/net/message/NetMessage.h>

class GetStorageTargetInfoMsg: public NetMessage
{
   public:
      /*
       * @param targetIDs list of targetIDs to get info on; maybe empty, in which case server
       * should deliver info on all targets he has attached
       */
      GetStorageTargetInfoMsg(UInt16List* targetIDs) : NetMessage(NETMSGTYPE_GetStorageTargetInfo)
      {
         this->targetIDs = targetIDs;
      }

   protected:
      GetStorageTargetInfoMsg() : NetMessage(NETMSGTYPE_GetStorageTargetInfo)
      {
      }

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenUInt16List(targetIDs);
      }

   private:
      UInt16List* targetIDs; // not owned by this object

      // for deserializaztion
      const char* targetIDsStart;
      unsigned targetIDsLen;
      unsigned targetIDsElemNum;

   public:
      void parseTargetIDs(UInt16List *outList)
      {
         Serialization::deserializeUInt16List(targetIDsLen, targetIDsElemNum, targetIDsStart,
            outList);
      }


};

#endif /*GETSTORAGETARGETINFOMSG_H*/
