#include "ModeCheckFS.h"

#include <common/toolkit/ListTk.h>
#include <common/toolkit/UnitTk.h>
#include <components/DataFetcher.h>
#include <components/worker/RetrieveChunksWork.h>
#include <net/msghelpers/MsgHelperRepair.h>
#include <toolkit/DatabaseTk.h>
#include <toolkit/FsckTkEx.h>

#include <program/Program.h>

#include <ftw.h>

template<unsigned Actions>
UserPrompter::UserPrompter(const FsckRepairAction (&possibleActions)[Actions],
   FsckRepairAction defaultRepairAction)
   : askForAction(true), possibleActions(possibleActions, possibleActions + Actions),
     repairAction(FsckRepairAction_UNDEFINED)
{
   if(Program::getApp()->getConfig()->getReadOnly() )
      askForAction = false;
   else
   if(Program::getApp()->getConfig()->getAutomatic() )
   {
      askForAction = false;
      repairAction = defaultRepairAction;
   }
}

FsckRepairAction UserPrompter::chooseAction(const std::string& prompt)
{
   if(askForAction)
      FsckTkEx::fsckOutput("> " + prompt, OutputOptions_LINEBREAK | OutputOptions_NOLOG);

   FsckTkEx::fsckOutput("> " + prompt, OutputOptions_NOSTDOUT);

   while(askForAction)
   {
      for(size_t i = 0; i < possibleActions.size(); i++)
      {
         FsckTkEx::fsckOutput(
            "   " + StringTk::uintToStr(i + 1) + ") "
               + FsckTkEx::getRepairActionDesc(possibleActions[i]), OutputOptions_NOLOG |
               OutputOptions_LINEBREAK);
      }

      for(size_t i = 0; i < possibleActions.size(); i++)
      {
         FsckTkEx::fsckOutput(
            "   " + StringTk::uintToStr(i + possibleActions.size() + 1) + ") "
               + FsckTkEx::getRepairActionDesc(possibleActions[i]) + " (apply for all)",
               OutputOptions_NOLOG | OutputOptions_LINEBREAK);
      }

      std::string inputStr;
      getline(std::cin, inputStr);

      unsigned input = StringTk::strToUInt(inputStr);

      if( (input > 0) && (input <= possibleActions.size() ) )
      {
         // user chose for this error
         repairAction = possibleActions[input - 1];
         break;
      }

      if( (input > possibleActions.size() ) && (input <= possibleActions.size() * 2) )
      {
         // user chose for all errors => do not ask again
         askForAction = false;
         repairAction = possibleActions[input - possibleActions.size() - 1];
         break;
      }
   }

   FsckTkEx::fsckOutput(" - [ra: " + FsckTkEx::getRepairActionDesc(repairAction, true) + "]",
      OutputOptions_LINEBREAK | OutputOptions_NOSTDOUT);

   return repairAction;
}



ModeCheckFS::ModeCheckFS()
 : log("ModeCheckFS"),
   lostAndFoundNode(NULL)
{
}

ModeCheckFS::~ModeCheckFS()
{
}

void ModeCheckFS::printHelp()
{
   std::cout <<
      "MODE ARGUMENTS:\n"
      " Optional:\n"
      "  --readOnly             Check only, skip repairs.\n"
      "  --runOnline            Run in online mode, which means user may access the\n"
      "                         file system while the check is running.\n"
      "  --automatic            Do not prompt for repair actions and automatically use\n"
      "                         reasonable default actions.\n"
      "  --noFetch              Do not build a new database from the servers and\n"
      "                         instead work on an existing database (e.g. from a\n"
      "                         previous read-only run).\n"
      "  --quotaEnabled         Enable checks for quota support.\n"
      "  --databasePath=<path>  Path to store for the database files. On systems with \n"
      "                         many files, the database can grow to a size of several \n"
      "                         100 GB.\n"
      "                         (Default: " CONFIG_DEFAULT_DBPATH ")\n"
      "  --overwriteDbFile      Overwrite an existing database file without prompt.\n"
      "  --ignoreDBDiskSpace    Ignore free disk space check for database file.\n"
      "  --logOutFile=<path>    Path to the fsck output file, which contains a copy of\n"
      "                         the console output.\n"
      "                         (Default: " CONFIG_DEFAULT_OUTFILE ")\n"
      "  --logStdFile=<path>    Path to the program error log file, which contains e.g.\n"
      "                         network error messages.\n"
      "                         (Default: " CONFIG_DEFAULT_LOGFILE ")\n"
      "\n"
      "USAGE:\n"
      " This mode performs a full check and optional repair of a BeeGFS file system\n"
      " instance by building a database of the current file system contents on the\n"
      " local machine.\n"
      "\n"
      " The fsck gathers information from all BeeGFS server daemons in parallel through\n"
      " their configured network interfaces. All server components of the file\n"
      " system have to be running to start a check.\n"
      "\n"
      " If the fsck is running without the \"--runonline\" argument, users may not\n"
      " access the file system during a run (otherwise false errors might be reported).\n"
      "\n"
      " Example: Check for errors, but skip repairs\n"
      "  $ beegfs-fsck --checkfs --runonline --readonly\n";

   std::cout << std::flush;
}

