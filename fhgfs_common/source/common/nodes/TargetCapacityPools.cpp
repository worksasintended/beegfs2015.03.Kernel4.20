#include <common/nodes/TargetMapper.h>
#include <common/toolkit/MinMaxStore.h>
#include "TargetCapacityPools.h"

/**
 * @param lowSpace only used for getPoolTypeFromFreeSpace()
 * @param emergencySpace only used for getPoolTypeFromFreeSpace()
 */
TargetCapacityPools::TargetCapacityPools(bool useDynamicPools,
      const DynamicPoolLimits& poolLimitsSpace, const DynamicPoolLimits& poolLimitsInodes) :
   poolLimitsSpace(poolLimitsSpace),
   poolLimitsInodes(poolLimitsInodes),
   pools(CapacityPool_END_DONTUSE), groupedTargetPools(CapacityPool_END_DONTUSE)
{
   this->dynamicPoolsEnabled = useDynamicPools;

   this->lastRoundRobinTarget = 0;
}

/**
 * Note: Unlocked, so caller must hold the write lock when calling this.
 *
 * @param previousNodeID the node to which this target was mapped; can be 0 if it was not previously
 * mapped.
 * @param keepPool targetID won't be removed from this particular pool.
 */
void TargetCapacityPools::removeFromOthersUnlocked(uint16_t targetID, uint16_t previousNodeID,
   CapacityPoolType keepPool)
{
   /* note: targetID can only exist in one pool, so we can stop as soon as we erased an element from
      any pool */

   for(unsigned poolType=0;
       poolType < CapacityPool_END_DONTUSE;
       poolType++)
   {
      if(poolType == (unsigned)keepPool)
         continue;

      size_t numErased = pools[poolType].erase(targetID);
      if(!numErased)
         continue;

      // targetID existed in this pool, also remove from same poolType for groupedTargets

      if(previousNodeID)
      { // previous mapping existed
         groupedTargetPools[poolType][previousNodeID].erase(targetID);
      }

      return;
   }

}

/**
 * @return true if the target was either completely new or existed before but was moved to a
 * different pool now
 */
bool TargetCapacityPools::addOrUpdate(uint16_t targetID, uint16_t nodeID,
   CapacityPoolType poolType)
{
   SafeRWLock lock(&rwlock, SafeRWLock_WRITE); // L O C K write

   BEEGFS_BUG_ON_DEBUG(pools.size() != CapacityPool_END_DONTUSE,
      "invalid pools.size()");
   BEEGFS_BUG_ON_DEBUG(groupedTargetPools.size() != CapacityPool_END_DONTUSE,
      "invalid pools.size()");


   bool isNewInsertion = addOrUpdateUnlocked(targetID, nodeID, poolType);

   lock.unlock(); // U N L O C K write

   return isNewInsertion;
}

/**
 * @return true if the target was either completely new or existed before but was moved to a
 * different pool now
 */
bool TargetCapacityPools::addOrUpdateUnlocked(uint16_t targetID, uint16_t nodeID,
   CapacityPoolType poolType)
{
   /* note: map::insert().second is true if the key was inserted and false if it already existed
      (so we need to check other pools only if it was really inserted) */

   bool isNewInsertion = false;
   isNewInsertion |= pools[poolType].insert(targetID).second;
   isNewInsertion |= groupedTargetPools[poolType][nodeID].insert(targetID).second;

   if(isNewInsertion)
   { // new insertion in given pool or for given nodeID, so find and remove previous (if any)
      uint16_t previousNodeID = getNodeIDFromTargetID(targetID, targetMap);

      removeFromOthersUnlocked(targetID, previousNodeID, poolType);
   }

   targetMap[targetID] = nodeID;

   return isNewInsertion;
}

/**
 * Add the targetID only if is doesn't exist in any of the pools yet (i.e. ensure that we do not
 * change the poolType), so this is typically used if caller doesn't really know the right poolType
 * and just wants to add the target.
 *
 * @return true if this is a new target, false if the target existed already
 */
