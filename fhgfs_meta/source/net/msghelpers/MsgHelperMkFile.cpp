#include <common/toolkit/MessagingTk.h>
#include <components/ModificationEventFlusher.h>
#include <program/Program.h>
#include "MsgHelperMkFile.h"


FhgfsOpsErr MsgHelperMkFile::mkFile(EntryInfo* parentInfo, MkFileDetails* mkDetails,
   UInt16List* preferredTargets, unsigned numtargets, unsigned chunksize, EntryInfo* outEntryInfo,
   FileInodeStoreData* outInodeData)
{
   const char* logContext = "MsgHelperMkFile (create file)";

   App* app = Program::getApp();
   MetaStore* metaStore = app->getMetaStore();
   ModificationEventFlusher* modEventFlusher = app->getModificationEventFlusher();
   bool modEventLoggingEnabled = modEventFlusher->isLoggingEnabled();

   FhgfsOpsErr retVal;

   // reference parent
   DirInode* dir = metaStore->referenceDir(parentInfo->getEntryID(), true);
   if(!dir)
      return FhgfsOpsErr_PATHNOTEXISTS;

   // create new stripe pattern
   StripePattern* stripePattern = dir->createFileStripePattern(
      preferredTargets, numtargets, chunksize);

   // check availability of stripe targets
   if(unlikely(!stripePattern ||  stripePattern->getStripeTargetIDs()->empty() ) )
   {
      LogContext(logContext).logErr(
         "Unable to create stripe pattern. No storage targets available? "
         "File: " + mkDetails->newName);

      SAFE_DELETE(stripePattern);
      retVal = FhgfsOpsErr_INTERNAL;
      goto cleanup;
   }

   // create meta file
   retVal = metaStore->mkNewMetaFile(dir, mkDetails, stripePattern, outEntryInfo, outInodeData);

   if ( (modEventLoggingEnabled ) && ( outEntryInfo ) )
   {
      std::string entryID = outEntryInfo->getEntryID();
      modEventFlusher->add(ModificationEvent_FILECREATED, entryID);
   }

cleanup:
   metaStore->releaseDir(parentInfo->getEntryID() );

   return retVal;
}


