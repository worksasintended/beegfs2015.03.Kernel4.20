#include <common/nodes/TargetMapper.h>

#include "MirrorBuddyGroupMapper.h"

void MirrorBuddyGroupMapper_init(MirrorBuddyGroupMapper* this)
{
   this->isValid = fhgfs_true;

   this->rwlock = RWLock_construct();

   if (unlikely(!this->rwlock))
      this->isValid = fhgfs_false;

   MirrorBuddyGroupMap_init(&this->mirrorBuddyGroups);
}

MirrorBuddyGroupMapper* MirrorBuddyGroupMapper_construct(void)
{
   MirrorBuddyGroupMapper* this = (MirrorBuddyGroupMapper*)os_kmalloc(sizeof(*this) );

   if (likely(this))
      MirrorBuddyGroupMapper_init(this);

   return this;
}

void MirrorBuddyGroupMapper_uninit(MirrorBuddyGroupMapper* this)
{
   // clear map (elements have to be destructed)
   MirrorBuddyGroupMapIter buddyGroupMapIter;

   buddyGroupMapIter = MirrorBuddyGroupMap_begin(&this->mirrorBuddyGroups);
   for(/* iter init'ed above */;
       !MirrorBuddyGroupMapIter_end(&buddyGroupMapIter);
       MirrorBuddyGroupMapIter_next(&buddyGroupMapIter) )
   {
      MirrorBuddyGroup* buddyGroup = MirrorBuddyGroupMapIter_value(&buddyGroupMapIter);
      MirrorBuddyGroup_destruct(buddyGroup);
   }

   MirrorBuddyGroupMap_uninit(&this->mirrorBuddyGroups);

   SAFE_DESTRUCT(this->rwlock, RWLock_destruct);
}

void MirrorBuddyGroupMapper_destruct(MirrorBuddyGroupMapper* this)
{
   MirrorBuddyGroupMapper_uninit(this);

   os_kfree(this);
}

/**
 * @return 0 if group ID not found
 */
uint16_t MirrorBuddyGroupMapper_getPrimaryTargetID(MirrorBuddyGroupMapper* this,
   uint16_t mirrorBuddyGroupID)
{
   MirrorBuddyGroup* buddyGroup;
   uint16_t targetID;

   RWLock_readLock(this->rwlock); // L O C K

   buddyGroup = __MirrorBuddyGroupMapper_getMirrorBuddyGroupUnlocked(this, mirrorBuddyGroupID);

   if(likely(buddyGroup))
      targetID = buddyGroup->firstTargetID;
   else
      targetID = 0;

   RWLock_readUnlock(this->rwlock); // U N L O C K

   return targetID;
}

/**
 * @return 0 if group ID not found
 */
uint16_t MirrorBuddyGroupMapper_getSecondaryTargetID(MirrorBuddyGroupMapper* this,
   uint16_t mirrorBuddyGroupID)
{
   MirrorBuddyGroup* buddyGroup;
   uint16_t targetID;

   RWLock_readLock(this->rwlock); // L O C K

   buddyGroup = __MirrorBuddyGroupMapper_getMirrorBuddyGroupUnlocked(this, mirrorBuddyGroupID);

   if(likely(buddyGroup))
      targetID = buddyGroup->secondTargetID;
   else
      targetID = 0;

   RWLock_readUnlock(this->rwlock); // U N L O C K

   return targetID;
}

/**
 * NOTE: no sanity checks in here
 */
void MirrorBuddyGroupMapper_syncGroupsFromLists(MirrorBuddyGroupMapper* this,
   UInt16List* buddyGroupIDs, UInt16List* primaryTargets, UInt16List* secondaryTargets)
{
   RWLock_writeLock(this->rwlock); // L O C K

   __MirrorBuddyGroupMapper_syncGroupsFromListsUnlocked(this, buddyGroupIDs,
      primaryTargets, secondaryTargets);

   RWLock_writeUnlock(this->rwlock); // U N L O C K
}


/**
 * note: caller must hold writelock.
 */
void __MirrorBuddyGroupMapper_syncGroupsFromListsUnlocked(MirrorBuddyGroupMapper* this,
   UInt16List* buddyGroupIDs, UInt16List* primaryTargets, UInt16List* secondaryTargets)
{
   UInt16ListIter buddyGroupIDsIter;
   UInt16ListIter primaryTargetsIter;
   UInt16ListIter secondaryTargetsIter;

   MirrorBuddyGroupMapIter buddyGroupMapIter;

   UInt16ListIter_init(&buddyGroupIDsIter, buddyGroupIDs);
   UInt16ListIter_init(&primaryTargetsIter, primaryTargets);
   UInt16ListIter_init(&secondaryTargetsIter, secondaryTargets);

   // clear map (elements have to be destructed)
   buddyGroupMapIter = MirrorBuddyGroupMap_begin(&this->mirrorBuddyGroups);
   for(/* iter init'ed above */;
       !MirrorBuddyGroupMapIter_end(&buddyGroupMapIter);
       MirrorBuddyGroupMapIter_next(&buddyGroupMapIter) )
   {
      MirrorBuddyGroup* buddyGroup = MirrorBuddyGroupMapIter_value(&buddyGroupMapIter);
      MirrorBuddyGroup_destruct(buddyGroup);
   }

   MirrorBuddyGroupMap_clear(&this->mirrorBuddyGroups);

   for(/* iters init'ed above */;
       !UInt16ListIter_end(&buddyGroupIDsIter);
       UInt16ListIter_next(&buddyGroupIDsIter), UInt16ListIter_next(&primaryTargetsIter),
          UInt16ListIter_next(&secondaryTargetsIter) )
   {
      uint16_t currentBuddyGroupID = UInt16ListIter_value(&buddyGroupIDsIter);
      uint16_t currentPrimaryTargetID = UInt16ListIter_value(&primaryTargetsIter);
      uint16_t currentSecondaryTargetID = UInt16ListIter_value(&secondaryTargetsIter);

      MirrorBuddyGroup* buddyGroup = MirrorBuddyGroup_constructFromTargetIDs(currentPrimaryTargetID,
         currentSecondaryTargetID);

      if(unlikely(!buddyGroup))
      {
         printk_fhgfs(KERN_INFO, "%s:%d: Failed to allocate memory for MirrorBuddyGroup; some "
            "entries could not be processed", __func__, __LINE__);
         // doesn't make sense to go further here
         break;
      }

      MirrorBuddyGroupMap_insert(&this->mirrorBuddyGroups, currentBuddyGroupID, buddyGroup);
   }

}

