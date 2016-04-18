#ifndef NICADDRESS_H_
#define NICADDRESS_H_

#include <common/Common.h>
#include <common/toolkit/SocketTk.h>


#define BEEGFS_IFNAMSIZ           16
#define BEEGFS_IFHWADDRLEN        6

#define NICADDRESS_IP_STR_LEN    16


enum NicAddrType;
typedef enum NicAddrType NicAddrType_t;

struct NicAddress;
typedef struct NicAddress NicAddress;

struct NicListCapabilities;
typedef struct NicListCapabilities NicListCapabilities;


extern fhgfs_bool NicAddress_preferenceComp(const NicAddress* lhs, const NicAddress* rhs);
extern int NicAddress_treeComparator(const void* key1, const void* key2);

// inliners
static inline void NicAddress_ipToStr(fhgfs_in_addr ipAddr, char* outStr);


enum NicAddrType        {NICADDRTYPE_STANDARD=0, NICADDRTYPE_SDP=1, NICADDRTYPE_RDMA=2};

struct NicAddress
{
   fhgfs_in_addr  ipAddr;
   fhgfs_in_addr  broadcastAddr;
   int            metric;
   NicAddrType_t  nicType;
   char           name[BEEGFS_IFNAMSIZ];
   char           hwAddr[BEEGFS_IFHWADDRLEN];
};

struct NicListCapabilities
{
   fhgfs_bool supportsSDP;
   fhgfs_bool supportsRDMA;
};




/**
 * @param outStr must be at least NICADDRESS_STR_LEN bytes long
 */
void NicAddress_ipToStr(fhgfs_in_addr ipAddr, char* outStr)
{
   unsigned char* ipArray = (unsigned char*)&ipAddr;

   os_sprintf(outStr, "%u.%u.%u.%u", ipArray[0], ipArray[1], ipArray[2], ipArray[3]);
}

#endif /*NICADDRESS_H_*/