int ModeCheckFS::execute()
{
   App* app = Program::getApp();
   Config *cfg = app->getConfig();
   std::string databasePath = cfg->getDatabasePath();

   // check root privileges
   if ( geteuid() && getegid() )
   { // no root privileges
      FsckTkEx::printVersionHeader(true, true);
      FsckTkEx::fsckOutput("Error: beegfs-fsck requires root privileges.",
         OutputOptions_NOLOG | OutputOptions_STDERR | OutputOptions_DOUBLELINEBREAK);
      return APPCODE_INITIALIZATION_ERROR;
   }

   if ( this->checkInvalidArgs(cfg->getUnknownConfigArgs()) )
      return APPCODE_INVALID_CONFIG;

   FsckTkEx::printVersionHeader(false);
   printHeaderInformation();

   if ( !FsckTkEx::checkReachability() )
      return APPCODE_COMMUNICATION_ERROR;

   if(cfg->getNoFetch() )
   {
      try {
         this->database.reset(new FsckDB(databasePath + "/fsckdb", cfg->getTuneDbFragmentSize(),
            cfg->getTuneDentryCacheSize(), false) );
      } catch (const FragmentDoesNotExist& e) {
         std::string err = "Database was found to be incomplete in path " + databasePath;
         log.logErr(err);
         FsckTkEx::fsckOutput(err);
         return APPCODE_RUNTIME_ERROR;
      }
   }
   else
   {
      int initDBRes = initDatabase();
      if ( initDBRes )
         return initDBRes;

      boost::scoped_ptr<ModificationEventHandler> modificationEventHandler;

      if ( cfg->getRunOnline() )
      {
         modificationEventHandler.reset(
            new ModificationEventHandler(*this->database->getModificationEventsTable() ) );
         modificationEventHandler->start();
         Program::getApp()->setModificationEventHandler(modificationEventHandler.get() );

         // start modification logging
         bool startLogRes = FsckTkEx::startModificationLogging(app->getMetaNodes(),
            app->getLocalNode());

         if ( !startLogRes )
         {
            std::string errStr = "Unable to start file system modification logging. Fsck cannot "
               "proceed";

            log.logErr(errStr);
            FsckTkEx::fsckOutput(errStr);

            return APPCODE_RUNTIME_ERROR;
         }
      }

      bool gatherDataRes = gatherData();

      if ( cfg->getRunOnline() )
      {
         // stop modification logging
         bool eventLoggingOK = FsckTkEx::stopModificationLogging(app->getMetaNodes());
         // stop mod event handler (to make it flush for the last time
         Program::getApp()->setModificationEventHandler(NULL);
         modificationEventHandler->selfTerminate();
         modificationEventHandler->join();

         // if event logging is not OK (i.e. that not all events might have been processed), go
         // into read-only mode
         if ( !eventLoggingOK )
         {
            Program::getApp()->getConfig()->disableAutomaticRepairMode();
            Program::getApp()->getConfig()->setReadOnly();

            FsckTkEx::fsckOutput("-----",
               OutputOptions_ADDLINEBREAKBEFORE | OutputOptions_FLUSH | OutputOptions_LINEBREAK);
            FsckTkEx::fsckOutput(
               "WARNING: Fsck did not get all modification events from metadata servers. ",
               OutputOptions_FLUSH | OutputOptions_LINEBREAK);
            FsckTkEx::fsckOutput(
               "For instance, this might have happened due to network timeouts.",
               OutputOptions_FLUSH | OutputOptions_LINEBREAK);
            FsckTkEx::fsckOutput(
               "This means the online run is not aware of all changes in the filesystem and it"
               "is not safe to trigger repair actions. This might even result in data loss!",
               OutputOptions_FLUSH | OutputOptions_DOUBLELINEBREAK);
            FsckTkEx::fsckOutput(
               "Thus, beegfs-fsck automatically enabled read-only mode. However, you can still "
               "force repair by running beegfs-fsck again on the existing database with the "
               "--noFetch option.",
               OutputOptions_FLUSH | OutputOptions_DOUBLELINEBREAK);
            FsckTkEx::fsckOutput("Please press any key to continue.",
               OutputOptions_FLUSH | OutputOptions_LINEBREAK);
            FsckTkEx::fsckOutput("-----", OutputOptions_FLUSH | OutputOptions_DOUBLELINEBREAK);

            std::cin.get();
         }

      }

      if ( !gatherDataRes )
      {
         std::string errStr =
            "An error occured while fetching data from servers. Fsck cannot proceed. "
               "Please see log file for more information";

         log.logErr(errStr);
         FsckTkEx::fsckOutput(errStr);

         return APPCODE_RUNTIME_ERROR;
      }
   }

   checkAndRepair();

   return APPCODE_NO_ERROR;
}

/*
 * initializes the database, this is done here and not inside the main app, because we do not want
 * it to effect the help modes; DB file shall only be created if user really runs a check
 *
 * @return integer value symbolizing an appcode
 */
int ModeCheckFS::initDatabase()
{
   Config* cfg = Program::getApp()->getConfig();

   // create the database path
   Path dbPath(cfg->getDatabasePath() + "/fsckdb");

   if ( !StorageTk::createPathOnDisk(dbPath, false) )
   {
      FsckTkEx::fsckOutput("Could not create path for database files: " +
         dbPath.getPathAsStr());
      return APPCODE_INITIALIZATION_ERROR;
   }

   // check disk space
   if ( !FsckTkEx::checkDiskSpace(dbPath) )
      return APPCODE_INITIALIZATION_ERROR;

   // check if DB file path already exists and is not empty
   if ( StorageTk::pathExists(dbPath.getPathAsStr())
      && StorageTk::pathHasChildren(dbPath.getPathAsStr()) && (!cfg->getOverwriteDbFile()) )
   {
      FsckTkEx::fsckOutput("The database path already exists: " + dbPath.getPathAsStr());
      FsckTkEx::fsckOutput("If you continue now any existing database files in that path will be "
         "deleted.");

      char input = '\0';

      while ( (input != 'N') && (input != 'n') && (input != 'Y') && (input != 'y') )
      {
         FsckTkEx::fsckOutput("Do you want to continue? (Y/N)");
         std::cin >> input;
      }

      switch(input)
      {
         case 'Y':
         case 'y':
            break; // just do nothing and go ahead
         case 'N':
         case 'n':
         default:
            // abort here
            return APPCODE_USER_ABORTED;
      }
   }

   struct ops
   {
      static int visit(const char* path, const struct stat*, int type, struct FTW* state)
      {
         if(state->level == 0)
            return 0;
         else
         if(type == FTW_F || type == FTW_SL)
            return ::unlink(path);
         else
            return ::rmdir(path);
      }
   };

   int ftwRes = ::nftw(dbPath.getPathAsStr().c_str(), ops::visit, 10, FTW_DEPTH | FTW_PHYS);
   if(ftwRes)
   {
      FsckTkEx::fsckOutput("Could not empty path for database files: " + dbPath.getPathAsStr() );
      return APPCODE_INITIALIZATION_ERROR;
   }

   try {
      this->database.reset(
         new FsckDB(dbPath.getPathAsStr(), cfg->getTuneDbFragmentSize(),
            cfg->getTuneDentryCacheSize(), true) );
   } catch (const std::runtime_error& e) {
      FsckTkEx::fsckOutput("Database " + dbPath.getPathAsStr() + " is corrupt");
      return APPCODE_RUNTIME_ERROR;
   }

   return 0;
}

void ModeCheckFS::printHeaderInformation()
{
   Config* cfg = Program::getApp()->getConfig();

   // get the current time and create some nice output, so the user can see at which time the run
   // was started (especially nice to find older runs in the log file)
   time_t t;
   time(&t);
   std::string timeStr = std::string(ctime(&t));
   FsckTkEx::fsckOutput(
      "Started BeeGFS fsck in forward check mode [" + timeStr.substr(0, timeStr.length() - 1)
         + "]\nLog will be written to " + cfg->getLogStdFile() + "\nDatabase will be saved in "
         + cfg->getDatabasePath(), OutputOptions_LINEBREAK | OutputOptions_HEADLINE);
}