/**
 * Adds a new buddy group to the map.
 * @param buddyGroupID The ID of the new buddy group. Must be non-zero.
 * @param allowUpdate Allow updating an existing buddy group.
 */
FhgfsOpsErr MirrorBuddyGroupMapper_addGroup(MirrorBuddyGroupMapper* this,
   TargetMapper* targetMapper, uint16_t buddyGroupID, uint16_t primaryTargetID,
   uint16_t secondaryTargetID, fhgfs_bool allowUpdate)
{
   FhgfsOpsErr res = FhgfsOpsErr_SUCCESS;

   uint16_t primaryInGroup;
   uint16_t secondaryInGroup;
   MirrorBuddyGroup* buddyGroup;

   // ID = 0 is an error.
   if (buddyGroupID == 0)
      return FhgfsOpsErr_INVAL;

   RWLock_writeLock(this->rwlock); // L O C K

   if (!allowUpdate)
   {
      // If group already exists return error.
      MirrorBuddyGroupMapIter iter =
         MirrorBuddyGroupMap_find(&this->mirrorBuddyGroups, buddyGroupID);

      if (!MirrorBuddyGroupMapIter_end(&iter) )
      {
         res = FhgfsOpsErr_EXISTS;
         goto unlock;
      }
   }

   // Check if both targets exist.
   if (  TargetMapper_getNodeID(targetMapper, primaryTargetID) == 0
      || TargetMapper_getNodeID(targetMapper, secondaryTargetID) == 0)
   {
      res = FhgfsOpsErr_UNKNOWNTARGET;
      goto unlock;
   }

   // Check that both targets are not yet part of any group.
   primaryInGroup = __MirrorBuddyGroupMapper_getBuddyGroupIDUnlocked(this, primaryTargetID);
   secondaryInGroup = __MirrorBuddyGroupMapper_getBuddyGroupIDUnlocked(this, secondaryTargetID);

   if (  ( (primaryInGroup != 0) && (primaryInGroup != buddyGroupID) )
      || ( (secondaryInGroup != 0) && (secondaryInGroup != buddyGroupID) ) )
   {
      res = FhgfsOpsErr_INUSE;
      goto unlock;
   }

   // Create and insert new mirror buddy group.
   buddyGroup = MirrorBuddyGroup_constructFromTargetIDs(primaryTargetID, secondaryTargetID);

   if (unlikely(!buddyGroup) )
   {
      printk_fhgfs(KERN_INFO, "%s:%d: Failed to allocate memory for MirrorBuddyGroup.",
         __func__, __LINE__);
      res = FhgfsOpsErr_OUTOFMEM;
      goto unlock;
   }

   MirrorBuddyGroupMap_insert(&this->mirrorBuddyGroups, buddyGroupID, buddyGroup);

unlock:
   RWLock_writeUnlock(this->rwlock); // U N L O C K

   return res;
}


/**
 * @return a pointer to the buddy group in the map or NULL if key does not exist
 *
 * NOTE: no locks, so caller should hold an RWLock
 */
MirrorBuddyGroup* __MirrorBuddyGroupMapper_getMirrorBuddyGroupUnlocked(MirrorBuddyGroupMapper* this,
   uint16_t mirrorBuddyGroupID)
{
   MirrorBuddyGroup* buddyGroup = NULL;

   MirrorBuddyGroupMapIter iter;

   iter = MirrorBuddyGroupMap_find(&this->mirrorBuddyGroups, mirrorBuddyGroupID);
   if(likely(!MirrorBuddyGroupMapIter_end(&iter)))
      buddyGroup = MirrorBuddyGroupMapIter_value(&iter);

   return buddyGroup;
}

uint16_t __MirrorBuddyGroupMapper_getBuddyGroupIDUnlocked(MirrorBuddyGroupMapper* this,
   uint16_t targetID)
{
   uint16_t buddyGroupID = 0;
   MirrorBuddyGroupMapIter buddyGroupMapIter;

   for (buddyGroupMapIter = MirrorBuddyGroupMap_begin(&this->mirrorBuddyGroups);
        !MirrorBuddyGroupMapIter_end(&buddyGroupMapIter);
        MirrorBuddyGroupMapIter_next(&buddyGroupMapIter) )
   {
      MirrorBuddyGroup* buddyGroup = MirrorBuddyGroupMapIter_value(&buddyGroupMapIter);
      if (buddyGroup->firstTargetID == targetID || buddyGroup->secondTargetID == targetID)
      {
         targetID = MirrorBuddyGroupMapIter_key(&buddyGroupMapIter);
         break;
      }
   }

   return buddyGroupID;
}