bool TargetCapacityPools::addIfNotExists(uint16_t targetID, uint16_t nodeID,
   CapacityPoolType poolType)
{
   /* note: this is called often with already existing targets, so we optimize for this case by
      having a quick path with only a read lock. */

   // quick read lock check-only path

   SafeRWLock readLock(&rwlock, SafeRWLock_READ); // L O C K read

   BEEGFS_BUG_ON_DEBUG(pools.size() != CapacityPool_END_DONTUSE,
      "invalid pools.size()");
   BEEGFS_BUG_ON_DEBUG(groupedTargetPools.size() != CapacityPool_END_DONTUSE,
      "invalid pools.size()");


   bool needUpdate = true; // whether or not we need the slow path to update something

   for(unsigned currentPoolType=0;
       currentPoolType < CapacityPool_END_DONTUSE;
       currentPoolType++)
   {
      if(pools[currentPoolType].find(targetID) == pools[currentPoolType].end() )
         continue; // not found in this pool

      // target found in this pool, check correct nodeID mapping

      uint16_t previousNodeID = getNodeIDFromTargetID(targetID, targetMap);
      if(nodeID == previousNodeID)
         needUpdate = false;

      break;
   }

   readLock.unlock(); // U N L O C K read

   if(!needUpdate)
      return false;


   // slow write lock path (because target wasn't found in quick path)

   SafeRWLock writeLock(&rwlock, SafeRWLock_WRITE); // L O C K write

   // recheck, because target might have been added after read lock release

   needUpdate = true;

   for(unsigned currentPoolType=0;
      currentPoolType < CapacityPool_END_DONTUSE;
      currentPoolType++)
   {
      if(pools[currentPoolType].find(targetID) == pools[currentPoolType].end() )
         continue; // not found in this pool

      // target found in this pool, check correct nodeID mapping

      uint16_t previousNodeID = getNodeIDFromTargetID(targetID, targetMap);
      if(nodeID == previousNodeID)
         needUpdate = false;

      // save poolType, because we don't want to change previous poolType
      poolType = (CapacityPoolType)currentPoolType;

      break;
   }

   if(needUpdate)
      addOrUpdateUnlocked(targetID, nodeID, poolType);

   writeLock.unlock(); // U N L O C K write

   return needUpdate;
}


void TargetCapacityPools::remove(uint16_t targetID)
{
   SafeRWLock lock(&rwlock, SafeRWLock_WRITE);

   for(unsigned currentPoolType = 0;
      currentPoolType < CapacityPool_END_DONTUSE;
      currentPoolType++)
   {
      size_t numErased = pools[currentPoolType].erase(targetID);
      if(!numErased)
         continue;

      // found in this pool, can only exist in grouped pools if contained in targetMap
      uint16_t nodeID = getNodeIDFromTargetID(targetID, targetMap);
      if(!nodeID)
         break;

      // found a nodeID for this target
      GroupedTargetsIter groupsIter = groupedTargetPools[currentPoolType].find(nodeID);
      if(groupsIter == groupedTargetPools[currentPoolType].end() )
         break; // node not found in this pool

      size_t numGroupedErased = groupsIter->second.erase(targetID);
      if(!numGroupedErased)
         break; // target not found in this node group

      // remove the node from this pool if this was the last target
      if(groupsIter->second.empty() )
         groupedTargetPools[currentPoolType].erase(groupsIter);

      break;
   }


   lock.unlock();
}

/**
 * @param newTargetMap needed because we can't do any calls into locked TargetMap methods here,
 * because we already have the lock path TargetMap lock -> TargetCapacityPools lock; this map will
 * be modified, so don't use it anymore after calling this method.
 */
void TargetCapacityPools::syncPoolsFromLists(UInt16List& listNormal, UInt16List& listLow,
   UInt16List& listEmergency, TargetMap& newTargetMap)
{
   /* note: it can happen that not all list elements are actually contained in the newTargetsMap;
      we skip those elements for groupedTargets, because we need to know nodes for chooser from
      different failure domains */

   // create temporary sets first without lock, then swap with lock (to keep write-lock time short)

   UInt16SetVector newPools(CapacityPool_END_DONTUSE);

   newPools[CapacityPool_NORMAL].insert(listNormal.begin(), listNormal.end() );
   newPools[CapacityPool_LOW].insert(listLow.begin(), listLow.end() );
   newPools[CapacityPool_EMERGENCY].insert(listEmergency.begin(), listEmergency.end() );

   GroupedTargetsVector newGroupedTargetPools(CapacityPool_END_DONTUSE);

   groupTargetsByNode(newPools, newTargetMap, &newGroupedTargetPools);


   // temporary sets are ready, now swap with internal sets

   SafeRWLock updateLock(&rwlock, SafeRWLock_WRITE); // L O C K (write)

   newPools.swap(pools);

   newGroupedTargetPools.swap(groupedTargetPools);
   newTargetMap.swap(targetMap);

   updateLock.unlock(); // U N L O C K (write)


   newTargetMap.clear(); // (carefully clear the map to prevent accidental further use by caller)
}

