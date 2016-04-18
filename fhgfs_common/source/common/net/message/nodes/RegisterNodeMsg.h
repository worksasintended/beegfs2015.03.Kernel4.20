#ifndef REGISTERNODEMSG_H_
#define REGISTERNODEMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/net/sock/NetworkInterfaceCard.h>
#include <common/Common.h>

/**
 * Registers a new node with the management daemon.
 *
 * This also provides the mechanism to retrieve a numeric node ID from the management node.
 */
class RegisterNodeMsg : public NetMessage
{
   public:

      /**
       * @param nodeID just a reference, so do not free it as long as you use this object!
       * @param nodeNumID set to 0 if no numeric ID was assigned yet and the numeric ID will be set
       * in the reponse
       * @param nicList just a reference, so do not free it as long as you use this object!
       * @param nodeFeatureFlags just a reference, so do not free it as long as you use this object
       * @param portUDP 0 means undefined
       * @param portTCP 0 means undefined
       */
      RegisterNodeMsg(const char* nodeID, uint16_t nodeNumID, NodeType nodeType,
         NicAddressList* nicList, const BitStore* nodeFeatureFlags,
         uint16_t portUDP, uint16_t portTCP) : NetMessage(NETMSGTYPE_RegisterNode)
      {
         this->nodeID = nodeID;
         this->nodeIDLen = strlen(nodeID);

         this->nodeNumID = nodeNumID;

         this->nodeType = nodeType;
         this->fhgfsVersion = 0; // set via setFhgfsVersion()

         this->rootNumID = 0; // 0 means unknown/undefined

         this->instanceVersion = 0; // reserved for future use

         this->nodeFeatureFlags = nodeFeatureFlags;

         this->nicListVersion = 0; // reserved for future use
         this->nicList = nicList;

         this->portUDP = portUDP;
         this->portTCP = portTCP;
      }


   protected:
      /**
       * For deserialization only
       */
      RegisterNodeMsg() : NetMessage(NETMSGTYPE_RegisterNode)
      {
      }


      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            nodeFeatureFlags->serialLen() + // (8byte aligned)
            Serialization::serialLenUInt64() + // instanceVersion
            Serialization::serialLenUInt64() + // nicListVersion
            Serialization::serialLenStrAlign4(nodeIDLen) +
            Serialization::serialLenNicList(nicList) + // (4byte aligned)
            Serialization::serialLenInt() + // nodeType
            Serialization::serialLenUInt() + // fhgfsVersion
            Serialization::serialLenUShort() + // nodeNumID
            Serialization::serialLenUShort() + // rootNumID
            Serialization::serialLenUShort() + // portUDP
            Serialization::serialLenUShort(); // portTCP
      }


   private:
      unsigned nodeIDLen;
      const char* nodeID;
      int nodeType;
      unsigned fhgfsVersion;
      uint16_t nodeNumID; // 0 means "undefined"
      uint16_t rootNumID; // 0 means "undefined"
      uint64_t instanceVersion; // not used currently
      uint64_t nicListVersion; // not used currently
      uint16_t portUDP; // 0 means "undefined"
      uint16_t portTCP; // 0 means "undefined"

      // for serialization
      const BitStore* nodeFeatureFlags; // not owned by this object
      NicAddressList* nicList; // not owned by this object!

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

      uint16_t getPortUDP()
      {
         return portUDP;
      }

      uint16_t getPortTCP()
      {
         return portTCP;
      }

};


#endif /* REGISTERNODEMSG_H_ */