bool ModeCheckFS::gatherData()
{
   bool retVal;

   FsckTkEx::fsckOutput("Step 2: Gather data from nodes: ", OutputOptions_DOUBLELINEBREAK);

   DataFetcher dataFetcher(*this->database);
   retVal = dataFetcher.execute();

   FsckTkEx::fsckOutput("", OutputOptions_LINEBREAK);

   return retVal;
}

template<typename Obj, typename State>
int64_t ModeCheckFS::checkAndRepairGeneric(Cursor<Obj> cursor,
   void (ModeCheckFS::*repair)(Obj&, State&), State& state)
{
   int64_t errorCount = 0;

   while(cursor.step() )
   {
      Obj* entry = cursor.get();

      (this->*repair)(*entry, state);
      errorCount++;
   }

   if(errorCount)
      FsckTkEx::fsckOutput(">>> Found " + StringTk::int64ToStr(errorCount)
         + " errors. Detailed information can also be found in "
         + Program::getApp()->getConfig()->getLogOutFile() + ".",
      OutputOptions_DOUBLELINEBREAK);

   return errorCount;
}

int64_t ModeCheckFS::checkAndRepairDanglingDentry()
{
   FsckRepairAction fileActions[] = {
      FsckRepairAction_NOTHING,
      FsckRepairAction_DELETEDENTRY,
   };

   FsckRepairAction dirActions[] = {
      FsckRepairAction_NOTHING,
      FsckRepairAction_DELETEDENTRY,
      FsckRepairAction_CREATEDEFAULTDIRINODE,
   };

   UserPrompter forFiles(fileActions, FsckRepairAction_DELETEDENTRY);
   UserPrompter forDirs(dirActions, FsckRepairAction_CREATEDEFAULTDIRINODE);

   std::pair<UserPrompter*, UserPrompter*> prompt(&forFiles, &forDirs);

   FsckTkEx::fsckOutput("* Dangling directory entry ...",
      OutputOptions_FLUSH |OutputOptions_LINEBREAK);

   return checkAndRepairGeneric(this->database->findDanglingDirEntries(),
      &ModeCheckFS::repairDanglingDirEntry, prompt);
}

int64_t ModeCheckFS::checkAndRepairWrongInodeOwner()
{
   FsckRepairAction possibleActions[] = {
      FsckRepairAction_NOTHING,
      FsckRepairAction_CORRECTOWNER,
   };

   UserPrompter prompt(possibleActions, FsckRepairAction_CORRECTOWNER);

   FsckTkEx::fsckOutput("* Wrong owner node saved in inode ...",
      OutputOptions_FLUSH | OutputOptions_LINEBREAK);

   return checkAndRepairGeneric(this->database->findInodesWithWrongOwner(),
      &ModeCheckFS::repairWrongInodeOwner, prompt);
}

int64_t ModeCheckFS::checkAndRepairWrongOwnerInDentry()
{
   FsckRepairAction possibleActions[] = {
      FsckRepairAction_NOTHING,
      FsckRepairAction_CORRECTOWNER,
   };

   UserPrompter prompt(possibleActions, FsckRepairAction_CORRECTOWNER);

   FsckTkEx::fsckOutput("* Dentry points to inode on wrong node ...",
      OutputOptions_FLUSH | OutputOptions_LINEBREAK);

   return checkAndRepairGeneric(this->database->findDirEntriesWithWrongOwner(),
      &ModeCheckFS::repairWrongInodeOwnerInDentry, prompt);
}

int64_t ModeCheckFS::checkAndRepairOrphanedContDir()
{
   FsckRepairAction possibleActions[] = {
      FsckRepairAction_NOTHING,
      FsckRepairAction_CREATEDEFAULTDIRINODE,
   };

   UserPrompter prompt(possibleActions, FsckRepairAction_CREATEDEFAULTDIRINODE);

   FsckTkEx::fsckOutput("* Content directory without an inode ...",
      OutputOptions_FLUSH | OutputOptions_LINEBREAK);

   return checkAndRepairGeneric(this->database->findOrphanedContDirs(),
      &ModeCheckFS::repairOrphanedContDir, prompt);
}

int64_t ModeCheckFS::checkAndRepairOrphanedDirInode()
{
   FsckRepairAction possibleActions[] = {
      FsckRepairAction_NOTHING,
      FsckRepairAction_LOSTANDFOUND,
   };

   UserPrompter prompt(possibleActions, FsckRepairAction_LOSTANDFOUND);

   FsckTkEx::fsckOutput("* Dir inode without a dentry pointing to it (orphaned inode) ...",
      OutputOptions_FLUSH | OutputOptions_LINEBREAK);

   bool result = checkAndRepairGeneric(this->database->findOrphanedDirInodes(),
      &ModeCheckFS::repairOrphanedDirInode, prompt);

   releaseLostAndFound();
   return result;
}

int64_t ModeCheckFS::checkAndRepairOrphanedFileInode()
{
   FsckRepairAction possibleActions[] = {
      FsckRepairAction_NOTHING,
      FsckRepairAction_DELETEINODE,
      //FsckRepairAction_LOSTANDFOUND,
   };

   UserPrompter prompt(possibleActions, FsckRepairAction_DELETEINODE);

   FsckTkEx::fsckOutput("* File inode without a dentry pointing to it (orphaned inode) ...",
      OutputOptions_FLUSH | OutputOptions_LINEBREAK);

   return checkAndRepairGeneric(this->database->findOrphanedFileInodes(),
      &ModeCheckFS::repairOrphanedFileInode, prompt);
}

int64_t ModeCheckFS::checkAndRepairOrphanedChunk()
{
   FsckRepairAction possibleActions[] = {
      FsckRepairAction_NOTHING,
      FsckRepairAction_DELETECHUNK,
   };

   UserPrompter prompt(possibleActions, FsckRepairAction_DELETECHUNK);
   RepairChunkState state = { &prompt, "", FsckRepairAction_UNDEFINED };

   FsckTkEx::fsckOutput("* Chunk without an inode pointing to it (orphaned chunk) ...",
      OutputOptions_FLUSH | OutputOptions_LINEBREAK);

   return checkAndRepairGeneric(this->database->findOrphanedChunks(),
      &ModeCheckFS::repairOrphanedChunk, state);
}

int64_t ModeCheckFS::checkAndRepairMissingContDir()
{
   FsckRepairAction possibleActions[] = {
      FsckRepairAction_NOTHING,
      FsckRepairAction_CREATECONTDIR,
   };

   UserPrompter prompt(possibleActions, FsckRepairAction_CREATECONTDIR);

   FsckTkEx::fsckOutput("* Directory inode without a content directory ...",
      OutputOptions_FLUSH | OutputOptions_LINEBREAK);

   return checkAndRepairGeneric(this->database->findInodesWithoutContDir(),
      &ModeCheckFS::repairMissingContDir, prompt);
}

