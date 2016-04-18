#ifndef CHANGETARGETCONSISTENCYSTATESMSG_H
#define CHANGETARGETCONSISTENCYSTATESMSG_H

#include <common/net/message/AcknowledgeableMsg.h>
#include <common/Common.h>

/*
 *  NOTE: this message will be used in mgmtd to update target state store, but also in storage
 *  server to set local target states
 */

class ChangeTargetConsistencyStatesMsg : public AcknowledgeableMsg
{
   public:
      ChangeTargetConsistencyStatesMsg(NodeType nodeType, UInt16List* targetIDs,
         UInt8List* oldStates, UInt8List* newStates)
         : AcknowledgeableMsg(NETMSGTYPE_ChangeTargetConsistencyStates),
            nodeType(nodeType),
            targetIDs(targetIDs),
            oldStates(oldStates),
            newStates(newStates),
            ackIDLen(0),
            ackID("")
      {
      }

      ChangeTargetConsistencyStatesMsg()
         : AcknowledgeableMsg(NETMSGTYPE_ChangeTargetConsistencyStates)
      {
      }

   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      virtual unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenInt() +
            Serialization::serialLenUInt16List(targetIDs) +
            Serialization::serialLenUInt8List(oldStates) +
            Serialization::serialLenUInt8List(newStates) +
            Serialization::serialLenStrAlign4(ackIDLen);
      }

      void setAckIDInternal(const char* ackID)
      {
         this->ackID = ackID;
         this->ackIDLen = strlen(ackID);
      }

   private:
      int nodeType;

      UInt16List* targetIDs; // not owned by this object!
      UInt8List* oldStates; // not owned by this object!
      UInt8List* newStates; // not owned by this object!

      // for deserialization
      unsigned targetIDsElemNum;
      const char* targetIDsListStart;
      unsigned targetIDsBufLen;

      unsigned oldStatesElemNum;
      const char* oldStatesListStart;
      unsigned oldStatesBufLen;

      unsigned newStatesElemNum;
      const char* newStatesListStart;
      unsigned newStatesBufLen;

      unsigned ackIDLen;
      const char* ackID;

   public:
      NodeType getNodeType()
      {
         return (NodeType)nodeType;
      }

      void parseTargetIDs(UInt16List* outIDs)
      {
         Serialization::deserializeUInt16List(
            targetIDsBufLen, targetIDsElemNum, targetIDsListStart, outIDs);
      }

      void parseOldStates(UInt8List* outStates)
      {
         Serialization::deserializeUInt8List(
            oldStatesBufLen, oldStatesElemNum, oldStatesListStart, outStates);
      }

      void parseNewStates(UInt8List* outStates)
      {
         Serialization::deserializeUInt8List(
            newStatesBufLen, newStatesElemNum, newStatesListStart, outStates);
      }

      const char* getAckID()
      {
         return ackID;
      }
};

#endif /*CHANGETARGETCONSISTENCYSTATESMSG_H*/
