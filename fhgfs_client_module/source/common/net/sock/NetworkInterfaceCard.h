#ifndef NETWORKINTERFACECARD_H_
#define NETWORKINTERFACECARD_H_

#include <common/Common.h>
#include <common/toolkit/StringTk.h>
#include <common/net/sock/NicAddress.h>
#include <common/net/sock/NicAddressList.h>
#include <common/net/sock/NicAddressListIter.h>


extern void NIC_findAll(StrCpyList* allowedInterfaces, fhgfs_bool useSDP, fhgfs_bool useRDMA,
   NicAddressList* outList);
extern fhgfs_bool NIC_findAllV2(StrCpyList* allowedInterfaces, fhgfs_bool useSDP,
   fhgfs_bool useRDMA, NicAddressList* outList);

extern const char* NIC_nicTypeToString(NicAddrType_t nicType);
extern char* NIC_nicAddrToString(NicAddress* nicAddr);
extern char* NIC_nicAddrToStringLight(NicAddress* nicAddr);
extern char* NIC_hwAddrToString(char* hwAddr);

extern fhgfs_bool NIC_supportsSDP(NicAddressList* nicList);
extern fhgfs_bool NIC_supportsRDMA(NicAddressList* nicList);
extern void NIC_supportedCapabilities(NicAddressList* nicList,
   NicListCapabilities* outCapabilities);

extern void __NIC_findAllTCP(StrCpyList* allowedInterfaces, NicAddressList* outList);
extern fhgfs_bool __NIC_checkSDPAvailable(void);
extern void __NIC_filterInterfacesForRDMA(NicAddressList* list, NicAddressList* outList);


#endif /*NETWORKINTERFACECARD_H_*/