int64_t ModeCheckFS::checkAndRepairWrongFileAttribs()
{
   FsckRepairAction possibleActions[] = {
      FsckRepairAction_NOTHING,
      FsckRepairAction_UPDATEATTRIBS,
   };

   UserPrompter prompt(possibleActions, FsckRepairAction_UPDATEATTRIBS);

   FsckTkEx::fsckOutput("* Attributes of file inode are wrong ...",
      OutputOptions_FLUSH | OutputOptions_LINEBREAK);

   return checkAndRepairGeneric(this->database->findWrongInodeFileAttribs(),
      &ModeCheckFS::repairWrongFileAttribs, prompt);
}

int64_t ModeCheckFS::checkAndRepairWrongDirAttribs()
{
   FsckRepairAction possibleActions[] = {
      FsckRepairAction_NOTHING,
      FsckRepairAction_UPDATEATTRIBS,
   };

   UserPrompter prompt(possibleActions, FsckRepairAction_UPDATEATTRIBS);

   FsckTkEx::fsckOutput("* Attributes of dir inode are wrong ...",
      OutputOptions_FLUSH | OutputOptions_LINEBREAK);

   return checkAndRepairGeneric(this->database->findWrongInodeDirAttribs(),
      &ModeCheckFS::repairWrongDirAttribs, prompt);
}

int64_t ModeCheckFS::checkAndRepairFilesWithMissingTargets()
{
   FsckRepairAction possibleActions[] = {
      FsckRepairAction_NOTHING,
      FsckRepairAction_DELETEFILE,
   };

   UserPrompter prompt(possibleActions, FsckRepairAction_NOTHING);

   FsckTkEx::fsckOutput("* File has a missing target in stripe pattern ...",
      OutputOptions_FLUSH | OutputOptions_LINEBREAK);

   return checkAndRepairGeneric(
      this->database->findFilesWithMissingStripeTargets(
         Program::getApp()->getTargetMapper(), Program::getApp()->getMirrorBuddyGroupMapper() ),
      &ModeCheckFS::repairFileWithMissingTargets, prompt);
}

int64_t ModeCheckFS::checkAndRepairDirEntriesWithBrokeByIDFile()
{
   FsckRepairAction possibleActions[] = {
      FsckRepairAction_NOTHING,
      FsckRepairAction_RECREATEFSID,
   };

   UserPrompter prompt(possibleActions, FsckRepairAction_RECREATEFSID);

   FsckTkEx::fsckOutput("* Dentry-by-ID file is broken or missing ...",
      OutputOptions_FLUSH | OutputOptions_LINEBREAK);

   return checkAndRepairGeneric(this->database->findDirEntriesWithBrokenByIDFile(),
      &ModeCheckFS::repairDirEntryWithBrokenByIDFile, prompt);
}

int64_t ModeCheckFS::checkAndRepairOrphanedDentryByIDFiles()
{
   FsckRepairAction possibleActions[] = {
      FsckRepairAction_NOTHING,
      FsckRepairAction_RECREATEDENTRY,
   };

   UserPrompter prompt(possibleActions, FsckRepairAction_RECREATEDENTRY);

   FsckTkEx::fsckOutput("* Dentry-by-ID file is present, but no corresponding dentry ...",
      OutputOptions_FLUSH | OutputOptions_LINEBREAK);

   return checkAndRepairGeneric(this->database->findOrphanedFsIDFiles(),
      &ModeCheckFS::repairOrphanedDentryByIDFile, prompt);
}

int64_t ModeCheckFS::checkAndRepairChunksWithWrongPermissions()
{
   FsckRepairAction possibleActions[] = {
      FsckRepairAction_NOTHING,
      FsckRepairAction_FIXPERMISSIONS,
   };

   UserPrompter prompt(possibleActions, FsckRepairAction_FIXPERMISSIONS);

   FsckTkEx::fsckOutput("* Chunk has wrong permissions ...",
      OutputOptions_FLUSH | OutputOptions_LINEBREAK);

   return checkAndRepairGeneric(this->database->findChunksWithWrongPermissions(),
      &ModeCheckFS::repairChunkWithWrongPermissions, prompt);
}

// no repair at the moment
int64_t ModeCheckFS::checkAndRepairChunksInWrongPath()
{
   FsckRepairAction possibleActions[] = {
      FsckRepairAction_NOTHING,
      FsckRepairAction_MOVECHUNK,
   };

   UserPrompter prompt(possibleActions, FsckRepairAction_MOVECHUNK);

   FsckTkEx::fsckOutput("* Chunk is saved in wrong path ...",
      OutputOptions_FLUSH | OutputOptions_LINEBREAK);

   return checkAndRepairGeneric(this->database->findChunksInWrongPath(),
      &ModeCheckFS::repairWrongChunkPath, prompt);
}

int64_t ModeCheckFS::checkDuplicateInodeIDs()
{
   FsckTkEx::fsckOutput("* Duplicated inode IDs ...",
      OutputOptions_FLUSH | OutputOptions_LINEBREAK);

   int dummy = 0;
   return checkAndRepairGeneric(this->database->findDuplicateInodeIDs(),
      &ModeCheckFS::logDuplicateInodeID, dummy);
}

void ModeCheckFS::logDuplicateInodeID(std::pair<db::EntryID, std::set<uint32_t> >& dups, int&)
{
   FsckTkEx::fsckOutput(">>> Found duplicated ID " + dups.first.str(),
      OutputOptions_LINEBREAK | OutputOptions_NOSTDOUT);
   for(std::set<uint32_t>::const_iterator it = dups.second.begin(), end = dups.second.end();
         it != end; ++it)
   {
      FsckTkEx::fsckOutput("   * Found on node " + StringTk::uintToStr(*it),
         OutputOptions_LINEBREAK | OutputOptions_NOSTDOUT);
   }
}

int64_t ModeCheckFS::checkDuplicateChunks()
{
   FsckTkEx::fsckOutput("* Duplicated chunks ...", OutputOptions_FLUSH | OutputOptions_LINEBREAK);

   int dummy = 0;
   return checkAndRepairGeneric(this->database->findDuplicateChunks(),
      &ModeCheckFS::logDuplicateChunk, dummy);
}

void ModeCheckFS::logDuplicateChunk(std::list<FsckChunk>& dups, int&)
{
   FsckTkEx::fsckOutput(">>> Found duplicated Chunks for ID " + dups.begin()->getID(),
      OutputOptions_LINEBREAK | OutputOptions_NOSTDOUT);
   for(std::list<FsckChunk>::iterator it = dups.begin(), end = dups.end();
         it != end; ++it)
   {
      FsckTkEx::fsckOutput("   * Found on target " + StringTk::uintToStr(it->getTargetID() )
         + (it->getBuddyGroupID()
               ? ", buddy group " + StringTk::uintToStr(it->getBuddyGroupID() )
               : "")
         + " in path " + it->getSavedPath()->getPathAsStr(),
         OutputOptions_LINEBREAK | OutputOptions_NOSTDOUT);
   }
}

