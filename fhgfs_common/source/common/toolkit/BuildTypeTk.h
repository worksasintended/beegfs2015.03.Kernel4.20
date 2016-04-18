#ifndef BUILDTYPETK_H_
#define BUILDTYPETK_H_

#include <common/Common.h>
#include <common/app/config/InvalidConfigException.h>


/**
 * This works by setting the build type of the common lib via getCommonLibDebugBuildType() and setting
 * the build type of the program using the common lib vial getCurrentDebugBuildType().
 *
 * Note: Of course, there are a number of cases that won't be detected with this (e.g. rebuilds
 * of a program without a "make clean" etc.) and there is not guarantee that the check code will
 * even be reached when the build types differ.
 */

enum FhgfsBuildTypeDebug
   {FhgfsBuildType_DEBUG_OFF=0, FhgfsBuildType_DEBUG_ON=1};

enum FhgfsBuildTypeHSM
   {FhgfsBuildType_HSM_OFF=0, FhgfsBuildType_HSM_ON=1};

#ifdef BEEGFS_DEBUG
#define BUILDTYPE_CURRENT_DEBUG FhgfsBuildType_DEBUG_ON
#else
#define BUILDTYPE_CURRENT_DEBUG FhgfsBuildType_DEBUG_OFF
#endif


#ifdef BEEGFS_HSM
#define BUILDTYPE_CURRENT_HSM FhgfsBuildType_HSM_ON
#else
#define BUILDTYPE_CURRENT_HSM FhgfsBuildType_HSM_OFF
#endif

class BuildTypeTk
{
   public:
      static FhgfsBuildTypeDebug getCommonLibDebugBuildType();
      static FhgfsBuildTypeHSM getCommonLibHSMBuildType();

   private:
      BuildTypeTk() {}

   public:
      // inliners

      static inline FhgfsBuildTypeDebug getCurrentDebugBuildType()
      {
         return BUILDTYPE_CURRENT_DEBUG;
      }

      static inline FhgfsBuildTypeHSM getCurrentHSMBuildType()
      {
         return BUILDTYPE_CURRENT_HSM;
      }

      static inline void checkDebugBuildTypes() throw(InvalidConfigException)
      {
         if(getCurrentDebugBuildType() != getCommonLibDebugBuildType() )
            throw InvalidConfigException("Debug build types differ. Check your release/debug make "
               "settings.");
      }

      static inline void checkHSMBuildTypes() throw(InvalidConfigException)
      {
         if(getCurrentHSMBuildType() != getCommonLibHSMBuildType() )
            throw InvalidConfigException("HSM build types differ. Check your HSM make settings.");
      }

};


#endif /* BUILDTYPETK_H_ */
