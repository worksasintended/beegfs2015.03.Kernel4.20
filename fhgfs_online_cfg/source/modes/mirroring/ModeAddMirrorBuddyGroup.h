#ifndef MODEADDMIRRORBUDDYGROUP_H_
#define MODEADDMIRRORBUDDYGROUP_H_

#include <common/nodes/MirrorBuddyGroupCreator.h>
#include <common/Common.h>
#include <modes/Mode.h>



class ModeAddMirrorBuddyGroup : public Mode, public MirrorBuddyGroupCreator
{
   public:
      ModeAddMirrorBuddyGroup()
      {
         cfgAutomatic = false;
         cfgForce = false;
         cfgDryrun = false;
         cfgUnique = false;
      }

      virtual int execute();

      static void printHelp();


   private:
      bool cfgAutomatic;
      bool cfgForce; // ignore warnings/errors during automatic mode
      bool cfgDryrun; // only print the selected configuration
      bool cfgUnique; // use unique MirrorBuddyGroupIDs. The ID is not used as storage targetNumID

      FhgfsOpsErr doAutomaticMode(NodeStore* mgmtNodes, NodeStore* storageNodes, bool doForce,
         bool doDryrun, bool useUniqueGroupIDs);
      void printMirrorBuddyGroups(NodeStore* storageNodes, TargetMapper* targetMapper,
         UInt16List* buddyGroupIDs, UInt16List* primaryTargetIDs, UInt16List* secondaryTargetIDs);
      void printMirrorBuddyGroups(NodeStore* storageNodes, TargetMapper* targetMapper,
         UInt16List* buddyGroupIDs, MirrorBuddyGroupList* buddyGroups);
      void printAutomaticResults(FhgfsOpsErr retValGeneration, bool doForce,
         NodeStore* storageNodes, TargetMapper* targetMapper, UInt16List* newBuddyGroupIDs,
         MirrorBuddyGroupList* newBuddyGroups, UInt16List* oldBuddyGroupIDs,
         UInt16List* oldPrimaryTargetIDs, UInt16List* oldSecondaryTargetIDs);
};


#endif /* MODEADDMIRRORBUDDYGROUP_H_ */