/**
 * Take pools of targetIDs and a targetMap (which maps targetIDs to nodeIDs) and fill them into
 * GroupedTargetPools.
 *
 * note: targets, for which no nodeID is defined in the targetMap will be skipped (see comment in
 * method code for reason).
 */
void TargetCapacityPools::groupTargetsByNode(const UInt16SetVector& pools,
   const TargetMap& targetMap, GroupedTargetsVector* outGroupedTargetPools)
{
   for(unsigned currentPoolType=0;
      currentPoolType < CapacityPool_END_DONTUSE;
      currentPoolType++)
   {
      // walk all targets of current pool type and group them according to targetMap

      for(UInt16SetCIter targetsIter = pools[currentPoolType].begin();
         targetsIter != pools[currentPoolType].end();
         targetsIter++)
      {
         uint16_t nodeID = getNodeIDFromTargetID(*targetsIter, targetMap);
         if(unlikely(!nodeID) )
            continue; /* skip, because targets without known nodeID would make it impossible to
                         reliably select targets in different failure domains */

         (*outGroupedTargetPools)[currentPoolType][nodeID].insert(*targetsIter);
      }
   }
}

/**
 * Create a copy of groupedTargets, but leave out the nodes contained in stripNodes.
 *
 * Note: Obviously, creating a full copy is expensive, so use this carefully.
 *
 * @outGroupedTargetsStripped copy of groupedTargets without stripNodes.
 */
void TargetCapacityPools::copyAndStripGroupedTargets(const GroupedTargets& groupedTargets,
   const UInt16Vector& stripNodes, GroupedTargets& outGroupedTargetsStripped)
{
   outGroupedTargetsStripped = groupedTargets;

   for(UInt16VectorConstIter iter = stripNodes.begin(); iter != stripNodes.end(); iter++)
      outGroupedTargetsStripped.erase(*iter);
}

/**
 * Find nodeID for given targetID in given targetMap.
 *
 * Note: This is not only called with the internal targetMap (e.g. by syncPoolsFromLists() ), that's
 * why we need targetMap as an argument.
 *
 * @return 0 if not found in targetMap, mapped nodeID otherwise.
 */
uint16_t TargetCapacityPools::getNodeIDFromTargetID(uint16_t targetID,
   const TargetMap& targetMap)
{
   TargetMapCIter targetIter = targetMap.find(targetID);

   if(unlikely(targetIter == targetMap.end() ) )
      return 0;

   return targetIter->second;
}

void TargetCapacityPools::getPoolsAsLists(UInt16List& outListNormal, UInt16List& outListLow,
   UInt16List& outListEmergency)
{
   SafeRWLock lock(&rwlock, SafeRWLock_READ); // L O C K read

   /* note: build-in multi-element list::insert is inefficient (walks list two times), thus we do
      the copy ourselves here */

   for(UInt16SetCIter iter = pools[CapacityPool_NORMAL].begin();
       iter != pools[CapacityPool_NORMAL].end();
       iter++)
      outListNormal.push_back(*iter);

   for(UInt16SetCIter iter = pools[CapacityPool_LOW].begin();
       iter != pools[CapacityPool_LOW].end();
       iter++)
      outListLow.push_back(*iter);

   for(UInt16SetCIter iter = pools[CapacityPool_EMERGENCY].begin();
       iter != pools[CapacityPool_EMERGENCY].end();
       iter++)
      outListEmergency.push_back(*iter);

   lock.unlock(); // U N L O C K read
}