void ModeCheckFS::checkAndRepair()
{
   FsckTkEx::fsckOutput("Step 3: Check for errors... ", OutputOptions_DOUBLELINEBREAK);

   Config* cfg = Program::getApp()->getConfig();

   int64_t errorCount = 0;

   errorCount += checkDuplicateInodeIDs();
   errorCount += checkDuplicateChunks();

   if(errorCount)
   {
      FsckTkEx::fsckOutput("Found errors beegfs-fsck cannot fix. Please consult the log for "
         "more information.", OutputOptions_LINEBREAK);
      return;
   }

   errorCount += checkAndRepairFilesWithMissingTargets();
   errorCount += checkAndRepairOrphanedDentryByIDFiles();
   errorCount += checkAndRepairDirEntriesWithBrokeByIDFile();
   errorCount += checkAndRepairChunksInWrongPath();
   errorCount += checkAndRepairWrongInodeOwner();
   errorCount += checkAndRepairWrongOwnerInDentry();
   errorCount += checkAndRepairOrphanedContDir();
   errorCount += checkAndRepairOrphanedDirInode();
   errorCount += checkAndRepairOrphanedFileInode();
   errorCount += checkAndRepairDanglingDentry();
   errorCount += checkAndRepairOrphanedChunk();
   errorCount += checkAndRepairMissingContDir();
   errorCount += checkAndRepairWrongFileAttribs();
   errorCount += checkAndRepairWrongDirAttribs();

   if ( cfg->getQuotaEnabled())
   {
      errorCount += checkAndRepairChunksWithWrongPermissions();
   }

   if ( cfg->getReadOnly() )
   {
      FsckTkEx::fsckOutput(
         "Found " + StringTk::int64ToStr(errorCount)
            + " errors. Detailed information can also be found in "
            + Program::getApp()->getConfig()->getLogOutFile() + ".",
         OutputOptions_ADDLINEBREAKBEFORE | OutputOptions_LINEBREAK);
      return;
   }

   if(errorCount > 0)
      FsckTkEx::fsckOutput(">>> Found " + StringTk::int64ToStr(errorCount) + " errors <<< ",
         OutputOptions_ADDLINEBREAKBEFORE | OutputOptions_LINEBREAK);
}

//////////////////////////
// internals      ///////
/////////////////////////
void ModeCheckFS::repairDanglingDirEntry(db::DirEntry& entry,
   std::pair<UserPrompter*, UserPrompter*>& prompt)
{
   FsckDirEntry fsckEntry = entry;

   FsckRepairAction action;
   std::string promptText = "Entry ID: " + fsckEntry.getID() + "; Path: "
      + this->database->getDentryTable()->getPathOf(entry);

   if(fsckEntry.getEntryType() == FsckDirEntryType_DIRECTORY)
      action = prompt.second->chooseAction(promptText);
   else
      action = prompt.first->chooseAction(promptText);

   fsckEntry.setName(this->database->getDentryTable()->getNameOf(entry) );

   FsckDirEntryList entries(1, fsckEntry);
   FsckDirEntryList failedEntries;

   NodeStore* nodes = Program::getApp()->getMetaNodes();

   switch(action)
   {
   case FsckRepairAction_UNDEFINED:
   case FsckRepairAction_NOTHING:
      break;

   case FsckRepairAction_DELETEDENTRY: {
      Node* node = nodes->referenceNode(fsckEntry.getSaveNodeID() );

      MsgHelperRepair::deleteDanglingDirEntries(node, &entries, &failedEntries);

      if(failedEntries.empty() )
      {
         this->database->getDentryTable()->remove(entries);
         this->deleteFsIDsFromDB(entries);
      }

      nodes->releaseNode(&node);
      break;
   }

   case FsckRepairAction_CREATEDEFAULTDIRINODE: {
      Node* node = nodes->referenceNode(fsckEntry.getSaveNodeID() );

      StringList inodeIDs(1, fsckEntry.getID() );
      StringList failedCreates;

      FsckDirInodeList createdInodes;

      MsgHelperRepair::createDefDirInodes(node, &inodeIDs, &createdInodes);

      this->database->getDirInodesTable()->insert(createdInodes);

      nodes->releaseNode(&node);
      break;
   }

   default:
      throw std::runtime_error("bad repair action");
   }
}

void ModeCheckFS::repairWrongInodeOwner(FsckDirInode& inode, UserPrompter& prompt)
{
   FsckRepairAction action = prompt.chooseAction("Entry ID: " + inode.getID()
      + "; Path: "
      + this->database->getDentryTable()->getPathOf(db::EntryID::fromStr(inode.getID() ) ) );

   switch(action)
   {
   case FsckRepairAction_UNDEFINED:
   case FsckRepairAction_NOTHING:
      break;

   case FsckRepairAction_CORRECTOWNER: {
      NodeStore* nodes = Program::getApp()->getMetaNodes();
      Node* node = nodes->referenceNode(inode.getSaveNodeID() );

      inode.setOwnerNodeID(inode.getSaveNodeID() );

      FsckDirInodeList inodes(1, inode);
      FsckDirInodeList failed;

      MsgHelperRepair::correctInodeOwners(node, &inodes, &failed);

      if(failed.empty() )
         this->database->getDirInodesTable()->update(inodes);

      nodes->releaseNode(&node);
      break;
   }

   default:
      throw std::runtime_error("bad repair action");
   }
}

void ModeCheckFS::repairWrongInodeOwnerInDentry(std::pair<db::DirEntry, uint16_t>& error,
   UserPrompter& prompt)
{
   FsckDirEntry fsckEntry = error.first;

   FsckRepairAction action = prompt.chooseAction("File ID: " + fsckEntry.getID()
      + "; Path: " + this->database->getDentryTable()->getPathOf(error.first) );

   switch(action)
   {
   case FsckRepairAction_UNDEFINED:
   case FsckRepairAction_NOTHING:
      break;

   case FsckRepairAction_CORRECTOWNER: {
      NodeStore* nodes = Program::getApp()->getMetaNodes();
      Node* node = nodes->referenceNode(fsckEntry.getSaveNodeID() );

      fsckEntry.setName(this->database->getDentryTable()->getNameOf(error.first) );
      fsckEntry.setEntryOwnerNodeID(error.second);

      FsckDirEntryList dentries(1, fsckEntry);
      FsckDirEntryList failed;

      MsgHelperRepair::correctInodeOwnersInDentry(node, &dentries, &failed);

      if(failed.empty() )
         this->database->getDentryTable()->updateFieldsExceptParent(dentries);

      nodes->releaseNode(&node);
      break;
   }

   default:
      throw std::runtime_error("bad repair action");
   }
}

