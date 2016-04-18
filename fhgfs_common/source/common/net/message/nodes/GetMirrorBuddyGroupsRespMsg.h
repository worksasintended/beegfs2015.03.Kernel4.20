#ifndef GETMIRRORBUDDYGROUPSRESPMSG_H_
#define GETMIRRORBUDDYGROUPSRESPMSG_H_

#include <common/Common.h>
#include <common/net/message/NetMessage.h>


class GetMirrorBuddyGroupsRespMsg : public NetMessage
{
   public:
      GetMirrorBuddyGroupsRespMsg(UInt16List* buddyGroupIDs, UInt16List* primaryTargetIDs,
         UInt16List* secondaryTargetIDs) : NetMessage(NETMSGTYPE_GetMirrorBuddyGroupsResp)
      {
         this->buddyGroupIDs = buddyGroupIDs;
         this->primaryTargetIDs = primaryTargetIDs;
         this->secondaryTargetIDs = secondaryTargetIDs;
      }

      GetMirrorBuddyGroupsRespMsg() :
         NetMessage(NETMSGTYPE_GetMirrorBuddyGroupsResp)
      {
      }

   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      virtual unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenUInt16List(buddyGroupIDs) +
            Serialization::serialLenUInt16List(primaryTargetIDs) +
            Serialization::serialLenUInt16List(secondaryTargetIDs);
      }

   private:
      UInt16List* buddyGroupIDs; // not owned by this object!
      UInt16List* primaryTargetIDs; // not owned by this object!
      UInt16List* secondaryTargetIDs; // not owned by this object!

      // for deserialization
      unsigned buddyGroupIDsElemNum;
      const char* buddyGroupIDsListStart;
      unsigned buddyGroupIDsBufLen;

      unsigned primaryTargetIDsElemNum;
      const char* primaryTargetIDsListStart;
      unsigned primaryTargetIDsBufLen;

      unsigned secondaryTargetIDsElemNum;
      const char* secondaryTargetIDsListStart;
      unsigned secondaryTargetIDsBufLen;

   public:
      void parseBuddyGroupIDs(UInt16List* outIDs)
      {
         Serialization::deserializeUInt16List(
            buddyGroupIDsBufLen, buddyGroupIDsElemNum, buddyGroupIDsListStart, outIDs);
      }

      void parsePrimaryTargetIDs(UInt16List* outIDs)
      {
         Serialization::deserializeUInt16List(
            primaryTargetIDsBufLen, primaryTargetIDsElemNum, primaryTargetIDsListStart, outIDs);
      }

      void parseSecondaryTargetIDs(UInt16List* outIDs)
      {
         Serialization::deserializeUInt16List(
            secondaryTargetIDsBufLen, secondaryTargetIDsElemNum, secondaryTargetIDsListStart, outIDs);
      }
};


#endif /* GETMIRRORBUDDYGROUPSRESPMSG_H_ */
