#ifndef MODEHELP_H_
#define MODEHELP_H_

#include <common/Common.h>
#include <modes/migrate/ModeFind.h>
#include <modes/migrate/ModeMigrate.h>
#include <modes/mirroring/ModeAddMirrorBuddyGroup.h>
#include <modes/mirroring/ModeListMirrorBuddyGroups.h>
#include <modes/mirroring/ModeRemoveBuddyGroup.h>
#include <modes/mirroring/ModeSetMetadataMirroring.h>
#include <modes/mirroring/ModeStartStorageResync.h>
#include <modes/mirroring/ModeStorageResyncStats.h>
#include <modes/modeclientstats/ModeClientStats.h>
#include <modes/ModeCreateFile.h>
#include <modes/ModeCreateDir.h>
#include <modes/ModeDisposeUnusedFiles.h>
#include <modes/ModeGenericDebug.h>
#include <modes/ModeGetEntryInfo.h>
#include <modes/ModeGetNodes.h>
#include <modes/ModeGetQuotaInfo.h>
#include <modes/ModeHashDir.h>
#include <modes/ModeIoctl.h>
#include <modes/ModeIOStat.h>
#include <modes/ModeIOTest.h>
#include <modes/ModeListTargets.h>
#include <modes/ModeMapTarget.h>
#include <modes/ModeRefreshAllEntries.h>
#include <modes/ModeRefreshEntryInfo.h>
#include <modes/ModeRemoveDirEntry.h>
#include <modes/ModeRemoveNode.h>
#include <modes/ModeReverseLookup.h>
#include <modes/ModeSetPattern.h>
#include <modes/ModeSetQuota.h>
#include <modes/ModeStorageBench.h>
#include <modes/ModeRemoveTarget.h>

#include "Mode.h"


class ModeHelp : public Mode
{
   public:
      ModeHelp() {};
      
      virtual int execute();


   private:
      void printGeneralHelp();
      void printSpecificHelp(RunMode runMode);
      void printSpecificHelpHeader();
};


#endif /*MODEHELP_H_*/
