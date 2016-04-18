#ifndef MIRRORBUDDYGROUP_H
#define MIRRORBUDDYGROUP_H

#include <common/Common.h>

struct MirrorBuddyGroup;
typedef struct MirrorBuddyGroup MirrorBuddyGroup;

static inline void MirrorBuddyGroup_init(MirrorBuddyGroup* this);
static inline void MirrorBuddyGroup_initFromTargetIDs(MirrorBuddyGroup* this,
   uint16_t firstTargetID, uint16_t secondTargetID);
static inline MirrorBuddyGroup* MirrorBuddyGroup_construct(void);
static inline MirrorBuddyGroup* MirrorBuddyGroup_constructFromTargetIDs(uint16_t firstTargetID,
   uint16_t secondTargetID);
static inline void MirrorBuddyGroup_uninit(MirrorBuddyGroup* this);
static inline void MirrorBuddyGroup_destruct(MirrorBuddyGroup* this);

struct MirrorBuddyGroup
{
   uint16_t firstTargetID;
   uint16_t secondTargetID;
};

void MirrorBuddyGroup_init(MirrorBuddyGroup* this)
{
   this->firstTargetID = 0;
   this->secondTargetID = 0;
}

void MirrorBuddyGroup_initFromTargetIDs(MirrorBuddyGroup* this, uint16_t firstTargetID,
   uint16_t secondTargetID)
{
   this->firstTargetID = firstTargetID;
   this->secondTargetID = secondTargetID;
}

struct MirrorBuddyGroup* MirrorBuddyGroup_construct(void)
{
   struct MirrorBuddyGroup* this = (MirrorBuddyGroup*) os_kmalloc(sizeof(*this));

   if (likely(this))
      MirrorBuddyGroup_init(this);

   return this;
}

struct MirrorBuddyGroup* MirrorBuddyGroup_constructFromTargetIDs(uint16_t firstTargetID,
   uint16_t secondTargetID)
{
   struct MirrorBuddyGroup* this = (MirrorBuddyGroup*) os_kmalloc(sizeof(*this));

   if (likely(this))
      MirrorBuddyGroup_initFromTargetIDs(this, firstTargetID, secondTargetID);

   return this;
}

void MirrorBuddyGroup_uninit(MirrorBuddyGroup* this)
{
   // nothing to be done here
}

void MirrorBuddyGroup_destruct(MirrorBuddyGroup* this)
{
   MirrorBuddyGroup_uninit(this);

   os_kfree(this);
}

#endif /* MIRRORBUDDYGROUP_H */
