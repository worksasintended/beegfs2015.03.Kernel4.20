#include <app/App.h>
#include <program/Program.h>
#include "ModeHelp.h"


#define MODEHELP_ARG_SPECIFICHELP   RUNMODE_HELP_KEY_STRING /* print help for a specific mode */

int ModeHelp::execute()
{
   App* app = Program::getApp();
   Config* cfg = app->getConfig();

   RunMode runMode = cfg->determineRunMode();

   if(runMode == RunMode_INVALID)
      printGeneralHelp();
   else
      printSpecificHelp(runMode);

   std::cout << std::endl;

   return APPCODE_INVALID_CONFIG;
}

void ModeHelp::printGeneralHelp()
{
   std::cout << "FhGFS Command-Line Control Tool (http://www.fhgfs.com)" << std::endl;
   std::cout << "Version: " << BEEGFS_VERSION << std::endl;
   std::cout << std::endl;
   std::cout << "GENERAL USAGE:" << std::endl;
   std::cout << " $ beegfs-ctl --<modename> --help" << std::endl;
   std::cout << " $ beegfs-ctl --<modename> [mode_arguments] [client_arguments]" << std::endl;
   std::cout << std::endl;
   std::cout << "MODES:" << std::endl;
   std::cout << " --listnodes             => List registered clients and servers." << std::endl;
   std::cout << " --listtargets           => List metadata and storage targets." << std::endl;
   std::cout << " --removenode (*)        => Remove (unregister) a node." << std::endl;
   std::cout << " --removetarget (*)      => Remove (unregister) a storage target." << std::endl;
   std::cout << std::endl;
   std::cout << " --getentryinfo          => Show file system entry details." << std::endl;
   std::cout << " --setpattern (*)        => Set a new striping configuration." << std::endl;
   std::cout << " --mirrormd (*)          => Enable metadata mirroring. (EXPERIMENTAL)" << std::endl;
   //std::cout << " --reverselookup       => Resolve path for file or directory ID. (EXPERIMENTAL)" << std::endl;
   std::cout << " --find                  => Find files located on certain servers." << std::endl;
   std::cout << " --refreshentryinfo      => Refresh file system entry metadata." << std::endl;
   //std::cout << " --refreshallentries(*)=> Refresh metadata of all entries. (EXPERIMENTAL)" << std::endl;
   std::cout << std::endl;
   std::cout << " --createfile (*)        => Create a new file." << std::endl;
   std::cout << " --createdir (*)         => Create a new directory." << std::endl;
   std::cout << " --migrate               => Migrate files to other storage servers." << std::endl;
   std::cout << " --disposeunused (*)     => Purge remains of unlinked files." << std::endl;
   std::cout << std::endl;
   std::cout << " --serverstats           => Show server IO statistics." << std::endl;
   std::cout << " --clientstats           => Show client IO statistics." << std::endl;
   std::cout << " --userstats             => Show user IO statistics." << std::endl;
   std::cout << " --storagebench (*)      => Run a storage targets benchmark." << std::endl;
   std::cout << std::endl;
   std::cout << " --getquota              => Show quota information for users or groups." << std::endl;
   std::cout << " --setquota (*)          => Sets the quota limits for users or groups." << std::endl;
   std::cout << std::endl;
   std::cout << " --listmirrorgroups      => List mirror buddy groups." << std::endl;
   std::cout << " --addmirrorgroup (*)    => Add a mirror buddy group." << std::endl;
   std::cout << " --resyncstorage (*)     => Start resync of a storage target." << std::endl;
   std::cout << " --resyncstoragestats    => Get statistics on a storage target resync." << std::endl;
   std::cout << std::endl;
   std::cout << "*) Marked modes require root privileges." << std::endl;
   std::cout << std::endl;
   std::cout << "USAGE:" << std::endl;
   std::cout << " This is the FhGFS command-line control tool." << std::endl;
   std::cout << std::endl;
   std::cout << " Choose a control mode from the list above and use the parameter \"--help\" to" << std::endl;
   std::cout << " show arguments and usage examples for that particular mode." << std::endl;
   std::cout << std::endl;
   std::cout << " Example: Show help for mode \"--listnodes\"" << std::endl;
   std::cout << "  $ beegfs-ctl --listnodes --help" << std::endl;
}

/**
 * Note: don't call this with RunMode_INVALID.
 */
