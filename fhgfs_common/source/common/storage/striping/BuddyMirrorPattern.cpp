#include "BuddyMirrorPattern.h"


void BuddyMirrorPattern::serializePattern(char* buf)
{
   size_t bufPos = 0;
   
   // defaultNumTargets
   bufPos += Serialization::serializeUInt(&buf[bufPos], defaultNumTargets);

   // mirrorBuddyGroupIDs
   bufPos += Serialization::serializeUInt16Vector(&buf[bufPos], &mirrorBuddyGroupIDs);
}


bool BuddyMirrorPattern::deserializePattern(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   // defaultNumTargets
   unsigned defNumInfoFieldLen;
   if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
      &defaultNumTargets, &defNumInfoFieldLen) )
      return false;

   bufPos += defNumInfoFieldLen;

   // mirrorBuddyGroupIDs
   unsigned mirrorBuddyGroupIDsLen;
   unsigned mirrorBuddyGroupIDsElemNum;
   const char* mirrorBuddyGroupIDsListStart;

   if ( !Serialization::deserializeUInt16VectorPreprocess(&buf[bufPos], bufLen - bufPos,
      &mirrorBuddyGroupIDsElemNum, &mirrorBuddyGroupIDsListStart, &mirrorBuddyGroupIDsLen) )
      return false;

   bufPos += mirrorBuddyGroupIDsLen;
   
   if ( !Serialization::deserializeUInt16Vector(mirrorBuddyGroupIDsLen, mirrorBuddyGroupIDsElemNum,
      mirrorBuddyGroupIDsListStart, &mirrorBuddyGroupIDs) )
      return false;
   
   // calc stripeSetSize
   stripeSetSize = mirrorBuddyGroupIDs.size() * getChunkSize();
   
   // note: empty mirrorBuddyGroupIDs is not an error, because directories never have any.
   
   return true;
}

bool BuddyMirrorPattern::updateStripeTargetIDs(StripePattern* updatedStripePattern)
{
   if ( !updatedStripePattern )
      return false;

   if ( this->mirrorBuddyGroupIDs.size() != updatedStripePattern->getStripeTargetIDs()->size() )
      return false;

   for ( unsigned i = 0; i < mirrorBuddyGroupIDs.size(); i++ )
   {
      mirrorBuddyGroupIDs[i] = updatedStripePattern->getStripeTargetIDs()->at(i);
   }

   return true;
}

bool BuddyMirrorPattern::patternEquals(const StripePattern* second, bool checkHeader) const
{
   if(checkHeader && (!headerEquals(second) ) )
      return false;

   BuddyMirrorPattern* pattern = (BuddyMirrorPattern*) second;

   // defaultNumTargets
   if(this->defaultNumTargets != pattern->getDefaultNumTargets() )
      return false;

   // stripeTargetIDs
   if(this->getNumStripeTargetIDs() != pattern->getNumStripeTargetIDs() )
      return false;

   if(!std::equal(this->mirrorBuddyGroupIDs.begin(), this->mirrorBuddyGroupIDs.end(),
      pattern->getStripeTargetIDs()->begin() ) )
      return false;

   return true;
}
