#include <common/toolkit/MathTk.h>
#include "Raid10Pattern.h"

void Raid10Pattern_serializePattern(StripePattern* this, char* buf)
{
   Raid10Pattern* thisCast = (Raid10Pattern*)this;

   size_t bufPos = 0;

   // defaultNumTargets
   bufPos += Serialization_serializeUInt(&buf[bufPos], thisCast->defaultNumTargets);

   // stripeTargetIDs
   bufPos += Serialization_serializeUInt16Vec(&buf[bufPos], &thisCast->stripeTargetIDs);

   // mirrorTargetIDs
   bufPos += Serialization_serializeUInt16Vec(&buf[bufPos], &thisCast->mirrorTargetIDs);
}

fhgfs_bool Raid10Pattern_deserializePattern(StripePattern* this, const char* buf, size_t bufLen)
{
   Raid10Pattern* thisCast = (Raid10Pattern*)this;

   size_t bufPos = 0;

   unsigned defNumInfoFieldLen;
   unsigned targetIDsLen;
   unsigned targetIDsElemNum;
   const char* targetIDsListStart;
   unsigned mirrorTargetIDsLen;
   unsigned mirrorTargetIDsElemNum;
   const char* mirrorTargetIDsListStart;

   // defaultNumTargets
   if(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos,
      &thisCast->defaultNumTargets, &defNumInfoFieldLen) )
      return fhgfs_false;

   bufPos += defNumInfoFieldLen;

   // targetIDs
   if(!Serialization_deserializeUInt16VecPreprocess(&buf[bufPos], bufLen-bufPos,
      &targetIDsElemNum, &targetIDsListStart, &targetIDsLen) )
      return fhgfs_false;

   bufPos += targetIDsLen;

   if(!Serialization_deserializeUInt16Vec(targetIDsLen, targetIDsElemNum,
      targetIDsListStart, &thisCast->stripeTargetIDs) )
      return fhgfs_false;

   // mirrorTargetIDs
   if(!Serialization_deserializeUInt16VecPreprocess(&buf[bufPos], bufLen-bufPos,
      &mirrorTargetIDsElemNum, &mirrorTargetIDsListStart, &mirrorTargetIDsLen) )
      return fhgfs_false;

   bufPos += mirrorTargetIDsLen;

   if(!Serialization_deserializeUInt16Vec(mirrorTargetIDsLen, mirrorTargetIDsElemNum,
      mirrorTargetIDsListStart, &thisCast->mirrorTargetIDs) )
      return fhgfs_false;


   // calc stripeSetSize
   thisCast->stripeSetSize = UInt16Vec_length(
      &thisCast->stripeTargetIDs) * StripePattern_getChunkSize(this);


   return fhgfs_true;
}

unsigned Raid10Pattern_serialLen(StripePattern* this)
{
   Raid10Pattern* thisCast = (Raid10Pattern*)this;

   return STRIPEPATTERN_HEADER_LENGTH +
      Serialization_serialLenUInt() + // defaultNumTargets
      Serialization_serialLenUInt16Vec(&thisCast->stripeTargetIDs) +
      Serialization_serialLenUInt16Vec(&thisCast->mirrorTargetIDs);
}

size_t Raid10Pattern_getStripeTargetIndex(StripePattern* this, int64_t pos)
{
   Raid10Pattern* thisCast = (Raid10Pattern*)this;

   /* the code below is an optimization (wrt division/modulo) of following the two lines:
         int64_t stripeSetInnerOffset = pos % thisCast->stripeSetSize;
         int64_t targetIndex = stripeSetInnerOffset / StripePattern_getChunkSize(this); */

   // note: do_div(n64, base32) assigns the result to n64 and returns the remainder!
   // (do_div is needed for 64bit division on 32bit archs)

   unsigned stripeSetSize = thisCast->stripeSetSize;

   int64_t stripeSetInnerOffset;
   unsigned chunkSize;
   size_t targetIndex;

   if(MathTk_isPowerOfTwo(stripeSetSize) )
   { // quick path => no modulo needed
      stripeSetInnerOffset = pos & (stripeSetSize - 1);
   }
   else
   { // slow path => modulo
      stripeSetInnerOffset = do_div(pos, thisCast->stripeSetSize);

      // warning: do_div modifies pos! (so do not use it afterwards within this method)
   }

   chunkSize = StripePattern_getChunkSize(this);

   // this is "a=b/c" written as "a=b>>log2(c)", because chunkSize is a power of two.
   targetIndex = (stripeSetInnerOffset >> MathTk_log2Int32(chunkSize) );

   return targetIndex;
}

uint16_t Raid10Pattern_getStripeTargetID(StripePattern* this, int64_t pos)
{
   Raid10Pattern* thisCast = (Raid10Pattern*)this;

   size_t targetIndex = Raid10Pattern_getStripeTargetIndex(this, pos);

   return UInt16Vec_at(&thisCast->stripeTargetIDs, targetIndex);
}

void Raid10Pattern_getStripeTargetIDsCopy(StripePattern* this, UInt16Vec* outTargetIDs)
{
   Raid10Pattern* thisCast = (Raid10Pattern*)this;

   ListTk_copyUInt16ListToVec( (UInt16List*)&thisCast->stripeTargetIDs, outTargetIDs);
}

UInt16Vec* Raid10Pattern_getStripeTargetIDs(StripePattern* this)
{
   Raid10Pattern* thisCast = (Raid10Pattern*)this;

   return &thisCast->stripeTargetIDs;
}

UInt16Vec* Raid10Pattern_getMirrorTargetIDs(StripePattern* this)
{
   Raid10Pattern* thisCast = (Raid10Pattern*)this;

   return &thisCast->mirrorTargetIDs;
}

unsigned Raid10Pattern_getMinNumTargets(StripePattern* this)
{
   return 2;
}

unsigned Raid10Pattern_getDefaultNumTargets(StripePattern* this)
{
   Raid10Pattern* thisCast = (Raid10Pattern*)this;

   return thisCast->defaultNumTargets;
}