/**
 * @param numTargets number of desired targets
 * @param minNumRequiredTargets the minimum number of targets the caller needs; ususally 1
 * for RAID-0 striping and 2 for mirroring (so this parameter is intended to avoid problems when
 * there is only 1 target left in the normal pool and the user has mirroring turned on).
 * @param preferredTargets may be NULL
 */
void TargetCapacityPools::chooseStorageTargets(unsigned numTargets, unsigned minNumRequiredTargets,
   UInt16List* preferredTargets, UInt16Vector* outTargets)
{
   SafeRWLock lock(&rwlock, SafeRWLock_READ);

   if(!preferredTargets || preferredTargets->empty() )
   { // no preference => start with first pool that contains any targets

      if(pools[CapacityPool_NORMAL].size() )
      {
         chooseStorageNodesNoPref(pools[CapacityPool_NORMAL],
            numTargets, outTargets);

         if(outTargets->size() >= minNumRequiredTargets)
            goto unlock_and_exit;
      }

      /* note: no "else if" here, because we want to continue with next pool if we didn't find
         enough targets for minNumRequiredTargets in previous pool */

      if(pools[CapacityPool_LOW].size() )
      {
         chooseStorageNodesNoPref(pools[CapacityPool_LOW],
            numTargets - outTargets->size(), outTargets);

         if(outTargets->size() >= minNumRequiredTargets)
            goto unlock_and_exit;
      }

      chooseStorageNodesNoPref(pools[CapacityPool_EMERGENCY],
         numTargets, outTargets);
   }
   else
   {
      // caller has preferred targets, so we can't say a priori whether nodes will be found or not
      // in a pool. our strategy here is to automatically allow non-preferred targets before
      // touching the emergency pool.

      // try normal and low pool with preferred targets...

      chooseStorageNodesWithPref(pools[CapacityPool_NORMAL],
         numTargets, preferredTargets, false, outTargets);

      if(outTargets->size() >= minNumRequiredTargets)
         goto unlock_and_exit;

      chooseStorageNodesWithPref(pools[CapacityPool_LOW],
         numTargets - outTargets->size(), preferredTargets, false, outTargets);

      if(!outTargets->empty() )
         goto unlock_and_exit;

      /* note: currently, we cannot just continue here with non-empty outTargets (even if
         "outTargets->size() < minNumRequiredTargets"), because we would need a mechanism to exclude
         the already chosen outTargets for that (e.g. like an inverted preferredTargets list). */

      // no targets yet - allow non-preferred targets before using emergency pool...

      chooseStorageNodesWithPref(pools[CapacityPool_NORMAL],
         numTargets, preferredTargets, true, outTargets);

      if(outTargets->size() >= minNumRequiredTargets)
         goto unlock_and_exit;

      chooseStorageNodesWithPref(pools[CapacityPool_LOW],
         numTargets - outTargets->size(), preferredTargets, true, outTargets);

      if(!outTargets->empty() )
         goto unlock_and_exit;

      /* still no targets available => we have to try the emergency pool (first with preference,
         then without preference) */

      chooseStorageNodesWithPref(pools[CapacityPool_EMERGENCY],
         numTargets, preferredTargets, false, outTargets);
      if(!outTargets->empty() )
         goto unlock_and_exit;

      chooseStorageNodesWithPref(pools[CapacityPool_EMERGENCY],
         numTargets, preferredTargets, true, outTargets);
   }


unlock_and_exit:

   lock.unlock();
}

/**
 * Alloc storage targets in a round-robin fashion.
 *
 * Disadvantages:
 *  - no support for preferred targets
 *     - would be really complex for multiple clients with distinct preferred targets and per-client
 *       lastTarget wouldn't lead to perfect balance)
 *  - requires write lock
 *  - lastTarget is not on a per-pool basis and hence
 *       - would not be perfectly round-robin when targets are moved to other pools
 *       - doesn't work with minNumRequiredTargets as switching pools would mess with the global
 *         lastTarget value
 *  - lastTarget is not saved across server restart
 *
 * ...so this should only be used in special scenarios and not as a general replacement for the
 * normal chooseStorageTargets() method.
 */
