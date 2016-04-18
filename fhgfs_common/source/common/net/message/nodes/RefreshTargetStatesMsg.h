#ifndef REFRESHTARGETSTATESMSG_H_
#define REFRESHTARGETSTATESMSG_H_

#include <common/net/message/AcknowledgeableMsg.h>
#include <common/Common.h>


/**
 * This msg notifies the receiver about a change in the targets states store. It is typically sent
 * by the mgmtd. The receiver should update its target state store
 */
class RefreshTargetStatesMsg : public AcknowledgeableMsg
{
   public:
      /**
       * Default constructor for serialization and deserialization.
       */
      RefreshTargetStatesMsg() : AcknowledgeableMsg(NETMSGTYPE_RefreshTargetStates)
      {
         this->ackID = "";
         this->ackIDLen = 0;
      }


   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      virtual unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
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
      unsigned ackIDLen;
      const char* ackID;


   public:
      // getters & setters
      const char* getAckID()
      {
         return ackID;
      }
};


#endif /* REFRESHTARGETSTATESMSG_H_ */