void ModeHelp::printSpecificHelp(RunMode runMode)
{
   printSpecificHelpHeader(); // print general usage and client options info

   switch(runMode)
   {
      case RunMode_GETENTRYINFO:
      { ModeGetEntryInfo::printHelp(); } break;

      case RunMode_SETPATTERN:
      { ModeSetPattern::printHelp(); } break;

      case RunMode_LISTNODES:
      { ModeGetNodes::printHelp(); } break;

      case RunMode_DISPOSEUNUSED:
      { ModeDisposeUnusedFiles::printHelp(); } break;

      case RunMode_CREATEFILE:
      { ModeCreateFile::printHelp(); } break;

      case RunMode_CREATEDIR:
      { ModeCreateDir::printHelp(); } break;

      case RunMode_REMOVENODE:
      { ModeRemoveNode::printHelp(); } break;

      case RunMode_IOSTAT:
      case RunMode_SERVERSTATS: // serverstats is an alias for iostat
      { ModeIOStat::printHelp(); } break;

      case RunMode_IOTEST:
      { ModeIOTest::printHelp(); } break;

      case RunMode_REFRESHENTRYINFO:
      { ModeRefreshEntryInfo::printHelp(); } break;

      case RunMode_REMOVEDIRENTRY:
      { ModeRemoveDirEntry::printHelp(); } break;

      case RunMode_MAPTARGET:
      { ModeMapTarget::printHelp(); } break;

      case RunMode_REMOVETARGET:
      { ModeRemoveTarget::printHelp(); } break;

      case RunMode_LISTTARGETS:
      { ModeListTargets::printHelp(); } break;

      case RunMode_IOCTL:
      { ModeIoctl::printHelp(); } break;

      case RunMode_GENERICDEBUG:
      { ModeGenericDebug::printHelp(); } break;

      case RunMode_CLIENTSTATS:
      case RunMode_USERSTATS: // userstats is an alias for "--clienstats --peruser"
      { ModeClientStats::printHelp(); } break;

      case RunMode_REFRESHALLENTRIES:
      { ModeRefreshAllEntries::printHelp(); } break;

      case RunMode_FIND:
      { ModeFind::printHelp(); } break;

      case RunMode_MIGRATE:
      { ModeMigrate::printHelpMigrate(); } break;

      case RunMode_REVERSELOOKUP:
      { ModeReverseLookup::printHelp(); } break;

      case RunMode_STORAGEBENCH:
      { ModeStorageBench::printHelp(); } break;

      case RunMode_SETMETADATAMIRRORING:
      { ModeSetMetadataMirroring::printHelp(); } break;

      case RunMode_GETQUOTAINFO:
      { ModeGetQuotaInfo::printHelp(); } break;

      case RunMode_SETQUOTALIMITS:
      { ModeSetQuota::printHelp(); } break;

      case RunMode_HASHDIR:
      { ModeHashDir::printHelp(); } break;

      case RunMode_ADDMIRRORBUDDYGROUP:
      { ModeAddMirrorBuddyGroup::printHelp(); } break;

      case RunMode_LISTMIRRORBUDDYGROUPS:
      { ModeListMirrorBuddyGroups::printHelp(); } break;

      case RunMode_STARTSTORAGERESYNC:
      { ModeStartStorageResync::printHelp(); } break;

      case RunMode_STORAGERESYNCSTATS:
      { ModeStorageResyncStats::printHelp(); } break;

      default:
      {
         std::cerr << "Error: Unhandled mode specified. Mode number: " << runMode << std::endl;
         std::cout << std::endl;

         printGeneralHelp();
      } break;
   }

}

/**
 * Print the help message header that applies to any specific mode help. Contains general usage
 * and client options info.
 */
void ModeHelp::printSpecificHelpHeader()
{
   std::cout << "GENERAL USAGE:" << std::endl;
   std::cout << "  $ beegfs-ctl --<modename> [mode_arguments] [client_arguments]" << std::endl;
   std::cout << std::endl;
   std::cout << "CLIENT ARGUMENTS:" << std::endl;
   std::cout << " Optional:" << std::endl;
   std::cout << "  --mount=<path>           Use config settings from this BeeGFS mountpoint" << std::endl;
   std::cout << "                           (instead of \"--cfgFile\")." << std::endl;
   std::cout << "  --cfgFile=<path>         Path to BeeGFS client config file." << std::endl;
   std::cout << "                           (Default: " CONFIG_DEFAULT_CFGFILENAME ")" << std::endl;
   std::cout << "  --logEnabled             Enable detailed logging." << std::endl;
   std::cout << "  --<key>=<value>          Any setting from the client config file to override" << std::endl;
   std::cout << "                           the config file values (e.g. \"--logLevel=5\")." << std::endl;
   std::cout << std::endl;
}