void TargetCapacityPools::chooseStorageTargetsRoundRobin(unsigned numTargets,
   UInt16Vector* outTargets)
{
   SafeRWLock lock(&rwlock, SafeRWLock_WRITE);

   // no preference settings supported => use first pool that contains any targets
   UInt16Set* pool;

   if(pools[CapacityPool_NORMAL].size() )
      pool = &pools[CapacityPool_NORMAL];
   else
   if(pools[CapacityPool_LOW].size() )
      pool = &pools[CapacityPool_LOW];
   else
      pool = &pools[CapacityPool_EMERGENCY];

   chooseStorageNodesNoPrefRoundRobin(*pool, numTargets, outTargets);

   lock.unlock();
}

/**
 * Select storage targets that are attached to different nodes (different failure domains).
 *
 * Note: The current implementation does not really try hard to deliver minNumRequiredTargets.
 */
void TargetCapacityPools::chooseTargetsInterdomain(unsigned numTargets,
   unsigned minNumRequiredTargets, UInt16Vector* outTargets)
{
   SafeRWLock lock(&rwlock, SafeRWLock_READ); // L O C K read

   UInt16Vector usedNodes; // to avoid using the same node twice from different pools

   for(unsigned currentPoolType = 0;
      currentPoolType < CapacityPool_END_DONTUSE;
      currentPoolType++)
   {
      if(groupedTargetPools[currentPoolType].empty() )
         continue; // we can't do anything with an empty pool

      GroupedTargets* groupedTargets = &groupedTargetPools[currentPoolType];
      GroupedTargets groupedTargetsStripped;

      if(!usedNodes.empty() )
      { /* create tmp groups: we already have some used nodes, so create grouped targets copy
           without these nodes (expensive, but at least it works with only read-lock) */
         copyAndStripGroupedTargets(groupedTargetPools[currentPoolType],
            usedNodes, groupedTargetsStripped);
         groupedTargets = &groupedTargetsStripped;
      }

      chooseTargetsInterdomainNoPref(*groupedTargets,
         numTargets, *outTargets, usedNodes);

      if(outTargets->size() >= minNumRequiredTargets)
         break; // we have enough targets for min requirement
   }

   lock.unlock(); // U N L O C K read
}

/**
 * Select storage targets that are attached to the same node/domain.
 *
 * Note: The current implementation does not really try hard to deliver minNumRequiredTargets.
 */
void TargetCapacityPools::chooseTargetsIntradomain(unsigned numTargets,
   unsigned minNumRequiredTargets, UInt16Vector* outTargets)
{
   SafeRWLock lock(&rwlock, SafeRWLock_READ); // L O C K read

   UInt16Vector usedGroups; // to avoid using the same node twice from different pools

   for(unsigned currentPoolType = 0;
      currentPoolType < CapacityPool_END_DONTUSE;
      currentPoolType++)
   {
      if(groupedTargetPools[currentPoolType].empty() )
         continue; // we can't do anything with an empty pool

      GroupedTargets* groupedTargets = &groupedTargetPools[currentPoolType];
      GroupedTargets groupedTargetsStripped;

      chooseTargetsIntradomainNoPref(*groupedTargets,
         numTargets, *outTargets, usedGroups);

      if(!usedGroups.empty() )
         break; // cancel if we have targtes (no special handling of min required currently)
   }

   lock.unlock(); // U N L O C K read
}

/**
 * Note: Unlocked (=> caller must hold read lock)
 *
 * @param outTargets might cotain less than numTargets if not enough targets are known
 */
void TargetCapacityPools::chooseStorageNodesNoPref(UInt16Set& activeTargets,
   unsigned numTargets, UInt16Vector* outTargets)
{
   if(activeTargets.empty() )
      return; // there's nothing we can do without any storage targets

   unsigned activeTargetsSize = activeTargets.size();

   if(numTargets > activeTargetsSize)
      numTargets = activeTargetsSize;

   UInt16SetIter iter;
   int partSize = activeTargetsSize / numTargets;

   moveIterToRandomElem<UInt16Set, UInt16SetIter>(activeTargets, iter);

   outTargets->reserve(numTargets);

   // for each target in numTargets
   for(unsigned i=0; i < numTargets; i++)
   {
      int rangeMin = i*partSize;
      int rangeMax = (i==(numTargets-1) ) ? (activeTargetsSize-1) : (rangeMin + partSize - 1);
      // rangeMax decision because numTargets might not devide activeTargetsSize without a remainder

      int next = randGen.getNextInRange(rangeMin, rangeMax);

      /* move iter to the chosen target, add it to outTargets, and skip the remaining targets of
         this part */

      for(int j=rangeMin; j < next; j++)
         moveIterToNextRingElem<UInt16Set, UInt16SetIter>(activeTargets, iter);

      outTargets->push_back(*iter);

      for(int j=next; j < (rangeMax+1); j++)
         moveIterToNextRingElem<UInt16Set, UInt16SetIter>(activeTargets, iter);
   }

}

