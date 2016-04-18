#ifndef VERSIONTK_H_
#define VERSIONTK_H_

#include <common/toolkit/StringTk.h>
#include <common/Common.h>


/*
 * Note: Remember to always keep version encoding in sync with fhgfs_client's VersionTk.h
 */

/*
 * Size of BEEGFS_VERSION_CODE is 4 bytes (unsigned int). Format from highest to lowest byte:
 * 1 byte release_type number
 * 1 byte version_year-2000, e.g. 2012-2000=12 (-2000 to make it fit into a 1 byte value)
 * 1 byte version_month
 * 1 byte release_num
 */


/**
 * Short-hand for BEEGFS_VERSION_NUM_STRIP(BEEGFS_VERSION_CODE).
 */
#define BEEGFS_VERSION_NUM \
   BEEGFS_VERSION_NUM_STRIP(BEEGFS_VERSION_CODE)

/**
 * Strip build type from BEEGFS_VERSION_CODE, so that the result can be compared to other version
 * numbers, e.g. by using BEEGFS_VERSION_NUM_ENCODE().
 */
#define BEEGFS_VERSION_NUM_STRIP(fhgfsVersionCode) \
   (0xFFFFFF & fhgfsVersionCode) /* 1st byte is release type, so keep only last 3 bytes */

/**
 * Decode individual version elements from version code.
 */
#define BEEGFS_VERSION_NUM_DECODE(fhgfsVersionCode, outYear, outMonth, outReleaseNum) \
   do                                                                                \
   {                                                                                 \
      outYear  =    ( (fhgfsVersionCode >> 16) & 0xFF) + 2000;                       \
      outMonth =      (fhgfsVersionCode >>  8) & 0xFF;                               \
      outReleaseNum = (fhgfsVersionCode      ) & 0xFF;                               \
   } while(0)

/**
 * Encode fhgfs version code from year/month/releaseNum and leave release type number out.
 *
 * @return unsigned int
 */
#define BEEGFS_VERSION_NUM_ENCODE(year, month, releaseNum) \
   ( ( (year-2000) << 16) + (month << 8) + (releaseNum) )

/**
 * Short-hand for BEEGFS_VERSION_TYPE_STRIP(BEEGFS_VERSION_CODE).
 */
#define BEEGFS_VERSION_TYPE \
   BEEGFS_VERSION_TYPE_STRIP(BEEGFS_VERSION_CODE)

/**
 * Strip version numbers from BEEGFS_VERSION_CODE, so that the result can be compared to other build
 * types, e.g. by using BEEGFS_VERSION_TYPE_STRIP().
 */
#define BEEGFS_VERSION_TYPE_STRIP(fhgfsVersionCode) \
   (0xFF000000 & fhgfsVersionCode) /* only 1st byte is release type, so discard the rest) */



/**
 * Toolkit with helpers for version encoding, decoding and comparison.
 */
class VersionTk
{
   public:

   private:
      VersionTk() {}

   public:
      // inliners

      /**
       * We call this "pseudoVersionStr", because it's not not the real fhgfs version (e.g. no "-r"
       * and only one digit month), but this is simple and good enough for most use cases.
       */
      static std::string versionCodeToPseudoVersionStr(unsigned fhgfsVersionCode)
      {
         unsigned versionYear;
         unsigned versionMonth;
         unsigned versionReleaseNum;

         BEEGFS_VERSION_NUM_DECODE(fhgfsVersionCode, versionYear, versionMonth, versionReleaseNum);


         std::string fhgfsPseudoVersionStr =
            StringTk::uintToStr(versionYear) + "." +
            StringTk::uintToStr(versionMonth) + "-" +
            StringTk::uintToStr(versionReleaseNum);

         return fhgfsPseudoVersionStr;
      }

      /**
       * Compare version codes.
       * Intended to more conveniently check whether a certain feature is supported by a given node.
       *
       * @param requiredVersionCode minimum required version for a certain feature.
       * @param nodeVersionCode the version code to check (usually from the node that we want to
       * test for a certain feature).
       * @return true if the node version code is equal or greater than required version code.
       */
      static bool checkRequiredRelease(unsigned requiredVersionCode, unsigned nodeVersionCode)
      {
         unsigned requiredVersionCodeStrip = BEEGFS_VERSION_NUM_STRIP(requiredVersionCode);
         unsigned nodeVersionCodeStrip = BEEGFS_VERSION_NUM_STRIP(nodeVersionCode);

         return (nodeVersionCodeStrip >= requiredVersionCodeStrip);
      }
};


#endif /* VERSIONTK_H_ */
