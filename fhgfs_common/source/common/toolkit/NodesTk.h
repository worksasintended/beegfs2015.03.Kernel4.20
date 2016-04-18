#ifndef NODESTK_H_
#define NODESTK_H_

#include <common/components/AbstractDatagramListener.h>
#include <common/nodes/AbstractNodeStore.h>
#include <common/nodes/NodeStoreServers.h>
#include <common/nodes/TargetStateStore.h>
#include <common/Common.h>

class NodesTk
{
   public:
      static bool waitForMgmtHeartbeat(PThread* currentThread, AbstractDatagramListener* dgramLis,
         NodeStoreServers* mgmtNodes, std::string hostname, unsigned short portUDP,
         unsigned timeoutMS=0);
      static bool downloadNodes(Node* sourceNode, NodeType nodeType, NodeList* outNodeList,
         bool silenceLog, uint16_t* outRootNumID=NULL);
      static bool downloadTargetMappings(Node* sourceNode, UInt16List* outTargetIDs,
         UInt16List* outNodeIDs, bool silenceLog);
      static bool downloadMirrorBuddyGroups(Node* sourceNode, NodeType nodeType,
         UInt16List* outBuddyGroupIDs, UInt16List* outPrimaryTargetIDs,
         UInt16List* outSecondaryTargetIDs, bool silenceLog);
      static bool downloadTargetStates(Node* sourceNode, NodeType nodeType, UInt16List* outTargetIDs,
         UInt8List* outReachabilityStates, UInt8List* outConsistencyStates, bool silenceLog);
      static bool downloadStatesAndBuddyGroups(Node* sourceNode, UInt16List* outBuddyGroupIDs,
         UInt16List* outPrimaryTargetIDs, UInt16List* outSecondaryTargetIDs,
         UInt16List* outTargetIDs, UInt8List* outTargetReachabilityStates,
         UInt8List* outTargetConsistencyStates, bool silenceLog);


      static void moveNodesFromListToStore(NodeList* nodeList, AbstractNodeStore* nodeStore);
      static void deleteListNodes(NodeList* nodeList);
      static void copyListNodes(NodeList* sourceList, NodeList* outDestList);
      static void applyLocalNicCapsToList(Node* localNode, NodeList* nodeList);

      static unsigned getRetryDelayMS(int elapsedMS);

   private:
      NodesTk() {}
};


#endif /*NODESTK_H_*/