void ModeCheckFS::repairOrphanedDirInode(FsckDirInode& inode, UserPrompter& prompt)
{
   FsckRepairAction action = prompt.chooseAction("Directory ID: " + inode.getID() );

   switch(action)
   {
   case FsckRepairAction_UNDEFINED:
   case FsckRepairAction_NOTHING:
      break;

   case FsckRepairAction_LOSTANDFOUND: {
      if(!ensureLostAndFoundExists() )
      {
         log.logErr("Orphaned dir inodes could not be linked to lost+found, because lost+found "
            "directory could not be created");
         return;
      }

      FsckDirInodeList inodes(1, inode);
      FsckDirEntryList created;
      FsckDirInodeList failed;

      MsgHelperRepair::linkToLostAndFound(this->lostAndFoundNode, &this->lostAndFoundInfo, &inodes,
         &failed, &created);

      if(failed.empty() )
         this->database->getDentryTable()->insert(created);

      // on server side, each dentry also created a dentry-by-ID file
      FsckFsIDList createdFsIDs;
      for(FsckDirEntryListIter iter = created.begin(); iter != created.end(); iter++)
      {
         FsckFsID fsID(iter->getID(), iter->getParentDirID(), iter->getSaveNodeID(),
            iter->getSaveDevice(), iter->getSaveInode());
         createdFsIDs.push_back(fsID);
      }

      this->database->getFsIDsTable()->insert(createdFsIDs);
      break;
   }

   default:
      throw std::runtime_error("bad repair action");
   }
}

void ModeCheckFS::repairOrphanedFileInode(FsckFileInode& inode, UserPrompter& prompt)
{
   FsckRepairAction action = prompt.chooseAction("File ID: " + inode.getID() );

   switch(action)
   {
   case FsckRepairAction_UNDEFINED:
   case FsckRepairAction_NOTHING:
      break;

   case FsckRepairAction_DELETEINODE: {
      NodeStore* metaNodeStore = Program::getApp()->getMetaNodes();
      Node* node = metaNodeStore->referenceNode(inode.getSaveNodeID() );

      FsckFileInodeList inodes(1, inode);
      StringList failed;

      MsgHelperRepair::deleteFileInodes(node, inodes, failed);

      if(failed.empty() )
         this->database->getFileInodesTable()->remove(inodes);

      metaNodeStore->releaseNode(&node);
      break;
   }

   default:
      throw std::runtime_error("bad repair action");
   }
}

void ModeCheckFS::repairOrphanedChunk(FsckChunk& chunk, RepairChunkState& state)
{
   // ask for repair action only once per chunk id, not once per chunk id and node
   if(state.lastID != chunk.getID() )
      state.lastChunkAction = state.prompt->chooseAction("Chunk ID: " + chunk.getID() );

   state.lastID = chunk.getID();

   switch(state.lastChunkAction)
   {
   case FsckRepairAction_UNDEFINED:
   case FsckRepairAction_NOTHING:
      break;

   case FsckRepairAction_DELETECHUNK: {
      FhgfsOpsErr refErr;
      NodeStore* storageNodeStore = Program::getApp()->getStorageNodes();
      Node* node = storageNodeStore->referenceNodeByTargetID(chunk.getTargetID(),
         Program::getApp()->getTargetMapper(), &refErr);

      if(!node)
      {
         FsckTkEx::fsckOutput("could not get storage target "
            + StringTk::uintToStr(chunk.getTargetID() ), OutputOptions_LINEBREAK);
         return;
      }

      FsckChunkList chunks(1, chunk);
      FsckChunkList failed;

      MsgHelperRepair::deleteChunks(node, &chunks, &failed);

      if(failed.empty() )
         this->database->getChunksTable()->remove(db::EntryID::fromStr(chunk.getID() ) );

      storageNodeStore->releaseNode(&node);
      break;
   }

   default:
      throw std::runtime_error("bad repair action");
   }
}

void ModeCheckFS::repairMissingContDir(FsckDirInode& inode, UserPrompter& prompt)
{
   FsckRepairAction action = prompt.chooseAction("Directory ID: " + inode.getID()
      + "; Path: "
      + this->database->getDentryTable()->getPathOf(db::EntryID::fromStr(inode.getID() ) ) );

   switch(action)
   {
   case FsckRepairAction_UNDEFINED:
   case FsckRepairAction_NOTHING:
      break;

   case FsckRepairAction_CREATECONTDIR: {
      NodeStore* metaNodeStore = Program::getApp()->getMetaNodes();
      Node* node = metaNodeStore->referenceNode(inode.getSaveNodeID() );

      FsckDirInodeList inodes(1, inode);
      StringList failed;

      MsgHelperRepair::createContDirs(node, &inodes, &failed);

      if(failed.empty() )
      {
         // create a list of cont dirs from dir inode
         FsckContDirList contDirs(1,
            FsckContDir(inode.getID(), inode.getSaveNodeID() ) );

         this->database->getContDirsTable()->insert(contDirs);
      }

      metaNodeStore->releaseNode(&node);
      break;
   }

   default:
      throw std::runtime_error("bad repair action");
   }
}

void ModeCheckFS::repairOrphanedContDir(FsckContDir& dir, UserPrompter& prompt)
{
   FsckRepairAction action = prompt.chooseAction("Directory ID: " + dir.getID() );

   switch(action)
   {
   case FsckRepairAction_UNDEFINED:
   case FsckRepairAction_NOTHING:
      break;

   case FsckRepairAction_CREATEDEFAULTDIRINODE: {
      NodeStore* metaNodeStore = Program::getApp()->getMetaNodes();
      Node* node = metaNodeStore->referenceNode(dir.getSaveNodeID() );

      FsckContDirList contDirs(1, dir);
      StringList inodeIDs(1, dir.getID() );

      FsckDirInodeList createdInodes;
      MsgHelperRepair::createDefDirInodes(node, &inodeIDs, &createdInodes);

      this->database->getDirInodesTable()->insert(createdInodes);

      metaNodeStore->releaseNode(&node);
      break;
   }

   default:
      throw std::runtime_error("bad repair action");
   }
}