/**
 * Note: Unlocked (=> caller must hold write lock)
 *
 * @param outTargets might cotain less than numTargets if not enough targets are known
 */
void TargetCapacityPools::chooseStorageNodesNoPrefRoundRobin(UInt16Set& activeTargets,
   unsigned numTargets, UInt16Vector* outTargets)
{
   if(activeTargets.empty() )
      return; // there's nothing we can do without any storage targets

   unsigned activeTargetsSize = activeTargets.size();

   if(numTargets > activeTargetsSize)
      numTargets = activeTargetsSize;

   UInt16SetIter iter = activeTargets.lower_bound(lastRoundRobinTarget);
   if(iter == activeTargets.end() )
      iter = activeTargets.begin();

   outTargets->reserve(numTargets);

   // just add the requested number of targets sequentially from the set
   for(unsigned i=0; i < numTargets; i++)
   {
      moveIterToNextRingElem<UInt16Set, UInt16SetIter>(activeTargets, iter);
      outTargets->push_back(*iter);
   }

   lastRoundRobinTarget = *iter;
}

/**
 * Select targets from different nodes (different failure domains).
 *
 * Note: Unlocked (=> caller must hold read lock)
 *
 * @param groupedTargets caller must ensure that nodes, which were previously contained in outNodes
 * from a call to a higher level pool, are not contained in this set.
 * @param outTargets might cotain less than numTargets if not enough targets are known.
 * @param outNodes is relevant if this is called again with another pool to ensure that no targets
 * from an already used node are selected.
 */
void TargetCapacityPools::chooseTargetsInterdomainNoPref(GroupedTargets& groupedTargets,
   unsigned numTargets, UInt16Vector& outTargets, UInt16Vector& outNodes)
{
   if(groupedTargets.empty() )
      return; // there's nothing we can do without any storage targets

   unsigned groupedTargetsSize = groupedTargets.size();

   if(numTargets > groupedTargetsSize)
      numTargets = groupedTargetsSize;

   outTargets.reserve(numTargets);
   outNodes.reserve(numTargets);

   // move iter to random node, select random target from this node and go to next sequential node

   GroupedTargetsCIter groupsIter;

   moveIterToRandomElem(groupedTargets, groupsIter);

   for(unsigned i=0; i < numTargets; i++)
   {
      // select random target from current node

      UInt16SetCIter targetsIter;

      BEEGFS_BUG_ON_DEBUG(groupsIter->second.empty(), "found node without targets");

      moveIterToRandomElem(groupsIter->second, targetsIter);

      outTargets.push_back(*targetsIter);
      outNodes.push_back(groupsIter->first);

      // move on to next sequential node
      moveIterToNextRingElem(groupedTargets, groupsIter);
   }

}

/**
 * Select targets on same node (same domain).
 *
 * Note: Unlocked (=> caller must hold read lock)
 *
 * @param groupedTargets the set of available domains.
 * @param outTargets might cotain less than numTargets if not enough targets are known selected
 * domain.
 * @param outGroups contains the selected domain from the set of available domains.
 */
void TargetCapacityPools::chooseTargetsIntradomainNoPref(GroupedTargets& groupedTargets,
   unsigned numTargets, UInt16Vector& outTargets, UInt16Vector& outGroups)
{
   if(groupedTargets.empty() )
      return; // there's nothing we can do without any storage targets

   // move iter to random group

   GroupedTargetsCIter groupsIter;

   moveIterToRandomElem(groupedTargets, groupsIter);

   BEEGFS_BUG_ON_DEBUG(groupsIter->second.empty(), "found group without targets");

   outGroups.push_back(groupsIter->first);

   // adjust max number of targets that we can get from this group

   unsigned numTargetsInGroup = groupsIter->second.size();

   if(numTargets > numTargetsInGroup)
      numTargets = numTargetsInGroup;

   outTargets.reserve(numTargets);

   // select random first target from group and go on with next sequential targets

   UInt16SetCIter targetsIter;

   moveIterToRandomElem(groupsIter->second, targetsIter);

   outTargets.push_back(*targetsIter);

   // select remaining sequential targets from this group

   for(unsigned i=1; i < numTargets; i++)
   {
      moveIterToNextRingElem(groupsIter->second, targetsIter);

      outTargets.push_back(*targetsIter);
   }

}

