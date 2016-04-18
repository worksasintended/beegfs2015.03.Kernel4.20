#include <common/toolkit/MathTk.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/SessionTk.h>
#include <common/storage/striping/Raid0Pattern.h>
#include <net/msghelpers/MsgHelperTrunc.h>
#include <program/Program.h>
#include <storage/MetaStore.h>
#include "MsgHelperOpen.h"

/**
 * Note: This only does the basic open; you probably still want to create the session for this
 * opened file afterwards.
 * Note: Also performs truncation based on accessFlags if necessary.
 *
 * @param msgUserID only used for msg header info.
 * @param outOpenFile only set if return indicates success.
 */
FhgfsOpsErr MsgHelperOpen::openFile(EntryInfo* entryInfo, unsigned accessFlags,
   bool useQuota, unsigned msgUserID, FileInode** outFileInode)
{
   const char* logContext = "Open File Helper";
   IGNORE_UNUSED_VARIABLE(logContext);

   bool truncLocalRequired = false;

   if(accessFlags & OPENFILE_ACCESS_TRUNC)
      truncLocalRequired = MsgHelperTrunc::isTruncChunkRequired(entryInfo, 0);

   FhgfsOpsErr openRes = openMetaFile(entryInfo, accessFlags, outFileInode);

   if(openRes != FhgfsOpsErr_SUCCESS)
      return openRes;

   /* check chunkSize for compatibility.
      this check was introduced in 2011.05-r6, when we switched from arbitrary chunk sizes to the
      new chunk size constraints (min size and power of two). the check can be removed in a future
      release, when we are sure that there are no old installations with arbitrary chunk sizes
      left. */
   unsigned chunkSize = (*outFileInode)->getStripePattern()->getChunkSize();
   if(unlikely( (chunkSize < STRIPEPATTERN_MIN_CHUNKSIZE) ||
                !MathTk::isPowerOfTwo(chunkSize) ) )
   { // validity check failed => don't open this file (we would risk corrupting it otherwise)
      LogContext(logContext).logErr("This version of FhGFS is not compatible with this "
         "chunk size: " + StringTk::uintToStr(chunkSize) + ". "
         "Refusing to open file. "
         "parentInfo: " + entryInfo->getParentEntryID() + " "
         "entryInfo: " + entryInfo->getEntryID() );

      openMetaFileCompensate(entryInfo, *outFileInode, accessFlags);
      openRes = FhgfsOpsErr_INTERNAL;
   }

   if(truncLocalRequired)
   { // trunc was specified and is needed => do it
      LOG_DEBUG(logContext, Log_DEBUG, std::string("Opening with trunc local") );

      FhgfsOpsErr truncRes = MsgHelperTrunc::truncChunkFile(
         *outFileInode, entryInfo, 0, useQuota, msgUserID);

      if(unlikely(truncRes != FhgfsOpsErr_SUCCESS) )
      { // error => undo open()
         openMetaFileCompensate(entryInfo, *outFileInode, accessFlags);
         openRes = truncRes;
      }
   }

   return openRes;
}

FhgfsOpsErr MsgHelperOpen::openMetaFile(EntryInfo* entryInfo, unsigned accessFlags,
   FileInode** outOpenInode)
{
   MetaStore* metaStore = Program::getApp()->getMetaStore();

   FhgfsOpsErr openRes = metaStore->openFile(entryInfo, accessFlags, outOpenInode);

   return openRes;
}

/**
 * Undo an open.
 * (Typically called when an error occurred after a successful open).
 */
void MsgHelperOpen::openMetaFileCompensate(EntryInfo* entryInfo,
   FileInode* inode, unsigned accessFlags)
{
   MetaStore* metaStore = Program::getApp()->getMetaStore();
   unsigned numHardlinks; // ignored here
   unsigned numInodeRefs; // ignore here

   metaStore->closeFile(entryInfo, inode, accessFlags, &numHardlinks, &numInodeRefs);
}
