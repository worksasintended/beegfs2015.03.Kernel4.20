#include <program/Program.h>

#include "ListChunkDirIncrementalMsgEx.h"

bool ListChunkDirIncrementalMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("ListChunkDirIncrementalMsg incoming");

   #ifdef BEEGFS_DEBUG
      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG_CONTEXT(log, Log_DEBUG,
         std::string("Received a ListChunkDirIncrementalMsg from: ") + peer);
   #endif // BEEGFS_DEBUG

   uint16_t targetID = getTargetID();
   bool isMirror = getIsMirror();
   std::string relativeDir = getRelativeDir();
   int64_t offset = getOffset();
   unsigned maxOutEntries = getMaxOutEntries();
   bool onlyFiles = getOnlyFiles();

   FhgfsOpsErr result;
   StringList names;
   IntList entryTypes;
   int64_t newOffset;

   result = readChunks(targetID, isMirror, relativeDir, offset, maxOutEntries, onlyFiles, names,
      entryTypes, newOffset);

   // send response...
   ListChunkDirIncrementalRespMsg respMsg(result, &names, &entryTypes, newOffset);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return true;
}

/*
 * CAUTION: No locking here!
 */
FhgfsOpsErr ListChunkDirIncrementalMsgEx::readChunks(uint16_t targetID, bool isMirror,
   std::string& relativeDir, int64_t offset, unsigned maxOutEntries, bool onlyFiles,
   StringList& outNames, IntList& outEntryTypes, int64_t &outNewOffset)
{
   App* app = Program::getApp();

   uint64_t numEntries = 0;
   struct dirent* dirEntry = NULL;

   int dirFD;

   if (likely(!relativeDir.empty()))
      dirFD = openat(app->getTargetFD(targetID, isMirror), relativeDir.c_str(), O_RDONLY);
   else
   {
      dirFD = dup(app->getTargetFD(targetID, isMirror));
      fcntl(dirFD, F_SETFL, O_RDONLY);
   }

   if(dirFD == -1)
   {
      int errCode = errno;

      if((errCode != ENOENT)
         || (!isMsgHeaderFeatureFlagSet(LISTCHUNKDIRINCREMENTALMSG_FLAG_IGNORENOTEXISTS)))
      {
         LogContext(__func__).logErr(
            "Unable to open chunks directory; targetID: " + StringTk::uintToStr(targetID)
               + "; isMirror: " + StringTk::intToStr((int) isMirror) + "; relativeDir: "
               + relativeDir + ". SysErr: " + System::getErrString(errCode));
      }
      else
      if (errCode == ENOENT)
         return FhgfsOpsErr_PATHNOTEXISTS;

      return FhgfsOpsErr_INTERNAL;
   }

   DIR* dirHandle = fdopendir(dirFD);
   if(!dirHandle)
   {
      int errCode = errno;

      close(dirFD);

      if((errCode != ENOENT)
         || (!isMsgHeaderFeatureFlagSet(LISTCHUNKDIRINCREMENTALMSG_FLAG_IGNORENOTEXISTS)))
      {
         LogContext(__func__).logErr(
            "Unable to create dir handle; targetID: " + StringTk::uintToStr(targetID)
               + "; isMirror: " + StringTk::intToStr((int) isMirror) + "; relativeDir: "
               + relativeDir + ". SysErr: " + System::getErrString(errCode));
      }
      else
      if (errCode == ENOENT)
         return FhgfsOpsErr_PATHNOTEXISTS;

      return FhgfsOpsErr_INTERNAL;
   }

   errno = 0; // recommended by posix (readdir(3p) )

   // seek to offset
   seekdir(dirHandle, offset); // (seekdir has no return value)

   // the actual entry reading
   for(; (numEntries < maxOutEntries) && (dirEntry = StorageTk::readdirFiltered(dirHandle));
      numEntries++)
   {
      // get the entry type
      DirEntryType entryType;

      if(dirEntry->d_type != DT_UNKNOWN)
         entryType = StorageTk::direntToDirEntryType(dirEntry->d_type);
      else
      {
         struct stat statBuf;
         int statRes = fstatat(dirFD, dirEntry->d_name, &statBuf, 0);

         if(statRes == 0)
            entryType = MetadataTk::posixFileTypeToDirEntryType(statBuf.st_mode);
         else
            entryType = DirEntryType_INVALID;
      }

      if ( (entryType != DirEntryType_DIRECTORY) || (!onlyFiles) )
      {
         outNames.push_back(dirEntry->d_name);
         outEntryTypes.push_back(int(entryType));
      }

      outNewOffset = dirEntry->d_off;
   }

   int errnoCopy = errno; // copy value before closedir() for readdir() error check below

   closedir(dirHandle);

   if(!dirEntry && errnoCopy)
   {
      LogContext(__func__).logErr("Unable to fetch chunk entries. "
         "targetID: " + StringTk::uintToStr(targetID) + "; "
            "isMirror: " + StringTk::intToStr((int) isMirror) + "; "
            "relativeDir: " + relativeDir + "; "
            "SysErr: " + System::getErrString(errnoCopy) );

      return FhgfsOpsErr_INTERNAL;
   }
   else
      return FhgfsOpsErr_SUCCESS;
}
