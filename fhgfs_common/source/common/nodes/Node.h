#ifndef NODE_H_
#define NODE_H_

#include <common/nodes/NodeFeatureFlags.h>
#include <common/threading/SafeMutexLock.h>
#include <common/threading/Condition.h>
#include <common/toolkit/BitStore.h>
#include <common/toolkit/Time.h>
#include <common/Common.h>
#include "NodeConnPool.h"

// forward declaration
class Node;


typedef std::list<Node*> NodeList;
typedef NodeList::iterator NodeListIter;


enum NodeType
   {NODETYPE_Invalid = 0, NODETYPE_Meta = 1, NODETYPE_Storage = 2, NODETYPE_Client = 3,
   NODETYPE_Mgmt = 4, NODETYPE_Hsm = 5, NODETYPE_Helperd = 6, NODETYPE_Admon = 7};


/**
 * This class represents a metadata, storage, client, etc node (aka service). It contains things
 * like ID and feature flags of a node and also provides the connection pool to communicate with
 * that node.
 */
class Node
{
   public:
      Node(std::string nodeID, uint16_t nodeNumID, unsigned short portUDP, unsigned short portTCP,
         NicAddressList& nicList);
      virtual ~Node();

      void updateLastHeartbeatT();
      Time getLastHeartbeatT();
      bool waitForNewHeartbeatT(Time* oldT, int timeoutMS);
      void updateInterfaces(unsigned short portUDP, unsigned short portTCP,
         NicAddressList& nicList);

      std::string getTypedNodeID();
      std::string getNodeIDWithTypeStr();
      std::string getNodeTypeStr();
      
      void setFeatureFlags(const BitStore* featureFlags);
      void updateFeatureFlagsThreadSafe(const BitStore* featureFlags);

      // static
      static std::string getTypedNodeID(std::string nodeID, uint16_t nodeNumID, NodeType nodeType);
      static std::string getNodeIDWithTypeStr(std::string nodeID, uint16_t nodeNumID,
         NodeType nodeType);
      static std::string nodeTypeToStr(NodeType nodeType);

      
   protected:
      Mutex mutex;
      Condition changeCond; // for last heartbeat time changes only 

      Node(std::string nodeID, uint16_t nodeNumID, unsigned short portUDP);
      
      void updateLastHeartbeatTUnlocked();
      Time getLastHeartbeatTUnlocked();
      void updateInterfacesUnlocked(unsigned short portUDP, unsigned short portTCP,
         NicAddressList& nicList);

      // getters & setters
      void setConnPool(NodeConnPool* connPool)
      {
         this->connPool = connPool;
      }
            
   
   private:
      std::string id; // string ID, generated locally on each node
      uint16_t numID; // numeric ID, assigned by mgmtd server store

      NodeType nodeType;

      unsigned fhgfsVersion; // fhgfs version code of this node
      BitStore nodeFeatureFlags; /* supported features of this node (access not protected by mutex,
                                    so be careful with updates) */

      NodeConnPool* connPool;
      unsigned short portUDP;

      Time lastHeartbeatT; // last heartbeat receive time
      
      
   public:
      // getters & setters

      std::string getID()
      {
         return id;
      }
      
      uint16_t getNumID() const
      {
         return numID;
      }

      void setNumID(uint16_t numID)
      {
         this->numID = numID;
      }

      NicAddressList getNicList()
      {
         return connPool->getNicList();
      }
      
      NodeConnPool* getConnPool()
      {
         return connPool;
      }
      
      unsigned short getPortUDP()
      {
         unsigned short retVal;
         
         SafeMutexLock mutexLock(&mutex);
         
         retVal = this->portUDP;
         
         mutexLock.unlock();
         
         return retVal;
      }
      
      virtual unsigned short getPortTCP()
      {
         return this->connPool->getStreamPort();
      }
      
      NodeType getNodeType() const
      {
         return nodeType;
      }

      void setNodeType(NodeType nodeType)
      {
         this->nodeType = nodeType;
      }

      unsigned getFhgfsVersion() const
      {
         return this->fhgfsVersion;
      }

      void setFhgfsVersion(unsigned fhgfsVersion)
      {
         this->fhgfsVersion = fhgfsVersion;
      }

      /**
       * Check if this node supports a certain feature.
       */
      bool hasFeature(unsigned featureBitIndex) const
      {
         return nodeFeatureFlags.getBitNonAtomic(featureBitIndex);
      }

      /**
       * Add a feature flag to this node.
       */
      void addFeature(unsigned featureBitIndex)
      {
         nodeFeatureFlags.setBit(featureBitIndex, true);
      }

      /**
       * note: returns a reference to internal flags, so this is only valid while you hold a
       * reference to this node.
       */
      const BitStore* getNodeFeatures()
      {
         return &nodeFeatureFlags;
      }

};


#endif /*NODE_H_*/
