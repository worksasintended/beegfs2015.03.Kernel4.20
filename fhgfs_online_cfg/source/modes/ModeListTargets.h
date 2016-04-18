#ifndef MODELISTTARGETS_H_
#define MODELISTTARGETS_H_

#include <common/Common.h>
#include <common/net/message/nodes/GetNodeCapacityPoolsMsg.h>
#include <common/nodes/Node.h>
#include <common/nodes/NodeStore.h>
#include "Mode.h"


class ModeListTargets : public Mode
{
   public:
      ModeListTargets()
      {
         cfgPrintNodesFirst = false;
         cfgPrintLongNodes = false;
         cfgHideNodeID = false;
         cfgPrintState = false;
         cfgPrintCapacityPools = false;
         cfgPrintSpaceInfo = false;
         cfgGetFromMeta = false;
         cfgUseBuddyGroups = false;
         cfgNodeType = NODETYPE_Storage;
         cfgPoolQueryType = CapacityPoolQuery_STORAGE;

         poolSourceNodeID = 0;
         poolSourceNodeStore = NULL;
      }

      virtual int execute();

      static void printHelp();


   protected:
      bool cfgPrintNodesFirst;
      bool cfgPrintLongNodes;
      bool cfgHideNodeID;
      bool cfgPrintState;
      bool cfgPrintCapacityPools;
      bool cfgPrintSpaceInfo;
      bool cfgGetFromMeta;
      bool cfgUseBuddyGroups;
      NodeType cfgNodeType;
      CapacityPoolQueryType cfgPoolQueryType;

   private:
      uint16_t poolSourceNodeID; // where we get our data from
      NodeStoreServers* poolSourceNodeStore; // contains node where we get our data from
      UInt16List poolNormalList;
      UInt16List poolLowList;
      UInt16List poolEmergencyList;

      UInt16List buddyGroupIDs;
      UInt16List buddyGroupPrimaryTargetIDs;
      UInt16List buddyGroupSecondaryTargetIDs;

      int checkConfig();
      int prepareData(Node* mgmtNode);
      bool getPoolsComm(NodeStoreServers* nodes, uint16_t nodeID,
         CapacityPoolQueryType poolQueryType);

      int print();
      int printHeader();
      int printTarget(uint16_t targetID, bool isPrimaryTarget, Node* node, uint16_t buddyGroupID,
         uint16_t buddyTargetID);
      int addBuddyGroupToLine(char* inOutString, int* inOutOffset, uint16_t currentBuddyGroupID,
         bool isPrimaryTarget);
      int addTargetIDToLine(char* inOutString, int* inOutOffset, uint16_t currentTargetID);
      int addNodeIDToLine(char* inOutString, int* inOutOffset, Node* node);
      int addStateToLine(char* inOutString, int* inOutOffset, uint16_t currentTargetID,
         uint16_t buddyTargetID);
      int addPoolToLine(char* inOutString, int* inOutOffset, uint16_t currentTargetID);
      int addSpaceToLine(char* inOutString, int* inOutOffset, uint16_t currentTargetID, Node* node);
};

#endif /* MODELISTTARGETS_H_ */
