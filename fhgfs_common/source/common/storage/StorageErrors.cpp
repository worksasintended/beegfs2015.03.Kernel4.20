#include <common/app/log/LogContext.h>
#include "StorageErrors.h"

/**
 * Note: This is based on the FhgfsOpsErr entries
 * Note: We use EREMOTEIO as a generic error here
 */
struct FhgfsOpsErrListEntry const __FHGFSOPS_ERRLIST[] =
{
   {"Success", 0}, // FhgfsOpsErr_SUCCESS
   {"Internal error", EREMOTEIO}, // FhgfsOpsErr_INTERNAL
   {"Interrupted system call", EINTR}, // FhgfsOpsErr_INTERRUPTED
   {"Communication error", ECOMM}, // FhgfsOpsErr_COMMUNICATION
   {"Communication timeout", ETIMEDOUT}, // FhgfsOpsErr_COMMTIMEDOUT
   {"Unknown node", EREMOTEIO}, // FhgfsOpsErr_UNKNOWNNODE
   {"Node is not owner of entry", EREMOTEIO}, // FhgfsOpsErr_NOTOWNER
   {"Entry exists already", EEXIST}, // FhgfsOpsErr_EXISTS
   {"Path does not exist", ENOENT}, // FhgfsOpsErr_PATHNOTEXISTS
   {"Entry is in use", EBUSY}, // FhgfsOpsErr_INUSE
   {"Dynamic attributes of entry are outdated", EREMOTEIO}, // FhgfsOpsErr_INUSE
   {"Cannot move directory to one of its subdirs", EINVAL}, // FhgfsOpsErr_PARENTTOSUBDIR
   {"Entry is not a directory", ENOTDIR}, // FhgfsOpsErr_NOTADIR
   {"Directory is not empty", ENOTEMPTY}, // FhgfsOpsErr_NOTEMPTY
   {"No space left", ENOSPC}, // FhgfsOpsErr_NOSPACE
   {"Unknown storage target", EREMOTEIO}, // FhgfsOpsErr_UNKNOWNTARGET
   {"Operation would block", EWOULDBLOCK}, // FhgfsOpsErr_WOULDBLOCK
   {"Inode not inlined", EREMOTEIO}, // FhgfsOpsErr_INODENOTINLINED
   {"Underlying file system error", EREMOTEIO}, // FhgfsOpsErr_SAVEERROR
   {"Argument too large", EFBIG},  // FhgfsOpsErr_TOOBIG
   {"Invalid argument", EINVAL},   // FhgfsOpsErr_INVAL
   {"Bad memory address", EFAULT}, // FhgfsOpsErr_ADDRESSFAULT
   {"Try again", EAGAIN},          // FhgfsOpsErr_AGAIN
   {"Potential cache loss for open file handle. (Server crash detected.)" , EREMOTEIO}, /*
                                                                  FhgfsOpsErr_STORAGE_SRV_CRASHED*/
   {"Permission denied", EPERM},   // FhgfsOpsErr_PERM
   {"Quota exceeded", EDQUOT},     // FhgfsOpsErr_DQUOT
   {"Out of memory", ENOMEM},      // FhgfsOpsErr_OUTOFMEM
   {"Numerical result out of range", ERANGE}, // FhgfsOpsErr_RANGE
   {"No data available", ENODATA}, // FhgfsOpsErr_NODATA
   {"Operation not supported", EOPNOTSUPP}, // FhgfsOpsErr_NOTSUPP
   {NULL, 0}
};


/**
 * @return human-readable error string
 */
const char* FhgfsOpsErrTk::toErrString(FhgfsOpsErr errCode)
{
   size_t unsignedErrCode = (size_t)errCode;

   if(likely(unsignedErrCode < __FHGFSOPS_ERRLIST_SIZE) )
      return __FHGFSOPS_ERRLIST[errCode].errString;

   // error code unknown...

   LogContext log("FhgfsOpsErrTk::toErrString");

   log.log(Log_CRITICAL, "Unknown errCode given: " +
      StringTk::intToStr(errCode) + "/" + StringTk::uintToStr(errCode) + " "
      "(dumping stack...)");

   log.logBacktrace();

   return "Unknown error code";
}

/**
 * @return positive linux error code
 */
int FhgfsOpsErrTk::toSysErr(FhgfsOpsErr errCode)
{
   size_t unsignedErrCode = (size_t)errCode;

   if(likely(unsignedErrCode < __FHGFSOPS_ERRLIST_SIZE) )
      return __FHGFSOPS_ERRLIST[errCode].sysErr;

   // error code unknown...

   LogContext log("FhgfsOpsErrTk::toSysErr");

   log.log(Log_CRITICAL, "Unknown errCode given: " +
      StringTk::intToStr(errCode) + "/" + StringTk::uintToStr(errCode) + " "
      "(dumping stack...)");

   log.logBacktrace();

   return EPERM;
}
