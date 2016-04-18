#ifndef SETMIRRORBUDDYGROUPRESPMSG_H_
#define SETMIRRORBUDDYGROUPRESPMSG_H_

#include <common/Common.h>
#include <common/net/message/NetMessage.h>


class SetMirrorBuddyGroupRespMsg : public NetMessage
{
   public:
      /**
       * @param result FhgfsOpsErr
       *
       * NOTE: result may be one of the following:
       * - FhgfsOpsErr_SUCCESS => new group added
       * - FhgfsOpsErr_EXISTS => group already exists and was updated
       * - FhgfsOpsErr_INUSE => group could not be updated because target is already in use
       *
       * @param groupID the created ID of the group, if the group was new; 0 otherwise
       */
      SetMirrorBuddyGroupRespMsg(FhgfsOpsErr result, uint16_t groupID = 0) :
         NetMessage(NETMSGTYPE_SetMirrorBuddyGroupResp), result(result), groupID(groupID)
      {
      }

      SetMirrorBuddyGroupRespMsg() :
         NetMessage(NETMSGTYPE_SetMirrorBuddyGroupResp)
      {
      }

   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      virtual unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenInt()    + // result
            Serialization::serialLenUInt16() + // groupID
            Serialization::serialLenUInt16();  // padding for 4-byte alignment
      }

   private:
      FhgfsOpsErr result;
      uint16_t groupID;

   public:
      FhgfsOpsErr getResult() const
      {
         return result;
      }

      uint16_t getBuddyGroupID() const
      {
         return groupID;
      }
};


#endif /* SETMIRRORBUDDYGROUPRESPMSG_H_ */
