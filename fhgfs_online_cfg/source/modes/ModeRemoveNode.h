#ifndef MODEREMOVENODE_H_
#define MODEREMOVENODE_H_

#include <common/storage/StorageErrors.h>
#include <common/Common.h>
#include "Mode.h"


class ModeRemoveNode : public Mode
{
   public:
      ModeRemoveNode()
      {
         cfgNodeID = "<undefined>";
         cfgRemoveUnreachable = false;
         cfgReachabilityNumRetries = 6; // (>= 1)
         cfgReachabilityRetryTimeoutMS = 500;
         cfgForce = false;
      }
      
      virtual int execute();
   
      static void printHelp();

      
   private:
      std::string cfgNodeID;
      bool cfgRemoveUnreachable;
      unsigned cfgReachabilityNumRetries; // (>= 1)
      unsigned cfgReachabilityRetryTimeoutMS;
      bool cfgForce;

      int removeSingleNode(std::string nodeID, uint16_t nodeNumID, NodeType nodeType);
      int removeUnreachableNodes(NodeType nodeType);
      FhgfsOpsErr removeNodeComm(std::string nodeID, uint16_t nodeNumID, NodeType nodeType);
      FhgfsOpsErr isPartOfMirrorBuddyGroup(Node* mgmtNode, uint16_t nodeNumID, NodeType nodeType,
         bool* outIsPartOfMBG);
};


#endif /* MODEREMOVENODE_H_ */
