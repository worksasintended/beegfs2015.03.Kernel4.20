#ifndef SETMIRRORBUDDYGROUPMSG_H_
#define SETMIRRORBUDDYGROUPMSG_H_

#include <common/Common.h>
#include <common/net/message/AcknowledgeableMsg.h>

class SetMirrorBuddyGroupMsg : public AcknowledgeableMsg
{
   public:
      /**
       * Create or update a mirror buddy group
       *
       * @param primaryTargetID
       * @param secondaryTargetID
       * @param buddyGroupID may be 0 => create a buddy group with random ID
       * @param allowUpdate if buddyGroupID is set and that group exists, allow to update it
       */
      SetMirrorBuddyGroupMsg(uint16_t primaryTargetID, uint16_t secondaryTargetID,
         uint16_t buddyGroupID = 0, bool allowUpdate = false) :
            AcknowledgeableMsg(NETMSGTYPE_SetMirrorBuddyGroup), primaryTargetID(primaryTargetID),
         secondaryTargetID(secondaryTargetID), buddyGroupID(buddyGroupID), allowUpdate(allowUpdate)
      {
         this->ackID = "";
         this->ackIDLen = 0;
      }

      SetMirrorBuddyGroupMsg() : AcknowledgeableMsg(NETMSGTYPE_SetMirrorBuddyGroup)
      {
      }


   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      virtual unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenUInt16() + // primaryTargetID
            Serialization::serialLenUInt16() + // secondaryTargetID
            Serialization::serialLenUInt16() + // buddyGroupID
            Serialization::serialLenBool()   + // allowUpdate
            Serialization::serialLenStr(ackIDLen);
      }

      /**
       * @param ackID just a reference
       */
      void setAckIDInternal(const char* ackID)
      {
         this->ackID = ackID;
         this->ackIDLen = strlen(ackID);
      }

   private:
      uint16_t primaryTargetID;
      uint16_t secondaryTargetID;
      uint16_t buddyGroupID;
      bool allowUpdate;

      const char* ackID;
      unsigned ackIDLen;

   public:
      // getters & setters
      uint16_t getPrimaryTargetID() const
      {
         return primaryTargetID;
      }

      uint16_t getSecondaryTargetID() const
      {
         return secondaryTargetID;
      }

      uint16_t getBuddyGroupID() const
      {
         return buddyGroupID;
      }

      bool getAllowUpdate() const
      {
         return allowUpdate;
      }

      const char* getAckID()
      {
         return ackID;
      }
};

#endif /* SETMIRRORBUDDYGROUPMSG_H_ */
