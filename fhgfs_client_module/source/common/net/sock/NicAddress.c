#include <common/net/sock/NicAddress.h>
#include <common/toolkit/Serialization.h>

/**
 * @return fhgfs_true if lhs (left-hand side) is preferred compared to rhs
 */
fhgfs_bool NicAddress_preferenceComp(const NicAddress* lhs, const NicAddress* rhs)
{
   // compares the preference of NICs
   // returns fhgfs_true if lhs is preferred compared to rhs

   unsigned lhsHostOrderIP;
   unsigned rhsHostOrderIP;


   // prefer lower metric
   if(lhs->metric < rhs->metric)
      return fhgfs_true;
   if(lhs->metric > rhs->metric)
      return fhgfs_false;

   // prefer RDMA NICs
   if( (lhs->nicType == NICADDRTYPE_RDMA) && (rhs->nicType != NICADDRTYPE_RDMA) )
      return fhgfs_true;
   if( (rhs->nicType == NICADDRTYPE_RDMA) && (lhs->nicType != NICADDRTYPE_RDMA) )
      return fhgfs_false;

   // prefer SDP NICs
   if( (lhs->nicType == NICADDRTYPE_SDP) && (rhs->nicType == NICADDRTYPE_STANDARD) )
      return fhgfs_true;
   if( (rhs->nicType == NICADDRTYPE_SDP) && (lhs->nicType == NICADDRTYPE_STANDARD) )
      return fhgfs_false;

   // no bandwidth in client NicAddress
//   // prefer higher bandwidth
//   if(lhs->bandwidth > rhs->bandwidth)
//      return fhgfs_true;
//   if(lhs->bandwidth < rhs->bandwidth)
//      return fhgfs_false;

   // prefer higher ipAddr
   lhsHostOrderIP = Serialization_ntohlTrans(lhs->ipAddr);
   rhsHostOrderIP = Serialization_ntohlTrans(rhs->ipAddr);

   // this is the original IP-order version
   if(lhsHostOrderIP > rhsHostOrderIP)
      return fhgfs_true;
   if(lhsHostOrderIP < rhsHostOrderIP)
      return fhgfs_false;


   /*
   // IP-order alternative
   if(lhsHostOrderIP < rhsHostOrderIP)
      return fhgfs_true;
   if(lhsHostOrderIP > rhsHostOrderIP)
      return fhgfs_false;
   */

   // prefer lower hwAddr
   return(os_memcmp(lhs->hwAddr, rhs->hwAddr, BEEGFS_IFHWADDRLEN) < 0);
}


/**
 * Compares elements suitable for a tree.
 *
 * @param key1 NicAddress pointer
 * @param key2 NicAddress pointer
 * @return: <0 if key1<key2, 0 for equal keys, >0 otherwise
 */
int NicAddress_treeComparator(const void* key1, const void* key2)
{
   NicAddress* lhs = (NicAddress*)key1;
   NicAddress* rhs = (NicAddress*)key2;

   // Note: We interpret "preferred" elements as "smaller" elements here.

   if(NicAddress_preferenceComp(lhs, rhs) )
      return -1;

   if(NicAddress_preferenceComp(rhs, lhs) )
      return 1;

   return 0;
}
