#ifndef MIRRORBUDDYGROUPMAPPER_H_
#define MIRRORBUDDYGROUPMAPPER_H_

#include <common/nodes/TargetMapper.h>
#include <common/threading/RWLock.h>
#include <common/toolkit/list/UInt16ListIter.h>
#include <common/toolkit/tree/MirrorBuddyGroupMapIter.h>
#include <common/toolkit/StringTk.h>
#include <common/storage/StorageErrors.h>

struct MirrorBuddyGroupMapper;
typedef struct MirrorBuddyGroupMapper MirrorBuddyGroupMapper;

extern void MirrorBuddyGroupMapper_init(MirrorBuddyGroupMapper* this);
extern MirrorBuddyGroupMapper* MirrorBuddyGroupMapper_construct(void);
extern void MirrorBuddyGroupMapper_uninit(MirrorBuddyGroupMapper* this);
extern void MirrorBuddyGroupMapper_destruct(MirrorBuddyGroupMapper* this);

extern uint16_t MirrorBuddyGroupMapper_getPrimaryTargetID(MirrorBuddyGroupMapper* this,
   uint16_t mirrorBuddyGroupID);
extern uint16_t MirrorBuddyGroupMapper_getSecondaryTargetID(MirrorBuddyGroupMapper* this,
   uint16_t mirrorBuddyGroupID);
extern void MirrorBuddyGroupMapper_syncGroupsFromLists(MirrorBuddyGroupMapper* this,
   UInt16List* buddyGroupIDs, UInt16List* primaryTargets, UInt16List* secondaryTargets);
extern FhgfsOpsErr MirrorBuddyGroupMapper_addGroup(MirrorBuddyGroupMapper* this,
   TargetMapper* targetMapper, uint16_t buddyGroupID, uint16_t primaryTargetID,
   uint16_t secondaryTargetID, fhgfs_bool allowUpdate);

extern MirrorBuddyGroup* __MirrorBuddyGroupMapper_getMirrorBuddyGroupUnlocked(
   MirrorBuddyGroupMapper* this, uint16_t mirrorBuddyGroupID);
extern uint16_t __MirrorBuddyGroupMapper_getBuddyGroupIDUnlocked(MirrorBuddyGroupMapper* this,
   uint16_t targetID);
extern void __MirrorBuddyGroupMapper_syncGroupsFromListsUnlocked(MirrorBuddyGroupMapper* this,
   UInt16List* buddyGroupIDs, UInt16List* primaryTargets, UInt16List* secondaryTargets);

static inline fhgfs_bool MirrorBuddyGroupMapper_getValid(MirrorBuddyGroupMapper* this);

struct MirrorBuddyGroupMapper
{
   // friend class TargetStateStore; // for atomic update of state change plus mirror group switch

   RWLock* rwlock;
   MirrorBuddyGroupMap mirrorBuddyGroups;
   fhgfs_bool isValid;
};

fhgfs_bool MirrorBuddyGroupMapper_getValid(MirrorBuddyGroupMapper* this)
{
   return this->isValid;
}


#endif /* MIRRORBUDDYGROUPMAPPER_H_ */
