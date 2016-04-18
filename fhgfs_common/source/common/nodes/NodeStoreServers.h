#ifndef NODESTORESERVERS_H_
#define NODESTORESERVERS_H_

#include <common/app/log/LogContext.h>
#include <common/threading/Mutex.h>
#include <common/threading/Condition.h>
#include <common/toolkit/ObjectReferencer.h>
#include <common/toolkit/Random.h>
#include <common/nodes/Node.h>
#include <common/nodes/NodeCapacityPools.h>
#include <common/nodes/TargetMapper.h>
#include <common/storage/StorageErrors.h>
#include <common/Common.h>
#include "AbstractNodeStore.h"


class NodeStoreServers : public AbstractNodeStore
{
   public:
      // type definitions

      typedef ObjectReferencer<Node*> NodeReferencer;

      typedef std::map<uint16_t, NodeReferencer*> NodeMap;
      typedef NodeMap::iterator NodeMapIter;
      typedef NodeMap::value_type NodeMapVal;

      /**
       * Note: Using a pointer-based map is very important for deleted nodes to handle the case where
       * the mgmtd re-assigns a numeric ID to a new node while another server (e.g. MDS) might still
       * reference the old node with this ID, due to the async store updates.
       */
      typedef std::map<Node*, NodeReferencer*> NodePtrMap;
      typedef NodePtrMap::iterator NodePtrMapIter;
      typedef NodePtrMap::value_type NodePtrMapVal;


      NodeStoreServers(NodeType storeType, bool channelsDirectDefault)
         throw(InvalidConfigException);
      virtual ~NodeStoreServers();

      virtual bool addOrUpdateNode(Node** node);
      virtual bool addOrUpdateNodeEx(Node** node, uint16_t* outNodeNumID);
      bool updateLastHeartbeatT(uint16_t id);
      virtual bool deleteNode(uint16_t id);

      Node* referenceNode(uint16_t id);
      Node* referenceRootNode();
      Node* referenceNodeByTargetID(uint16_t targetID, TargetMapper* targetMapper,
         FhgfsOpsErr* outErr=NULL);
      Node* referenceNodeByStringID(std::string nodeStrID);
      void releaseNode(Node** node);

      virtual Node* referenceRandomNode();
      virtual Node* referenceFirstNode();
      virtual Node* referenceNextNodeAndReleaseOld(Node* oldNode);
      Node* referenceNextNode(uint16_t id);

      virtual void referenceAllNodes(NodeList* outList);
      virtual void releaseAllNodes(NodeList* list);

      bool isNodeActive(uint16_t id);
      virtual size_t getSize();

      uint16_t getRootNodeNumID();
      virtual bool setRootNodeNumID(uint16_t id, bool ignoreExistingRoot);

      bool waitForFirstNode(int waitTimeoutMS);

      void syncNodes(NodeList* masterList, UInt16List* outAddedIDs, UInt16List* outRemovedIDs,
         bool updateExisting, Node* appLocalNode=NULL);

      void attachCapacityPools(NodeCapacityPools* capacityPools);
      void attachTargetMapper(TargetMapper* targetMapper);
      void attachStateStore(TargetStateStore* stateStore);

      std::string getNodeIDWithTypeStr(uint16_t numID);
      std::string getTypedNodeID(uint16_t numID);

   protected:
      Mutex mutex;
      Condition newNodeCond; // set when a new node is added to the store (or undeleted)
      Node* localNode;
      uint16_t rootNodeID;
      Random randGen; // must also be synchronized by mutex

      bool channelsDirectDefault; // for connpools, false to make all channels indirect by default

      NodeMap activeNodes; // key is numeric node id
      NodePtrMap deletedNodes; // key is node pointer (to avoid collision problems on num id reuse)

      NodeCapacityPools* capacityPools; // optional for auto remove (may be NULL)
      TargetMapper* targetMapper; // optional for auto remove (may be NULL)
      TargetStateStore* stateStore; // optional for auto remove (may be NULL)


      bool addOrUpdateNodeUnlocked(Node** node, uint16_t* outNodeNumID);
      bool addOrUpdateNodePrecheck(Node** node);


   private:
      void releaseNodeUnlocked(Node** node);
      uint16_t generateNodeNumID(Node* node);
      uint16_t retrieveNumIDFromStringID(std::string nodeID);
      bool checkNodeNumIDCollision(uint16_t numID);

      void handleNodeVersion(Node* node);


   public:
      // getters & setters

      /**
       * Note: this is supposed to be called before the multi-threading starts => no locking.
       *
       * @param localNode node object is owned by the app, not by this store.
       */
      void setLocalNode(Node* localNode)
      {
         this->localNode = localNode;

         if(capacityPools && localNode)
            capacityPools->addIfNotExists(localNode->getNumID(), CapacityPool_LOW);
      }


   private:
      // template inliners

      /**
       * Note: unlocked (=> caller must hold read lock)
       */
      template <class TCollection, class TIterator>
      void moveIterToRandomElem(TCollection& collection,
         TIterator& iter)
      {
         iter = collection.begin();

         if(collection.size() < 2)
            return;

         int randInt = randGen.getNextInRange(0, collection.size()-1);

         while(randInt--)
            iter++;
      }

};

#endif /*NODESTORESERVERS_H_*/
