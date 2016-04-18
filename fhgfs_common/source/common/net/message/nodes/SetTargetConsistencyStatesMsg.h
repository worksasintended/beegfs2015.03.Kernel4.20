#ifndef SETTARGETCONSISTENCYSTATESMSG_H
#define SETTARGETCONSISTENCYSTATESMSG_H

#include <common/net/message/AcknowledgeableMsg.h>
#include <common/Common.h>

/*
 *  NOTE: this message will be used in mgmtd to update target state store, but also in storage
 *  server to set local target states
 */

class SetTargetConsistencyStatesMsg : public AcknowledgeableMsg
{
   public:
      SetTargetConsistencyStatesMsg(UInt16List* targetIDs, UInt8List* states, bool setOnline) :
         AcknowledgeableMsg(NETMSGTYPE_SetTargetConsistencyStates)
      {
         this->targetIDs = targetIDs;
         this->states = states;
         this->setOnline = setOnline;

         this->ackID = "";
         this->ackIDLen = 0;
      }

      SetTargetConsistencyStatesMsg() : AcknowledgeableMsg(NETMSGTYPE_SetTargetConsistencyStates)
      {
      }

   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      virtual unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenUInt16List(targetIDs) +
            Serialization::serialLenUInt8List(states) +
            Serialization::serialLenStrAlign4(ackIDLen) +
            Serialization::serialLenBool();
      }

      void setAckIDInternal(const char* ackID)
      {
         this->ackID = ackID;
         this->ackIDLen = strlen(ackID);
      }

   private:
      UInt16List* targetIDs; // not owned by this object!
      UInt8List* states; // not owned by this object!

      /**
       * setOnline shall be set to "true" when reporting states for local targets (i.e. targets
       *           which are on the reporting server). When receiving state reports for buddy
       *           targets (the primary telling the mgmtd that the secondary needs a resync), the
       *           targets should not be ONLINEd again.
       */
      bool setOnline;


      // for deserialization
      unsigned targetIDsElemNum;
      const char* targetIDsListStart;
      unsigned targetIDsBufLen;

      unsigned statesElemNum;
      const char* statesListStart;
      unsigned statesBufLen;

      unsigned ackIDLen;
      const char* ackID;

   public:
      void parseTargetIDs(UInt16List* outIDs)
      {
         Serialization::deserializeUInt16List(
            targetIDsBufLen, targetIDsElemNum, targetIDsListStart, outIDs);
      }

      void parseStates(UInt8List* outStates)
      {
         Serialization::deserializeUInt8List(
            statesBufLen, statesElemNum, statesListStart, outStates);
      }

      bool getSetOnline()
      {
         return this->setOnline;
      }

      const char* getAckID()
      {
         return ackID;
      }
};

#endif /*SETTARGETCONSISTENCYSTATESMSG_H*/
