#ifndef GETSTATESANDBUDDYGROUPSRESPMSG_H_
#define GETSTATESANDBUDDYGROUPSRESPMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/Common.h>


/**
 * This message carries two maps:
 *    1) buddyGroupID -> primaryTarget, secondaryTarget
 *    2) targetID -> targetReachabilityState, targetConsistencyState
 */
class GetStatesAndBuddyGroupsRespMsg : public NetMessage
{
   public:
      GetStatesAndBuddyGroupsRespMsg(UInt16List* buddyGroupIDs, UInt16List* primaryTargetIDs,
            UInt16List* secondaryTargetIDs, UInt16List* targetIDs,
            UInt8List* targetReachabilityStates, UInt8List* targetConsistencyStates) :
            NetMessage(NETMSGTYPE_GetStatesAndBuddyGroupsResp)
      {
         this->buddyGroupIDs = buddyGroupIDs;
         this->primaryTargetIDs = primaryTargetIDs;
         this->secondaryTargetIDs = secondaryTargetIDs;

         this->targetIDs = targetIDs;
         this->targetReachabilityStates = targetReachabilityStates;
         this->targetConsistencyStates = targetConsistencyStates;
      }

      /**
       * For deserialization only.
       */
      GetStatesAndBuddyGroupsRespMsg() :
            NetMessage(NETMSGTYPE_GetStatesAndBuddyGroupsResp)
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
               Serialization::serialLenUInt16List(secondaryTargetIDs) +
               Serialization::serialLenUInt16List(targetIDs) +
               Serialization::serialLenUInt8List(targetReachabilityStates) +
               Serialization::serialLenUInt8List(targetConsistencyStates);
      }

   private:
      UInt16List* buddyGroupIDs; // Not owned by this object!
      UInt16List* primaryTargetIDs; // Not owned by this object!
      UInt16List* secondaryTargetIDs; // Not owned by this object!

      UInt16List* targetIDs; // Not owned by this object!
      UInt8List* targetReachabilityStates; // Not owned by this object!
      UInt8List* targetConsistencyStates; // Not owned by this object!

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

      unsigned targetIDsElemNum;
      const char* targetIDsListStart;
      unsigned targetIDsBufLen;

      unsigned targetReachabilityStatesElemNum;
      const char* targetReachabilityStatesListStart;
      unsigned targetReachabilityStatesBufLen;

      unsigned targetConsistencyStatesElemNum;
      const char* targetConsistencyStatesListStart;
      unsigned targetConsistencyStatesBufLen;

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

      void parseTargetIDs(UInt16List* outIDs)
      {
         Serialization::deserializeUInt16List(
            targetIDsBufLen, targetIDsElemNum, targetIDsListStart, outIDs);
      }

      void parseReachabilityStates(UInt8List* outStates)
      {
         Serialization::deserializeUInt8List(
            targetReachabilityStatesBufLen, targetReachabilityStatesElemNum,
            targetReachabilityStatesListStart, outStates);
      }

      void parseConsistencyStates(UInt8List* outStates)
      {
         Serialization::deserializeUInt8List(
            targetConsistencyStatesBufLen, targetConsistencyStatesElemNum,
            targetConsistencyStatesListStart, outStates);
      }
};

#endif /* GETSTATESANDBUDDYGROUPSRESPMSG_H_ */
