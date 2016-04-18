#ifndef HEARTBEATMSG_H_
#define HEARTBEATMSG_H_

#include <common/net/message/AcknowledgeableMsg.h>
#include <common/net/sock/NetworkInterfaceCard.h>
#include <common/toolkit/BitStore.h>
#include <common/Common.h>

class HeartbeatMsg : public AcknowledgeableMsg
{
   public:
      
      /**
       * @param nodeID just a reference, so do not free it as long as you use this object
       * @param nicList just a reference, so do not free it as long as you use this object
       * @param nodeFeatureFlags just a reference, so do not free it as long as you use this object
       */
      HeartbeatMsg(const char* nodeID, uint16_t nodeNumID, NodeType nodeType,
         NicAddressList* nicList, const BitStore* nodeFeatureFlags)
         : AcknowledgeableMsg(NETMSGTYPE_Heartbeat)
      {
         this->nodeID = nodeID;
         this->nodeIDLen = strlen(nodeID);
         this->nodeNumID = nodeNumID;
         
         this->nodeType = nodeType;
         this->fhgfsVersion = 0;
         
         this->rootNumID = 0; // 0 means "unknown/undefined"

         this->instanceVersion = 0; // reserved for future use

         this->nodeFeatureFlags = nodeFeatureFlags;

         this->nicListVersion = 0; // reserved for future use
         this->nicList = nicList;
         
         this->portUDP = 0; // 0 means "undefined"
         this->portTCP = 0; // 0 means "undefined"

         this->ackID = "";
         this->ackIDLen = 0;
      }

      /**
       * For deserialization only
       */
      HeartbeatMsg() : AcknowledgeableMsg(NETMSGTYPE_Heartbeat)
      {
      }

      virtual TestingEqualsRes testingEquals(NetMessage* msg);



   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            nodeFeatureFlags->serialLen() + // (8byte aligned)
            Serialization::serialLenUInt64() + // instanceVersion
            Serialization::serialLenUInt64() + // nicListVersion
            Serialization::serialLenInt() + // nodeType
            Serialization::serialLenUInt() + // fhgfsVersion
            Serialization::serialLenStrAlign4(nodeIDLen) +
            Serialization::serialLenStrAlign4(ackIDLen) +
            Serialization::serialLenUShort() + // nodeNumID
            Serialization::serialLenUShort() + // rootNumID
            Serialization::serialLenUShort() + // portUDP
            Serialization::serialLenUShort() + // portTCP
            Serialization::serialLenNicList(nicList); // (4byte aligned)
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
      unsigned nodeIDLen;
      const char* nodeID;
      int nodeType;
      unsigned fhgfsVersion;
      uint16_t nodeNumID;
      uint16_t rootNumID; // 0 means unknown/undefined
      uint64_t instanceVersion; // not used currently
      uint64_t nicListVersion; // not used currently
      uint16_t portUDP; // 0 means "undefined"
      uint16_t portTCP; // 0 means "undefined"
      unsigned ackIDLen;
      const char* ackID;
      
      // for serialization
      const BitStore* nodeFeatureFlags; // not owned by this object
      NicAddressList* nicList; // not owned by this object

      // for deserialization
      const char* nodeFeatureFlagsStart; // points to location in receive buffer (from preprocess)
      unsigned nicListElemNum; // NETMSG_NICLISTELEM_SIZE defines the element size
      const char* nicListStart; // see NETMSG_NICLISTELEM_SIZE for element structure
      

   public:
      // inliners

      void parseNicList(NicAddressList* outNicList)
      {
         Serialization::deserializeNicList(nicListElemNum, nicListStart, outNicList);
      }
      
      void parseNodeFeatureFlags(BitStore* outFeatureFlags)
      {
         unsigned deserLen;

         outFeatureFlags->deserialize(nodeFeatureFlagsStart, &deserLen);
      }

      // getters & setters

      const char* getNodeID()
      {
         return nodeID;
      }
      
      uint16_t getNodeNumID()
      {
         return nodeNumID;
      }

      NodeType getNodeType()
      {
         return (NodeType)nodeType;
      }

      unsigned getFhgfsVersion()
      {
         return fhgfsVersion;
      }

      void setFhgfsVersion(unsigned fhgfsVersion)
      {
         this->fhgfsVersion = fhgfsVersion;
      }

      uint16_t getRootNumID()
      {
         return rootNumID;
      }
      
      void setRootNumID(uint16_t rootNumID)
      {
         this->rootNumID = rootNumID;
      }

      const char* getAckID()
      {
         return ackID;
      }
      
      void setPorts(uint16_t portUDP, uint16_t portTCP)
      {
         this->portUDP = portUDP;
         this->portTCP = portTCP;
      }

      uint16_t getPortUDP()
      {
         return portUDP;
      }
      
      uint16_t getPortTCP()
      {
         return portTCP;
      }

};

#endif /*HEARTBEATMSG_H_*/
