#ifndef OPENTK_NETWORKINTERFACECARD_H_
#define OPENTK_NETWORKINTERFACECARD_H_

#include <opentk/OpenTk_Common.h>


struct net_device; // forward declaration


extern struct net_device* fhgfs_NIC_getFirstNetDevice(void);
extern struct net_device* fhgfs_NIC_getNextNetDevice(struct net_device* currentDev);


#endif /*OPENTK_NETWORKINTERFACECARD_H_*/
