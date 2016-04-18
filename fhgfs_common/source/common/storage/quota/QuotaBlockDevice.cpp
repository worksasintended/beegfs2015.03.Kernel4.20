#include <common/app/log/LogContext.h>

#include <sys/stat.h>
#include <mntent.h>
#include <fstab.h>

#include "QuotaBlockDevice.h"

/**
 * checks one storage target path and creates a QuotaBlockDevice
 */
QuotaBlockDevice QuotaBlockDevice::getBlockDeviceOfTarget(std::string& targetPath)
{
   static std::string mountInformationPath("/proc/mounts");

   std::string resMountPath;
   std::string resBlockDevicePath;
   QuotaBlockDeviceFsType resFsType = QuotaBlockDeviceFsType_UNKNOWN;

   struct mntent* mntData;
   FILE *mntFile = setmntent(mountInformationPath.c_str(), "r");

   while ( (mntData = getmntent(mntFile) ) )
   {
      std::string tmpMountPath(mntData->mnt_dir);

      // test all mounts, use the mount with the longest match and use the last match if multiple
      // mounts with the longest match exists
      if ( (targetPath.find(tmpMountPath) == 0) && (resMountPath.size() <= tmpMountPath.size() ) )
      {
         resMountPath = mntData->mnt_dir;
         resBlockDevicePath = mntData->mnt_fsname;
         std::string type(mntData->mnt_type);

         if ( type.compare("xfs") == 0 )
            resFsType = QuotaBlockDeviceFsType_XFS;
         else
         if ( type.find("ext") == 0 )
            resFsType = QuotaBlockDeviceFsType_EXTX;
         else
            resFsType = QuotaBlockDeviceFsType_UNKNOWN;
      }
   }

   endmntent(mntFile);

   QuotaBlockDevice blockDevice(resMountPath, resBlockDevicePath, resFsType, targetPath);
   return blockDevice;
}

/**
 * checks all storage target paths and creates a QuotaBlockDevice
 *
 * @param targetPaths list with the paths of the storage target to check for enabled quota
 * @param outBlockDevices the list with all QuotaBlockDevice
 */
void QuotaBlockDevice::getBlockDevicesOfTargets(TargetPathMap* targetPaths,
   QuotaBlockDeviceMap* outBlockDevices)
{
   for(TargetPathMapIter iter = targetPaths->begin(); iter != targetPaths->end(); iter++)
   {
      uint16_t targetNumID =  iter->first;
      std::string storageTargetPath = iter->second;

      QuotaBlockDevice blockDevice = getBlockDeviceOfTarget(storageTargetPath);
      outBlockDevices->insert(QuotaBlockDeviceMapVal(targetNumID, blockDevice) );
   }
}
