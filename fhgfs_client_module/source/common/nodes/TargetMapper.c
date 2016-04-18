#include "TargetMapper.h"


void TargetMapper_init(TargetMapper* this)
{
   this->rwlock = RWLock_construct();

   UInt16Map_init(&this->targets);
}

TargetMapper* TargetMapper_construct(void)
{
   TargetMapper* this = (TargetMapper*)os_kmalloc(sizeof(*this) );

   TargetMapper_init(this);

   return this;
}

void TargetMapper_uninit(TargetMapper* this)
{
   UInt16Map_uninit(&this->targets);

   RWLock_destruct(this->rwlock);
}

void TargetMapper_destruct(TargetMapper* this)
{
   TargetMapper_uninit(this);

   os_kfree(this);
}

/**
 * Note: re-maps targetID if it was mapped before.
 *
 * @return fhgfs_true if the targetID was not mapped before
 */
fhgfs_bool TargetMapper_mapTarget(TargetMapper* this, uint16_t targetID,
   uint16_t nodeID)
{
   size_t oldSize;
   size_t newSize;
   fhgfs_bool inserted;

   RWLock_writeLock(this->rwlock); // L O C K

   oldSize = UInt16Map_length(&this->targets);

   inserted = UInt16Map_insert(&this->targets, targetID, nodeID);
   if(!inserted)
   { // remove old key and re-insert
      UInt16Map_erase(&this->targets, targetID);
      UInt16Map_insert(&this->targets, targetID, nodeID);
   }

   newSize = UInt16Map_length(&this->targets);

   RWLock_writeUnlock(this->rwlock); // U N L O C K

   return (oldSize != newSize);
}

/**
 * @return fhgfs_true if the targetID was mapped
 */
fhgfs_bool TargetMapper_unmapTarget(TargetMapper* this, uint16_t targetID)
{
   fhgfs_bool keyExisted;

   RWLock_writeLock(this->rwlock); // L O C K

   keyExisted = UInt16Map_erase(&this->targets, targetID);

   RWLock_writeUnlock(this->rwlock); // U N L O C K

   return keyExisted;
}

/**
 * Applies the mapping from two separate lists (keys and values).
 *
 * Note: Does not add/remove targets from attached capacity pools.
 */
void TargetMapper_syncTargetsFromLists(TargetMapper* this, UInt16List* targetIDs,
   UInt16List* nodeIDs)
{
   UInt16ListIter targetIDsIter;
   UInt16ListIter nodeIDsIter;

   UInt16ListIter_init(&targetIDsIter, targetIDs);
   UInt16ListIter_init(&nodeIDsIter, nodeIDs);

   RWLock_writeLock(this->rwlock); // L O C K

   UInt16Map_clear(&this->targets);

   for(/* iters init'ed above */;
       !UInt16ListIter_end(&targetIDsIter);
       UInt16ListIter_next(&targetIDsIter), UInt16ListIter_next(&nodeIDsIter) )
   {
      uint16_t currentTargetID = UInt16ListIter_value(&targetIDsIter);
      uint16_t currentNodeID = UInt16ListIter_value(&nodeIDsIter);

      UInt16Map_insert(&this->targets, currentTargetID, currentNodeID);
   }

   RWLock_writeUnlock(this->rwlock); // U N L O C K
}

void TargetMapper_getTargetIDs(TargetMapper* this, UInt16List* outTargetIDs)
{
   UInt16MapIter targetsIter;

   RWLock_readLock(this->rwlock); // L O C K

   for(targetsIter = UInt16Map_begin(&this->targets);
       !UInt16MapIter_end(&targetsIter);
       UInt16MapIter_next(&targetsIter) )
   {
      uint16_t currentTargetID = UInt16MapIter_key(&targetsIter);

      UInt16List_append(outTargetIDs, currentTargetID);
   }

   RWLock_readUnlock(this->rwlock); // U N L O C K
}
