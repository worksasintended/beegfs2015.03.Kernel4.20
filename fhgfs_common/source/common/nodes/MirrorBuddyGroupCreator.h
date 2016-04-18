#ifndef COMMON_NODES_MIRRORBUDDYGROUPCREATOR_H_
#define COMMON_NODES_MIRRORBUDDYGROUPCREATOR_H_

#include <common/Common.h>
#include <common/nodes/MirrorBuddyGroupMapper.h>
#include <common/nodes/NodeStore.h>



// key: ServerNumID, value: counter of primary targets
typedef std::map<uint16_t, size_t> PrimaryTargetCounterMap;
typedef PrimaryTargetCounterMap::iterator PrimaryTargetCounterMapIter;
typedef PrimaryTargetCounterMap::const_iterator PrimaryTargetCounterMapCIter;
typedef PrimaryTargetCounterMap::value_type PrimaryTargetCounterMapVal;


class MirrorBuddyGroupCreator
{
   public:
      MirrorBuddyGroupCreator() {};

      FhgfsOpsErr addGroup(NodeStore* mgmtNodes, uint16_t primaryTargetID,
         uint16_t secondaryTargetID, uint16_t forcedGroupID);
      static uint16_t generateID(TargetMapper* targetMapper, UInt16List* usedMirrorBuddyGroups);


   protected:
      static FhgfsOpsErr addGroupComm(NodeStore* mgmtNodes, uint16_t primaryTargetID,
         uint16_t secondaryTargetID, uint16_t forcedGroupID, uint16_t& outNewGroupID);
      bool removeTargetsFromExistingMirrorBuddyGroups(TargetMapper* localTargetMapper,
         UInt16List* oldBuddyGroupIDs, UInt16List* oldPrimaryTargetIDs,
         UInt16List* oldSecondaryTargetIDs);
      uint16_t findNextTarget(NodeStore* storageNodes, TargetMapper* localTargetMapper,
         uint16_t nodeNumIdToIgnore);
      bool checkSizeOfTargets(NodeStoreServers* storageServer, TargetMapper* mapper);
      FhgfsOpsErr generateMirrorBuddyGroups(NodeStoreServers* storageServer,
         TargetMapper* systemTargetMapper, UInt16List* outBuddyGroupIDs,
         MirrorBuddyGroupList* outBuddyGroups, TargetMapper* localTargetMapper,
         UInt16List* usedMirrorBuddyGroupIDs, bool useUniqueGroupIDs);
      void selectPrimaryTarget(TargetMapper* targetMapper, PrimaryTargetCounterMap* primaryUsed,
         uint16_t* inOutPrimaryTargetID, uint16_t* inOutSecondaryTargetID);
      bool createMirrorBuddyGroups(NodeStore* mgmtNodes, FhgfsOpsErr retValGeneration, bool doForce,
         bool doDryrun, UInt16List* buddyGroupIDs, MirrorBuddyGroupList* buddyGroups);
};

#endif /* COMMON_NODES_MIRRORBUDDYGROUPCREATOR_H_ */
