#include "NetworkInterfaceCard.h"

#include <linux/if.h>
#include <linux/netdevice.h>
#include <linux/inetdevice.h>


#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,21)
#define fhgfs_first_net_device()             dev_base
#define fhgfs_next_net_device(currentDev)    currentDev->next
#elif LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,23)
#define fhgfs_first_net_device()             first_net_device()
#define fhgfs_next_net_device(currentDev)    next_net_device(currentDev)
#else
#define fhgfs_first_net_device()             first_net_device(&init_net)
#define fhgfs_next_net_device(currentDev)    next_net_device(currentDev)
#endif // LINUX_VERSION_CODE


struct net_device* fhgfs_NIC_getFirstNetDevice(void)
{
   return fhgfs_first_net_device();
}



struct net_device* fhgfs_NIC_getNextNetDevice(struct net_device* currentDev)
{
   return fhgfs_next_net_device(currentDev);
}

