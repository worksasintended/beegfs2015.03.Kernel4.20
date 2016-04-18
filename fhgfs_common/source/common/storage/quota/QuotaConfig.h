#ifndef QUOTACONFIG_H_
#define QUOTACONFIG_H_


#include <common/storage/quota/QuotaData.h>


/**
 * A parent struct for all struct with quota configurations
 */
struct QuotaConfig
{
   QuotaDataType cfgType;        // the type of the IDs: QuotaDataType_...
   unsigned cfgID;               // a single UID/GID
   bool cfgUseAll;               // true if all available UIDs/GIDs needed
   bool cfgUseList;              // true if a UID/GID list is given
   bool cfgUseRange;             // true if a UID/GID range is given
   unsigned cfgIDRangeStart;     // the first UIDs/GIDs of a range to collect the quota data/limits
   unsigned cfgIDRangeEnd;       // the last UIDs/GIDs of a range to collect the quota data/limits
   UIntList cfgIDList;           // the list of UIDs/GIDs to collect the quota data/limits
};


#endif /* QUOTACONFIG_H_ */