void ModeCheckFS::repairWrongFileAttribs(std::pair<FsckFileInode, checks::InodeAttribs>& error,
   UserPrompter& prompt)
{
   FsckRepairAction action = prompt.chooseAction("File ID: " + error.first.getID() + "; Path: "
      + this->database->getDentryTable()->getPathOf(db::EntryID::fromStr(error.first.getID() ) ) );

   switch(action)
   {
   case FsckRepairAction_UNDEFINED:
   case FsckRepairAction_NOTHING:
      break;

   case FsckRepairAction_UPDATEATTRIBS: {
      NodeStore* metaNodeStore = Program::getApp()->getMetaNodes();
      Node* node = metaNodeStore->referenceNode(error.first.getSaveNodeID() );

      FsckFileInodeList inodes(1, error.first);
      FsckFileInodeList failed;

      // update the file attribs in the inode objects (even though they may not be used
      // on the server side, we need updated values here to update the DB
      error.first.setFileSize(error.second.size);
      error.first.setNumHardLinks(error.second.nlinks);

      MsgHelperRepair::updateFileAttribs(node, &inodes, &failed);

      if(failed.empty() )
         this->database->getFileInodesTable()->update(inodes);

      metaNodeStore->releaseNode(&node);
      break;
   }

   default:
      throw std::runtime_error("bad repair action");
   }
}

void ModeCheckFS::repairWrongDirAttribs(std::pair<FsckDirInode, checks::InodeAttribs>& error,
   UserPrompter& prompt)
{
   std::string filePath = this->database->getDentryTable()->getPathOf(
      db::EntryID::fromStr(error.first.getID() ) );
   FsckRepairAction action = prompt.chooseAction("Directory ID: " + error.first.getID()
      + "; Path: " + filePath);

   switch(action)
   {
   case FsckRepairAction_UNDEFINED:
   case FsckRepairAction_NOTHING:
      break;

   case FsckRepairAction_UPDATEATTRIBS: {
      NodeStore* metaNodeStore = Program::getApp()->getMetaNodes();
      Node* node = metaNodeStore->referenceNode(error.first.getSaveNodeID() );

      FsckDirInodeList inodes(1, error.first);
      FsckDirInodeList failed;

      // update the file attribs in the inode objects (even though they may not be used
      // on the server side, we need updated values here to update the DB
      error.first.setSize(error.second.size);
      error.first.setNumHardLinks(error.second.nlinks);

      MsgHelperRepair::updateDirAttribs(node, &inodes, &failed);

      if(failed.empty() )
         this->database->getDirInodesTable()->update(inodes);

      metaNodeStore->releaseNode(&node);
      break;
   }

   default:
      throw std::runtime_error("bad repair action");
   }
}

void ModeCheckFS::repairFileWithMissingTargets(db::DirEntry& entry, UserPrompter& prompt)
{
   FsckDirEntry fsckEntry = entry;

   FsckRepairAction action = prompt.chooseAction("Entry ID: " + fsckEntry.getID()
      + "; Path: " + this->database->getDentryTable()->getPathOf(entry) );

   switch(action)
   {
   case FsckRepairAction_UNDEFINED:
   case FsckRepairAction_NOTHING:
      return;

   case FsckRepairAction_DELETEFILE: {
      NodeStore* metaNodeStore = Program::getApp()->getMetaNodes();
      Node* node = metaNodeStore->referenceNode(fsckEntry.getSaveNodeID() );

      fsckEntry.setName(this->database->getDentryTable()->getNameOf(entry) );

      FsckDirEntryList dentries(1, fsckEntry);
      FsckDirEntryList failed;

      MsgHelperRepair::deleteFiles(node, &dentries, &failed);

      if(failed.empty() )
         this->deleteFilesFromDB(dentries);

      metaNodeStore->releaseNode(&node);
      break;
   }

   default:
      throw std::runtime_error("bad repair action");
   }
}

void ModeCheckFS::repairDirEntryWithBrokenByIDFile(db::DirEntry& entry, UserPrompter& prompt)
{
   FsckDirEntry fsckEntry = entry;

   FsckRepairAction action = prompt.chooseAction("Entry ID: " + fsckEntry.getID()
      + "; Path: " + this->database->getDentryTable()->getPathOf(entry) );

   switch(action)
   {
   case FsckRepairAction_UNDEFINED:
   case FsckRepairAction_NOTHING:
      break;

   case FsckRepairAction_RECREATEFSID: {
      NodeStore* metaNodeStore = Program::getApp()->getMetaNodes();
      Node* node = metaNodeStore->referenceNode(fsckEntry.getSaveNodeID() );

      fsckEntry.setName(this->database->getDentryTable()->getNameOf(entry) );

      FsckDirEntryList dentries(1, fsckEntry);
      FsckDirEntryList failed;

      MsgHelperRepair::recreateFsIDs(node, &dentries, &failed);

      if(failed.empty() )
      {
         // create a FsID list from the dentry
         FsckFsIDList idList(1,
            FsckFsID(fsckEntry.getID(), fsckEntry.getParentDirID(), fsckEntry.getSaveNodeID(),
               fsckEntry.getSaveDevice(), fsckEntry.getSaveInode() ) );

         this->database->getFsIDsTable()->insert(idList);
      }

      metaNodeStore->releaseNode(&node);
      break;
   }

   default:
      throw std::runtime_error("bad repair action");
   }
}

void ModeCheckFS::repairOrphanedDentryByIDFile(FsckFsID& id, UserPrompter& prompt)
{
   FsckRepairAction action = prompt.chooseAction("Entry ID: " + id.getID() + "; Path: "
      + this->database->getDentryTable()->getPathOf(db::EntryID::fromStr(id.getID() ) ) );

   switch(action)
   {
   case FsckRepairAction_UNDEFINED:
   case FsckRepairAction_NOTHING:
      break;

   case FsckRepairAction_RECREATEDENTRY: {
      NodeStore* metaNodeStore = Program::getApp()->getMetaNodes();
      Node* node = metaNodeStore->referenceNode(id.getSaveNodeID() );

      FsckFsIDList fsIDs(1, id);
      FsckFsIDList failed;
      FsckDirEntryList createdDentries;
      FsckFileInodeList createdInodes;

      MsgHelperRepair::recreateDentries(node, &fsIDs, &failed, &createdDentries, &createdInodes);

      if(failed.empty() )
      {
         this->database->getDentryTable()->insert(createdDentries);
         this->database->getFileInodesTable()->insert(createdInodes);
      }

      metaNodeStore->releaseNode(&node);
      break;
   }

   default:
      throw std::runtime_error("bad repair action");
   }
}

