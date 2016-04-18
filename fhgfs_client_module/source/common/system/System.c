#include <common/toolkit/StringTk.h>
#include <opentk/system/OpenTk_System.h>
#include "System.h"

char* System_getHostnameCopy(void)
{
   return fhgfs_System_getHostnameCopy();
}

// note: old version (before fhgfs_client_opentk_module)
//char* System_getHostnameCopy(void)
//{
//   char* hostnameOrig;
//   char* hostnameCopy;
//   
//   down_read(&uts_sem);
//
//#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,18)
//   hostnameOrig = system_utsname.nodename;
//#else
//   hostnameOrig = utsname()->nodename;
//#endif // LINUX_VERSION_CODE
//   
//   hostnameCopy = StringTk_strDup(hostnameOrig);
//
//   up_read(&uts_sem);
//   
//   return hostnameCopy;   
//}