/**
 * Note: Unlocked (=> caller must hold read lock)
 *
 * Note: This method assumes that there really is at least one preferred target and that those
 * targets are probably active. So do not use it as a replacement for the version without preferred
 * targets!
 *
 * @param allowNonPreferredTargets true to enable use of non-preferred targets if not enough
 *    preferred targets are active to satisfy numTargets
 * @param outTargets might cotain less than numTargets if not enough targets are known
 */
void TargetCapacityPools::chooseStorageNodesWithPref(UInt16Set& activeTargets,
   unsigned numTargets, UInt16List* preferredTargets, bool allowNonPreferredTargets,
   UInt16Vector* outTargets)
{
   if(activeTargets.empty() )
      return; // there's nothing we can do without any storage targets

   unsigned activeTargetsSize = activeTargets.size();

   // max number of targets is limited by the number of known active targets
   if(numTargets > activeTargetsSize)
      numTargets = activeTargetsSize;

   // Stage 1: add all the preferred targets that are actually available to the outTargets

   // note: we use a separate set for the outTargets here to quickly find out (in stage 2) whether
   // we already added a certain node from the preferred targets (in stage 1)

   UInt16Set outTargetsSet; // (see note above)
   UInt16ListIter preferredIter;
   UInt16SetIter activeTargetsIter; // (will be re-used in stage 2)

   moveIterToRandomElem<UInt16List, UInt16ListIter>(*preferredTargets, preferredIter);

   outTargets->reserve(numTargets);

   // walk over all the preferred targets and add them to outTargets when they are available
   // (note: iterTmp is used to avoid calling preferredTargets->size() )
   for(UInt16ListIter iterTmp = preferredTargets->begin();
       (iterTmp != preferredTargets->end() ) && numTargets;
       iterTmp++)
   {
      activeTargetsIter = activeTargets.find(*preferredIter);

      if(activeTargetsIter != activeTargets.end() )
      { // this preferred node is active => add to outTargets and to outTargetsSet
         outTargets->push_back(*preferredIter);
         outTargetsSet.insert(*preferredIter);

         numTargets--;
      }

      moveIterToNextRingElem<UInt16List, UInt16ListIter>(*preferredTargets, preferredIter);
   }

   // Stage 2: add the remaining requested number of targets from the active targets

   // if numTargets is greater than 0 then we have some requested targets left, that could not be
   // taken from the preferred targets

   // we keep it simple here, because usually there will be enough preferred targets available,
   // so that this case is quite unlikely

   if(allowNonPreferredTargets && numTargets)
   {
      UInt16SetIter outTargetsSetIter;

      moveIterToRandomElem(activeTargets, activeTargetsIter);

      // while we haven't found the number of requested targets
      while(numTargets)
      {
         outTargetsSetIter = outTargetsSet.find(*activeTargetsIter);
         if(outTargetsSetIter == outTargetsSet.end() )
         {
            outTargets->push_back(*activeTargetsIter);
            outTargetsSet.insert(*activeTargetsIter);

            numTargets--;
         }

         moveIterToNextRingElem(activeTargets, activeTargetsIter);
      }
   }

}

/**
 * get the pool type to which a targetID is currently assigned.
 *
 * @return false if targetID not found.
 */
bool TargetCapacityPools::getPoolAssignment(uint16_t targetID, CapacityPoolType* outPoolType)
{
   SafeRWLock lock(&rwlock, SafeRWLock_READ); // L O C K read

   bool targetFound = false;

   *outPoolType = CapacityPool_EMERGENCY;

   for(unsigned poolType=0;
       poolType < CapacityPool_END_DONTUSE;
       poolType++)
   {
      UInt16SetCIter iter = pools[poolType].find(targetID);

      if(iter != pools[poolType].end() )
      {
         *outPoolType = (CapacityPoolType)poolType;
         targetFound = true;

         break;
      }
   }

   lock.unlock(); // U N L O C K read

   return targetFound;
}