void ModeCheckFS::repairChunkWithWrongPermissions(std::pair<FsckChunk, FsckFileInode>& error,
   UserPrompter& prompt)
{
   FsckRepairAction action = prompt.chooseAction("Chunk ID: " + error.first.getID()
      + "; Target ID: " + StringTk::uintToStr(error.first.getTargetID()) + "; File path: "
      + this->database->getDentryTable()->getPathOf(db::EntryID::fromStr(error.first.getID() ) ) );

   switch(action)
   {
   case FsckRepairAction_UNDEFINED:
   case FsckRepairAction_NOTHING:
      break;

   case FsckRepairAction_FIXPERMISSIONS: {
      NodeStore* nodeStore = Program::getApp()->getStorageNodes();
      TargetMapper* targetMapper = Program::getApp()->getTargetMapper();

      // we will need the PathInfo later to send the SetAttr message and we don't have it in the
      // chunk
      PathInfoList pathInfoList(1, *error.second.getPathInfo() );

      // set correct permissions
      error.first.setUserID(error.second.getUserID() );
      error.first.setGroupID(error.second.getGroupID() );

      FsckChunkList chunkList(1, error.first);
      FsckChunkList failed;

      Node* storageNode = nodeStore->referenceNode(
         targetMapper->getNodeID(error.first.getTargetID() ) );
      MsgHelperRepair::fixChunkPermissions(storageNode, chunkList, pathInfoList, failed);
      nodeStore->releaseNode(&storageNode);

      if(failed.empty() )
         this->database->getChunksTable()->update(chunkList);

      break;
   }

   default:
      throw std::runtime_error("bad repair action");
   }
}

void ModeCheckFS::repairWrongChunkPath(std::pair<FsckChunk, FsckFileInode>& error,
   UserPrompter& prompt)
{
   if(error.first.getBuddyGroupID() )
   {
      std::string moveToPath = DatabaseTk::calculateExpectedChunkPath(error.first.getID(),
         error.second.getPathInfo()->getOrigUID(),
         error.second.getPathInfo()->getOrigParentEntryID(),
         error.second.getPathInfo()->getFlags() );

      FsckTkEx::fsckOutput("Chunk file " + error.first.getSavedPath()->getPathAsStr()
         + " on target " + StringTk::uintToStr(error.first.getTargetID() )
         + " belongs to a mirror group. The file was expected in " + moveToPath
         + ". Manual intervention is required to repair this error.", OutputOptions_LINEBREAK);
      return;
   }

   FsckRepairAction action = prompt.chooseAction("Entry ID: " + error.first.getID()
      + "; File path: "
      + this->database->getDentryTable()->getPathOf(db::EntryID::fromStr(error.first.getID() ) ) );

   switch(action)
   {
   case FsckRepairAction_UNDEFINED:
   case FsckRepairAction_NOTHING:
      break;

   case FsckRepairAction_MOVECHUNK: {
      NodeStore* nodeStore = Program::getApp()->getStorageNodes();
      TargetMapper* targetMapper = Program::getApp()->getTargetMapper();

      Node* storageNode = nodeStore->referenceNode(
         targetMapper->getNodeID(error.first.getTargetID() ) );

      std::string moveToPath = DatabaseTk::calculateExpectedChunkPath(error.first.getID(),
         error.second.getPathInfo()->getOrigUID(),
         error.second.getPathInfo()->getOrigParentEntryID(),
         error.second.getPathInfo()->getFlags() );

      if(MsgHelperRepair::moveChunk(storageNode, error.first, moveToPath, false) )
      {
         FsckChunkList chunks(1, error.first);
         this->database->getChunksTable()->update(chunks);
         break;
      }

      // chunk file exists at the correct location
      FsckTkEx::fsckOutput("Chunk file for " + error.first.getID()
         + " already exists at the correct location. ", OutputOptions_NONE);

      if(Program::getApp()->getConfig()->getAutomatic() )
      {
         FsckTkEx::fsckOutput("Will not attempt automatic repair.", OutputOptions_LINEBREAK);
         break;
      }

      char chosen = 0;

      while(chosen != 'y' && chosen != 'n')
      {
         FsckTkEx::fsckOutput("Move anyway? (y/n) ", OutputOptions_NONE);

         std::string line;
         std::getline(std::cin, line);

         if(line.size() == 1)
            chosen = line[0];
      }

      if(chosen != 'y')
         break;

      if(MsgHelperRepair::moveChunk(storageNode, error.first, moveToPath, true) )
      {
         FsckChunkList chunks(1, error.first);
         this->database->getChunksTable()->update(chunks);
      }
      else
      {
         FsckTkEx::fsckOutput("Repair failed, see log file for more details.",
            OutputOptions_LINEBREAK);
      }

      break;
   }

   default:
      throw std::runtime_error("bad repair action");
   }
}

void ModeCheckFS::deleteFsIDsFromDB(FsckDirEntryList& dentries)
{
   // create the fsID list
   FsckFsIDList fsIDs;

   for ( FsckDirEntryListIter iter = dentries.begin(); iter != dentries.end(); iter++ )
   {
      FsckFsID fsID(iter->getID(), iter->getParentDirID(), iter->getSaveNodeID(),
         iter->getSaveDevice(), iter->getSaveInode());
      fsIDs.push_back(fsID);
   }

   this->database->getFsIDsTable()->remove(fsIDs);
}

void ModeCheckFS::deleteFilesFromDB(FsckDirEntryList& dentries)
{
   for ( FsckDirEntryListIter dentryIter = dentries.begin(); dentryIter != dentries.end();
      dentryIter++ )
   {
      std::pair<bool, db::FileInode> inode = this->database->getFileInodesTable()->get(
         dentryIter->getID() );

      if(inode.first)
      {
         this->database->getFileInodesTable()->remove(inode.second.id);
         this->database->getChunksTable()->remove(inode.second.id);
      }
   }

   this->database->getDentryTable()->remove(dentries);

   // delete the dentry-by-id files
   this->deleteFsIDsFromDB(dentries);
}

bool ModeCheckFS::ensureLostAndFoundExists()
{
   this->lostAndFoundNode = MsgHelperRepair::referenceLostAndFoundOwner(&this->lostAndFoundInfo);

   if(!this->lostAndFoundNode)
   {
      if(!MsgHelperRepair::createLostAndFound(&this->lostAndFoundNode, this->lostAndFoundInfo) )
         return false;
   }

   std::pair<bool, FsckDirInode> dbLaF = this->database->getDirInodesTable()->get(
      this->lostAndFoundInfo.getEntryID() );

   if(dbLaF.first)
      this->lostAndFoundInode.reset(new FsckDirInode(dbLaF.second) );

   return true;
}

void ModeCheckFS::releaseLostAndFound()
{
   if(!this->lostAndFoundNode)
      return;

   if(this->lostAndFoundInode)
   {
      FsckDirInodeList updates(1, *this->lostAndFoundInode);
      this->database->getDirInodesTable()->update(updates);
   }

   NodeStore* metaNodeStore = Program::getApp()->getMetaNodes();

   metaNodeStore->releaseNode(&this->lostAndFoundNode);
   this->lostAndFoundInode.reset();
}
