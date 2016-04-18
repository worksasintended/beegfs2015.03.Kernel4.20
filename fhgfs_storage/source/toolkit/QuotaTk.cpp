#include <common/toolkit/UnitTk.h>
#include "QuotaTk.h"

#include <xfs/xfs.h>
#include <xfs/xqm.h>
#include <sys/mount.h>

/**
 * get quota data for a single ID
 *
 * @param id the user or group ID to check
 * @param type the type of the ID user or group, enum QuotaDataType
 * @param blockDevices the QuotaBlockDevice to check
 * @param outQuotaDataList the return list with the quota data
 *
 * @return false on error (in which case outData is empty)
 */
bool QuotaTk::requestQuotaForID(unsigned id, QuotaDataType type,
   QuotaBlockDeviceMap* blockDevices, QuotaDataList* outQuotaDataList)
{
   QuotaData data(id, type);
   bool retVal = checkQuota(blockDevices, &data);

   if(retVal && (data.getSize() != 0 || data.getInodes() != 0) )
      outQuotaDataList->push_back(data);

   return retVal;
}

/**
 * get quota for the a range of IDs
 *
 * @param blockDevices the QuotaBlockDevice to check
 * @param rangeStart the first ID of the ID range
 * @param rangeEnd the last ID of the ID range
 * @param type the quota data ID type user/group, QuotaDataType_...
 * @param outQuotaDataList the return list with the quota data
 *
 * @return false on error (in which case outData is empty)
 */
bool QuotaTk::requestQuotaForRange(QuotaBlockDeviceMap* blockDevices, unsigned rangeStart,
   unsigned rangeEnd, QuotaDataType type, QuotaDataList* outQuotaDataList)
{
   bool retVal = true;

   for(unsigned id = rangeStart; id <= rangeEnd; id++)
   {
      bool errorVal = requestQuotaForID(id, type, blockDevices, outQuotaDataList);

      if(!errorVal)
         retVal = false;
   }

   return retVal;
}

/**
 * get quota for the a range of IDs
 *
 * @param blockDevices the QuotaBlockDevice to check
 * @param idList the list with the IDs to check
 * @param type the quota data ID type user/group, QuotaDataType_...
 * @param outQuotaDataList the return list with the quota data
 *
 * @return false on error (in which case outData is empty)
 */
bool QuotaTk::requestQuotaForList(QuotaBlockDeviceMap* blockDevices, UIntList* idList,
   QuotaDataType type, QuotaDataList* outQuotaDataList)
{
   bool retVal = true;

   for(UIntListIter iter = idList->begin(); iter != idList->end(); iter++)
   {
      bool errorVal = requestQuotaForID(*iter, type, blockDevices, outQuotaDataList);

      if(!errorVal)
         retVal = false;
   }

   return retVal;
}

/**
 * get QuotaData for the given QuotaData which is initialized with type and ID
 *
 * @param blockDevices the QuotaBlockDevice to check
 * @param outData needs to be initialized with type and ID
 *
 * @return false on error (in which case outData is not initialized)
 */
bool QuotaTk::checkQuota(QuotaBlockDeviceMap* blockDevices, QuotaData* outData)
{
   const char* logContext = "GetQuotaInfo (request quota)";

   bool retVal = true;
   int errorCode = 0;

   dqblk quotaData;
   fs_disk_quota xfsQuotaData;

   QuotaDataType quotaType = outData->getType();

   for(QuotaBlockDeviceMapIter iter = blockDevices->begin(); iter != blockDevices->end(); iter++)
   {
      QuotaBlockDeviceFsType fstype = iter->second.getFsType();
      if(fstype == QuotaBlockDeviceFsType_XFS)
      {
         if(quotaType == QuotaDataType_USER)
            errorCode = quotactl(QCMD(Q_XGETQUOTA, USRQUOTA),
               iter->second.getBlockDevicePath().c_str(),
               outData->getID(), (caddr_t)&xfsQuotaData);
         else
         if (quotaType == QuotaDataType_GROUP)
            errorCode = quotactl(QCMD(Q_XGETQUOTA, GRPQUOTA),
               iter->second.getBlockDevicePath().c_str(),
               outData->getID(), (caddr_t)&xfsQuotaData);
         else
         {
            LogContext(logContext).logErr("Error: Quota request - useless quota type. Type: " +
               QuotaData::QuotaDataTypeToString(outData->getType() ) + "; ID: " +
               StringTk::uintToStr(outData->getID() ) );

            retVal = false;
            goto error_finish;
         }
      }
      else
      {
         if(quotaType == QuotaDataType_USER)
            errorCode = quotactl(QCMD(Q_GETQUOTA, USRQUOTA),
               iter->second.getBlockDevicePath().c_str(),
               outData->getID(), (caddr_t)&quotaData);
         else
         if (quotaType == QuotaDataType_GROUP)
            errorCode = quotactl(QCMD(Q_GETQUOTA, GRPQUOTA),
               iter->second.getBlockDevicePath().c_str(),
               outData->getID(), (caddr_t)&quotaData);
         else
         {
            LogContext(logContext).logErr("Error: Quota request - useless quota type. Type: " +
               QuotaData::QuotaDataTypeToString(outData->getType() ) + "; ID: " +
               StringTk::uintToStr(outData->getID() ) );

            retVal = false;
            goto error_finish;
         }
      }

      if (errorCode != 0)
      {
         errorCode = errno;

         // ignore error if no quota for user found (ESRCH) , especially for XFS (ENOENT)
         if( (errorCode != ESRCH) &&
            !((iter->second.getFsType() == QuotaBlockDeviceFsType_XFS) && (errorCode == ENOENT) ) )
         {
            LogContext(logContext).logErr("Error: Quota request - quotactl failed. Type: " +
               QuotaData::QuotaDataTypeToString(outData->getType() ) + "; ID: " +
               StringTk::uintToStr(outData->getID()) + "; SysErr: " +
               System::getErrString(errorCode) );

            retVal = false;
            goto error_finish;
         }

         continue;
      }

      if(fstype == QuotaBlockDeviceFsType_XFS)
      {
         uint64_t blocks = UnitTk::quotaBlockCountToByte(xfsQuotaData.d_bcount,
            iter->second.getFsType());
         outData->forceMergeQuotaDataCounter(blocks, xfsQuotaData.d_icount);
      }
      else
      {
         if((quotaData.dqb_valid & QIF_USAGE) != 0)
         {
            uint64_t blocks = UnitTk::quotaBlockCountToByte(quotaData.dqb_curspace,
               iter->second.getFsType());

            outData->forceMergeQuotaDataCounter(blocks, quotaData.dqb_curinodes);
         }
         else
         {
            // set return value to false, but do not abort the quota request from the other targets,
            // maybe the other targets returns valid values
            LogContext(logContext).logErr("Error: Quota request - values not valid. Type: " +
               QuotaData::QuotaDataTypeToString(outData->getType() ) + "; ID: " +
               StringTk::uintToStr(outData->getID() ) );

            retVal = false;
         }
      }
   }

error_finish:
   return retVal;
}