/**
 * Return internal state as human-readable string.
 *
 * Note: Very expensive, only intended for debugging.
 */
void TargetCapacityPools::getStateAsStr(std::string& outStateStr)
{
   SafeRWLock lock(&rwlock, SafeRWLock_READ); // L O C K

   std::ostringstream stateStream;

   stateStream << " lastRoundRobinTarget: " << lastRoundRobinTarget << std::endl;
   stateStream << " lowSpace: " << this->poolLimitsSpace.getLowLimit() << std::endl;
   stateStream << " emergencySpace: " << this->poolLimitsSpace.getEmergencyLimit() << std::endl;
   stateStream << " lowInodes: " << this->poolLimitsInodes.getLowLimit() << std::endl;
   stateStream << " emergencyInodes: " << this->poolLimitsInodes.getEmergencyLimit() << std::endl;
   stateStream << " dynamicLimits: " << (this->dynamicPoolsEnabled ? "on" : "off") << std::endl;

   if(this->dynamicPoolsEnabled)
   {
      stateStream << "  normalSpaceSpreadThreshold: " <<
            this->poolLimitsSpace.getNormalSpreadThreshold() << std::endl;
      stateStream << "  lowSpaceSpreadThreshold: " <<
            this->poolLimitsSpace.getLowSpreadThreshold() << std::endl;
      stateStream << "  lowSpaceDynamicLimit: " <<
            this->poolLimitsSpace.getLowDynamicLimit() << std::endl;
      stateStream << "  emergencySpaceDynamicLimit: " <<
            this->poolLimitsSpace.getEmergencyDynamicLimit() << std::endl;

      stateStream << "  normalInodesSpreadThreshold: " <<
            this->poolLimitsInodes.getNormalSpreadThreshold() << std::endl;
      stateStream << "  lowInodesSpreadThreshold: " <<
            this->poolLimitsInodes.getLowSpreadThreshold() << std::endl;
      stateStream << "  lowInodesDynamicLimit: " <<
            this->poolLimitsInodes.getLowDynamicLimit() << std::endl;
      stateStream << "  emergencyInodesDynamicLimit: " <<
            this->poolLimitsInodes.getEmergencyDynamicLimit() << std::endl;
   }

   stateStream << std::endl;

   for(unsigned poolType=0;
       poolType < CapacityPool_END_DONTUSE;
       poolType++)
   {
      stateStream << " pool " << poolTypeToStr( (CapacityPoolType) poolType) << ":" <<
         std::endl;

      stateStream << "  plain pool contents:" << std::endl;

      stateStream << "   ";

      for(UInt16SetCIter iter = pools[poolType].begin();
          iter != pools[poolType].end();
          iter++)
      {
         stateStream << *iter << " ";
      }

      stateStream << std::endl;

      stateStream << "  grouped pool contents:" << std::endl;

      for(GroupedTargetsCIter groupsIter = groupedTargetPools[poolType].begin();
          groupsIter != groupedTargetPools[poolType].end();
           groupsIter++)
      {
         stateStream << "   nodeID: " << groupsIter->first << std::endl;

         stateStream << "    "; // intend before first targetID

         for(UInt16SetCIter targetsIter = groupsIter->second.begin();
            targetsIter != groupsIter->second.end();
            targetsIter++)
         {
            stateStream << *targetsIter << " ";
         }

         stateStream << std::endl;
      }

      stateStream << std::endl;
   }


   lock.unlock(); // U N L O C K

   outStateStr = stateStream.str();
}

/**
 * @return pointer to static string buffer (no free needed)
 */
const char* TargetCapacityPools::poolTypeToStr(CapacityPoolType poolType)
{
   switch(poolType)
   {
      case CapacityPool_NORMAL:
      {
         return "Normal";
      } break;

      case CapacityPool_LOW:
      {
         return "Low";
      } break;

      case CapacityPool_EMERGENCY:
      {
         return "Emergency";
      } break;

      default:
      {
         return "Unknown/Invalid";
      } break;
   }
}
