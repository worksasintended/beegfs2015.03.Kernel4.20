#ifndef DEMOTIONFLAGS_H_
#define DEMOTIONFLAGS_H_

/**
 * Stores the four flags to determine whether demotion (dynamic limit) is activated for a
 * particular pool and why.
 */
class DemotionFlags
{
   public:
      /**
       * Constructor.
       * Determines the flags from the TargetCapacityPools' limits and the four Min/Max values
       */
       DemotionFlags(const DynamicPoolLimits& poolLimitsSpace,
            const DynamicPoolLimits& poolLimitsInodes,
            const MinMaxStore<int64_t>& normalPoolFreeSpace,
            const MinMaxStore<int64_t>& lowPoolFreeSpace,
            const MinMaxStore<int64_t>& normalPoolInodes,
            const MinMaxStore<int64_t>& lowPoolInodes) :
            normalPoolSpace(
                  poolLimitsSpace.demotionActiveNormalPool(normalPoolFreeSpace) ),
            lowPoolSpace(
                  poolLimitsSpace.demotionActiveLowPool(lowPoolFreeSpace) ),
            normalPoolInodes(
                  poolLimitsInodes.demotionActiveNormalPool(normalPoolInodes) ),
            lowPoolInodes(
                  poolLimitsInodes.demotionActiveLowPool(lowPoolInodes) )
       { }


   private:
      const bool normalPoolSpace;
      const bool lowPoolSpace;
      const bool normalPoolInodes;
      const bool lowPoolInodes;


   public:
      // getters

      bool getNormalPoolSpaceFlag() const
      {
         return normalPoolSpace;
      }

      bool getLowPoolSpaceFlag() const
      {
         return lowPoolSpace;
      }

      bool getNormalPoolInodesFlag() const
      {
         return normalPoolInodes;
      }

      bool getLowPoolInodesFlag() const
      {
         return lowPoolInodes;
      }

      // inliners

      /**
       * @returns whether any of the four demotion flags is active
       */
      bool anyFlagActive() const
      {
         return normalPoolSpace || lowPoolSpace || normalPoolInodes || lowPoolInodes;
      }

};


#endif /* DEMOTIONFLAGS_H_ */
