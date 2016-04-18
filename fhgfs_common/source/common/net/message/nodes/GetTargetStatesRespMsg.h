#ifndef GETTARGETSTATESRESPMSG_H
#define GETTARGETSTATESRESPMSG_H

#include <common/Common.h>
#include <common/net/message/NetMessage.h>
#include <common/nodes/TargetStateStore.h>

class GetTargetStatesRespMsg : public NetMessage
{
   public:
      GetTargetStatesRespMsg(UInt16List* targetIDs, UInt8List* reachabilityStates,
            UInt8List* consistencyStates) :
         NetMessage(NETMSGTYPE_GetTargetStatesResp)
      {
         this->targetIDs = targetIDs;
         this->reachabilityStates = reachabilityStates;
         this->consistencyStates = consistencyStates;
      }

      GetTargetStatesRespMsg() :
         NetMessage(NETMSGTYPE_GetTargetStatesResp)
      {
      }

   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      virtual unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenUInt16List(targetIDs) +
            Serialization::serialLenUInt8List(reachabilityStates) +
            Serialization::serialLenUInt8List(consistencyStates);
      }

   private:
      UInt16List* targetIDs; // not owned by this object!
      UInt8List* reachabilityStates; // not owned by this object!
      UInt8List* consistencyStates; // not owned by this object!

      // for deserialization
      unsigned targetIDsElemNum;
      const char* targetIDsListStart;
      unsigned targetIDsBufLen;

      unsigned reachabilityStatesElemNum;
      const char* reachabilityStatesListStart;
      unsigned reachabilityStatesBufLen;

      unsigned consistencyStatesElemNum;
      const char* consistencyStatesListStart;
      unsigned consistencyStatesBufLen;

   public:
      void parseTargetIDs(UInt16List* outIDs)
      {
         Serialization::deserializeUInt16List(
            targetIDsBufLen, targetIDsElemNum, targetIDsListStart, outIDs);
      }

      void parseReachabilityStates(UInt8List* outStates)
      {
         Serialization::deserializeUInt8List(
            reachabilityStatesBufLen, reachabilityStatesElemNum, reachabilityStatesListStart,
            outStates);
      }

      void parseConsistencyStates(UInt8List* outStates)
      {
         Serialization::deserializeUInt8List(
            consistencyStatesBufLen, consistencyStatesElemNum, consistencyStatesListStart,
            outStates);
      }

      virtual TestingEqualsRes testingEquals(NetMessage* msg)
      {
         if ( msg->getMsgType() != this->getMsgType() )
            return TestingEqualsRes_FALSE;

         GetTargetStatesRespMsg *msgCast = (GetTargetStatesRespMsg*) msg;

         UInt16List targetIDsClone;
         UInt8List reachabilityStatesClone;
         UInt8List consistencyStatesClone;

         msgCast->parseTargetIDs(&targetIDsClone);
         msgCast->parseReachabilityStates(&reachabilityStatesClone);
         msgCast->parseConsistencyStates(&consistencyStatesClone);

         if (*(this->targetIDs) != targetIDsClone)
            return TestingEqualsRes_FALSE;

         if (*(this->reachabilityStates) != reachabilityStatesClone)
            return TestingEqualsRes_FALSE;

         if (*(this->consistencyStates) != consistencyStatesClone)
            return TestingEqualsRes_FALSE;

         return TestingEqualsRes_TRUE;
      }
};


#endif /* GETTARGETSTATESRESPMSG_H */
