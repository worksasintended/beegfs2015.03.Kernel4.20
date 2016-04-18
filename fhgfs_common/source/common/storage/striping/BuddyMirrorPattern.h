#ifndef BUDDYMIRRORPATTERN_H_
#define BUDDYMIRRORPATTERN_H_

#include <common/toolkit/serialization/Serialization.h>
#include <common/toolkit/MathTk.h>
#include "StripePattern.h"


#define BUDDYMIRRORPATTERN_DEFAULT_NUM_TARGETS  4


class BuddyMirrorPattern : public StripePattern
{
   friend class StripePattern;
   
   public:
      /**
       * @param chunkSize 0 for app-level default
       * @param defaultNumTargets default number of mirror buddy groups (0 for app-level default)
       */
      BuddyMirrorPattern(unsigned chunkSize, const UInt16Vector& mirrorBuddyGroupIDs,
         unsigned defaultNumTargets=0) :
         StripePattern(STRIPEPATTERN_BuddyMirror, chunkSize),
         mirrorBuddyGroupIDs(mirrorBuddyGroupIDs)
      {
         // this->stripeTargetIDs = stripeTargetIDs; // (done in initializer list now)

         this->defaultNumTargets =
            defaultNumTargets ? defaultNumTargets : BUDDYMIRRORPATTERN_DEFAULT_NUM_TARGETS;

         this->stripeSetSize = mirrorBuddyGroupIDs.size() * getChunkSize(); /* note: chunkSize might
            change in super class contructor, so we need to re-get the value here */
      }

      
   protected:
      /**
       * Note: for deserialization only
       */
      BuddyMirrorPattern(unsigned chunkSize) : StripePattern(STRIPEPATTERN_BuddyMirror, chunkSize)
      {
         // nothing to be done here
      }
      
      // (de)serialization

      virtual void serializePattern(char* buf);
      virtual bool deserializePattern(const char* buf, size_t bufLen);

      virtual unsigned serialLen()
      {
         return STRIPEPATTERN_HEADER_LENGTH +
            Serialization::serialLenUInt()                            + // defaultNumTargets
            Serialization::serialLenUInt16Vector(&mirrorBuddyGroupIDs); // mirrorBuddyGroupIDs
      };
      

   private:
      UInt16Vector mirrorBuddyGroupIDs;
      unsigned stripeSetSize; // = numStripeTargets * chunkSize
      unsigned defaultNumTargets;
      

   public:
      // getters & setters

      virtual size_t getStripeTargetIndex(int64_t pos) const
      {
         /* the code below is an optimization (wrt division) of the following two lines:
               int64_t stripeSetInnerOffset = pos % stripeSetSize;
               int64_t targetIndex = stripeSetInnerOffset / StripePattern_getChunkSize(this); */


         int64_t stripeSetInnerOffset;

         if(MathTk::isPowerOfTwo(stripeSetSize) )
         { // quick path => no modulo needed
            stripeSetInnerOffset = pos & (stripeSetSize - 1);
         }
         else
         { // slow path => modulo
            stripeSetInnerOffset = pos % stripeSetSize;
         }

         unsigned chunkSize = getChunkSize();
         
         // this is "a=b/c" written as "a=b>>log2(c)", because chunkSize is a power of two.
         size_t targetIndex = (stripeSetInnerOffset >> MathTk::log2Int32(chunkSize) );

         return targetIndex;
      }

      virtual uint16_t getStripeTargetID(int64_t pos) const
      {
         size_t targetIndex = getStripeTargetIndex(pos);

         return mirrorBuddyGroupIDs[targetIndex];
      }

      
      virtual void getStripeTargetIDs(UInt16Vector* outTargetIDs) const
      {
         *outTargetIDs = mirrorBuddyGroupIDs;
      }

      virtual const UInt16Vector* getStripeTargetIDs() const
      {
         return &mirrorBuddyGroupIDs;
      }

      virtual UInt16Vector* getStripeTargetIDsModifyable()
      {
         return &mirrorBuddyGroupIDs;
      }
      
      virtual bool updateStripeTargetIDs(StripePattern* updatedStripePattern);

      virtual unsigned getMinNumTargets() const
      {
         return getMinNumTargetsStatic();
      }

      virtual unsigned getDefaultNumTargets() const
      {
         return defaultNumTargets;
      }

      virtual void setDefaultNumTargets(unsigned defaultNumTargets)
      {
         this->defaultNumTargets = defaultNumTargets;
      }

      /**
       * Number of targets actually assigned
       */
      virtual size_t getAssignedNumTargets() const
      {
         return mirrorBuddyGroupIDs.size();
      }


      // inliners

      virtual StripePattern* clone() const
      {
         return clone(mirrorBuddyGroupIDs);
      }

      StripePattern* clone(const UInt16Vector& mirrorBuddyGroupIDs) const
      {
         BuddyMirrorPattern* pattern = new BuddyMirrorPattern(getChunkSize(), mirrorBuddyGroupIDs,
            defaultNumTargets);

         return pattern;
      }


      // static inliners

      static unsigned getMinNumTargetsStatic()
      {
         return 1;
      }

};

#endif /*BUDDYMIRRORPATTERN_H_*/
