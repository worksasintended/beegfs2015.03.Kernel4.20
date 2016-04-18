#include "ModeCheckFS.h"

#include <common/toolkit/ListTk.h>
#include <common/toolkit/UnitTk.h>
#include <components/DataFetcher.h>
#include <components/worker/RetrieveChunksWork.h>
#include <net/msghelpers/MsgHelperRepair.h>
#include <toolkit/FsckTkEx.h>

#include <program/Program.h>

#define PRINT_ERROR(text, askForAction) \
   if (askForAction) { FsckTkEx::fsckOutput(std::string("> ") + std::string(text),OutputOptions_LINEBREAK | OutputOptions_NOLOG); } \
   FsckTkEx::fsckOutput(std::string("> ") + std::string(text), OutputOptions_NOSTDOUT);

#define PRINT_ERROR_COUNT(count) \
   FsckTkEx::fsckOutput(">>> Found " + StringTk::int64ToStr(count) + " errors. " \
   "Detailed information can also be found in " + Program::getApp()->getConfig()->getLogOutFile() \
   + ".", OutputOptions_ADDLINEBREAKBEFORE | OutputOptions_DOUBLELINEBREAK);

#define PRINT_REPAIR_ACTION(action) \
   FsckTkEx::fsckOutput(" - [ra: " + FsckTkEx::getRepairActionDesc(action, true) + "]", \
   OutputOptions_LINEBREAK | OutputOptions_NOSTDOUT);


ModeCheckFS::ModeCheckFS()
{
   App* app = Program::getApp();
   this->database = app->getDatabase();

   this->log.setContext("ModeCheckFS");
}

ModeCheckFS::~ModeCheckFS()
{
}

void ModeCheckFS::printHelp()
{
   std::cout << "MODE ARGUMENTS:" << std::endl;
   std::cout << " Optional:" << std::endl;
   std::cout << "  --readOnly             Check only, skip repairs." << std::endl;
   std::cout << "  --runOnline            Run in online mode, which means user may access the"
      << std::endl;
   std::cout << "                         file system while the check is running." << std::endl;
   std::cout << "  --automatic            Do not prompt for repair actions and automatically use"
      << std::endl;
   std::cout << "                         reasonable default actions." << std::endl;
   std::cout << "  --noFetch              Do not build a new database from the servers and"
      << std::endl;
   std::cout << "                         instead work on an existing database (e.g. from a"
      << std::endl;
   std::cout << "                         previous read-only run)." << std::endl;
   std::cout << std::endl;
   std::cout << "  --quotaEnabled         Enable checks for quota support." << std::endl;
   std::cout << std::endl;
   std::cout << "  --databaseFile=<path>  Path to use for the database file. On systems with many"
      << std::endl;
   std::cout << "                         files, the database can grow to a size of several GB."
      << std::endl;
   std::cout << "                         (Default: " << CONFIG_DEFAULT_DBFILE << ")" << std::endl;
   std::cout << "  --overwriteDbFile      Overwrite an existing database file without prompt." << std::endl;
   std::cout << "  --ignoreDBDiskSpace    Ignore free disk space check for database file." << std::endl;
   std::cout << "  --logOutFile=<path>    Path to the fsck output file, which contains a copy of"
      << std::endl;
   std::cout << "                         the console output." << std::endl;
   std::cout << "                         (Default: " << CONFIG_DEFAULT_OUTFILE << ")" << std::endl;
   std::cout << "  --logStdFile=<path>    Path to the program error log file, which contains e.g."
      << std::endl;
   std::cout << "                         network error messages." << std::endl;
   std::cout << "                         (Default: " << CONFIG_DEFAULT_LOGFILE << ")" << std::endl;
   std::cout << std::endl;
   std::cout << "USAGE:" << std::endl;
   std::cout << " This mode performs a full check and optional repair of a BeeGFS file system"
      << std::endl;
   std::cout << " instance by building a database of the current file system contents on the"
      << std::endl;
   std::cout << " local machine." << std::endl;
   std::cout << std::endl;
   std::cout << " The fsck gathers information from all BeeGFS server daemons in parallel through"
      << std::endl;
   std::cout << " their configured network interfaces. All server components of the file"
      << std::endl;
   std::cout << " system have to be running to start a check." << std::endl;
   std::cout << std::endl;
   std::cout << " If the fsck is running without the \"--runonline\" argument, users may not"
      << std::endl;
   std::cout << " access the file system during a run (otherwise false errors might be reported)."
      << std::endl;
   std::cout << std::endl;
   std::cout << " Example: Check for errors, but skip repairs" << std::endl;
   std::cout << "  $ beegfs-fsck --checkfs --readonly" << std::endl;
}

int ModeCheckFS::execute()
{
   App* app = Program::getApp();
   Config *cfg = app->getConfig();

   if ( this->checkInvalidArgs(cfg->getUnknownConfigArgs()) )
      return APPCODE_INVALID_CONFIG;

   FsckTkEx::printVersionHeader(false);
   printHeaderInformation();

   if ( !FsckTkEx::checkReachability() )
      return APPCODE_COMMUNICATION_ERROR;

   if (cfg->getNoFetch())
   {
      // check if DB file exists
      bool dbExists = StorageTk::pathExists(cfg->getDatabaseFile());
      if ( !dbExists )
      {
         std::string errStr = "Given database file does not exist.";

         log.logErr(errStr);
         FsckTkEx::fsckOutput(errStr);

         return APPCODE_RUNTIME_ERROR;
      }
   }
   else
   {
      int initDBRes = initDatabase();
      if ( initDBRes )
         return initDBRes;

      if ( cfg->getRunOnline() )
      {
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
         Program::getApp()->getModificationEventHandler()->flush();

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

      bool processModEventsRes = this->database->processModificationEvents();

      if ( !processModEventsRes )
      {
         std::string errStr =
            "An error occured while processing modification events from servers. Fsck cannot "
            "proceed. Please see log file for more information";

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
   Path dbPath(cfg->getDatabaseFile());

   if ( !StorageTk::createPathOnDisk(dbPath, true) )
   {
      FsckTkEx::fsckOutput("Could not create path for database file: " + cfg->getDatabaseFile());
      return APPCODE_INITIALIZATION_ERROR;
   }

   // check disk space
   if ( !FsckTkEx::checkDiskSpace(dbPath) )
      return APPCODE_INITIALIZATION_ERROR;

   // check if DB file already exists
   if ( StorageTk::pathExists(cfg->getDatabaseFile()) && (!cfg->getOverwriteDbFile()) )
   {
      FsckTkEx::fsckOutput("The database file " + cfg->getDatabaseFile() + " already exists.");
      FsckTkEx::fsckOutput("If you continue now the existing file will be deleted.");

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
            break;
      }
   }

   this->database->init(!(cfg->getNoFetch()));

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
         + "]\nLog will be written to " + cfg->getLogStdFile() + "\nDatabase will be saved as "
         + cfg->getDatabaseFile(), OutputOptions_LINEBREAK | OutputOptions_HEADLINE);
}

bool ModeCheckFS::gatherData()
{
   bool retVal;

   FsckTkEx::fsckOutput("Step 2: Gather data from nodes: ", OutputOptions_DOUBLELINEBREAK);

   DataFetcher dataFetcher;
   retVal = dataFetcher.execute();

   FsckTkEx::fsckOutput("", OutputOptions_LINEBREAK);

   return retVal;
}

int64_t ModeCheckFS::checkAndRepairDanglingDentry()
{
   // FsckErrorCode_DANGLINGDENTRY
   FsckErrorCode errorCode = FsckErrorCode_DANGLINGDENTRY;

   // check for errors
   FsckTkEx::fsckOutput("* " + FsckTkEx::getErrorDesc(errorCode) + "... ", OutputOptions_FLUSH);

   if ( this->database->checkForAndInsertDanglingDirEntries() )
      FsckTkEx::fsckOutput("Finished", OutputOptions_LINEBREAK);
   else
      FsckTkEx::fsckOutput("Error", OutputOptions_LINEBREAK);

   // set repair action
   std::vector<FsckRepairAction> possibleActionsFileDentry;
   possibleActionsFileDentry.push_back(FsckRepairAction_NOTHING);
   possibleActionsFileDentry.push_back(FsckRepairAction_DELETEDENTRY);

   std::vector<FsckRepairAction> possibleActionsDirDentry;
   possibleActionsDirDentry.push_back(FsckRepairAction_NOTHING);
   possibleActionsDirDentry.push_back(FsckRepairAction_DELETEDENTRY);
   possibleActionsDirDentry.push_back(FsckRepairAction_CREATEDEFAULTDIRINODE);

   int64_t errorCount = this->database->countErrors(errorCode);

   if ( errorCount > 0 )
   {
      PRINT_ERROR_COUNT(errorCount);

      // first take only the dir entries representing a directory
      bool askForAction = true;
      FsckRepairAction repairAction = FsckRepairAction_UNDEFINED;

      if ( Program::getApp()->getConfig()->getReadOnly() )
      {
         askForAction = false;
      }
      else
      if ( Program::getApp()->getConfig()->getAutomatic() ) // repairmode is automatic
      {
         askForAction = false;
         repairAction = REPAIRACTION_DEF_DANGLINGDIRDENTRY;
      }

      // get only those with FsckRepairAction_UNDEFINED to prevent fsck from showing them in a
      // second run, if user chose to ignore them
      DBCursor<FsckDirEntry>* cursor = this->database->getDanglingDirectoryDirEntries(
         FsckRepairAction_UNDEFINED);

      FsckDirEntry* currentElement = cursor->open();

      while ( currentElement )
      {
         std::string filePath;
         DatabaseTk::getFullPath(this->database, currentElement, filePath);

         PRINT_ERROR(
            "Entry ID: " + currentElement->getID() + ";Entry Name: " + currentElement->getName()
            + "; Path: " + filePath, askForAction);

         if ( askForAction )
            repairAction = chooseAction(errorCode, possibleActionsDirDentry, askForAction);

         PRINT_REPAIR_ACTION(repairAction);

         // we need to reuse the already acquired handle; otherwise sqlite will deadlock
         // because the handle of the cursor already holds a lock
         if ( !this->database->setRepairAction(*currentElement, errorCode, repairAction,
            cursor->getHandle()) )
         {
            this->log.logErr(
               "Unable to set repair action for directory entry with ID: "
                  + currentElement->getID());
         }

         currentElement = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      // now ask for actions for dir entries representing something else than a directory
      askForAction = true;
      repairAction = FsckRepairAction_UNDEFINED;

      if ( Program::getApp()->getConfig()->getReadOnly() )
      {
         askForAction = false;
      }
      else
      if ( Program::getApp()->getConfig()->getAutomatic() ) // repairmode is automatic
      {
         askForAction = false;
         repairAction = REPAIRACTION_DEF_DANGLINGFILEDENTRY;
      }

      // get only those with FsckRepairAction_UNDEFINED to prevent fsck from showing them in a
      // second run, if user chose to ignore them
      cursor = this->database->getDanglingFileDirEntries(FsckRepairAction_UNDEFINED);

      currentElement = cursor->open();

      while ( currentElement )
      {
         std::string filePath;
         DatabaseTk::getFullPath(this->database, currentElement, filePath);

         PRINT_ERROR(
            "Entry ID: " + currentElement->getID() + ";Entry Name: " + currentElement->getName()
            + "; Path: " + filePath, askForAction);

         if ( askForAction )
            repairAction = chooseAction(errorCode, possibleActionsFileDentry, askForAction);

         PRINT_REPAIR_ACTION(repairAction);

         // we need to reuse the already acquired handle; otherwise sqlite will deadlock
         // because the handle of the cursor already holds a lock
         if ( !this->database->setRepairAction(*currentElement, errorCode, repairAction,
            cursor->getHandle()) )
         {
            this->log.logErr(
               "Unable to set repair action for directory entry with ID: "
                  + currentElement->getID());
         }

         currentElement = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if (! Program::getApp()->getConfig()->getReadOnly())
      {
         FsckTkEx::fsckOutput("");
         FsckTkEx::fsckOutput("Repairing now...");
      }

      // repair
      repairDanglingDirEntries();

      FsckTkEx::fsckOutput("");
   }
   return errorCount;
}

int64_t ModeCheckFS::checkAndRepairWrongInodeOwner()
{
   // FsckErrorCode_WRONGINODEOWNER
   FsckErrorCode errorCode = FsckErrorCode_WRONGINODEOWNER;

   // check
   FsckTkEx::fsckOutput("* " + FsckTkEx::getErrorDesc(errorCode) + "... ", OutputOptions_FLUSH);

   if ( this->database->checkForAndInsertInodesWithWrongOwner() )
      FsckTkEx::fsckOutput("Finished", OutputOptions_LINEBREAK);
   else
      FsckTkEx::fsckOutput("Error", OutputOptions_LINEBREAK);

   // set repair action
   std::vector<FsckRepairAction> possibleActions;
   possibleActions.push_back(FsckRepairAction_NOTHING);
   possibleActions.push_back(FsckRepairAction_CORRECTOWNER);

   int64_t errorCount = this->database->countErrors(errorCode);

   if ( errorCount > 0 )
   {
      bool askForAction = true;
      FsckRepairAction repairAction = FsckRepairAction_UNDEFINED;

      if ( Program::getApp()->getConfig()->getReadOnly() )
      {
         askForAction = false;
      }
      else
      if ( Program::getApp()->getConfig()->getAutomatic() ) // repairmode is automatic
      {
         askForAction = false;
         repairAction = REPAIRACTION_DEF_WRONGINODEOWNER;
      }

      PRINT_ERROR_COUNT(errorCount);

      // get only those with FsckRepairAction_UNDEFINED to prevent fsck from showing them in a
      // second run, if user chose to ignore them
      DBCursor<FsckDirInode>* cursor = this->database->getInodesWithWrongOwner(
         FsckRepairAction_UNDEFINED);

      FsckDirInode* currentElement = cursor->open();

      while ( currentElement )
      {
         std::string filePath;
         DatabaseTk::getFullPath(this->database, currentElement->getID(), filePath);

         PRINT_ERROR(
            "Entry ID: " + currentElement->getID() + "; Path: " + filePath, askForAction);

         if ( askForAction )
            repairAction = chooseAction(errorCode, possibleActions, askForAction);

         PRINT_REPAIR_ACTION(repairAction);

         // we need to reuse the already acquired handle; otherwise sqlite will deadlock
         // because the handle of the cursor already holds a lock
         if ( !this->database->setRepairAction(*currentElement, errorCode, repairAction,
            cursor->getHandle()) )
         {
            this->log.logErr(
               "Unable to set repair action for directory inode with ID: "
                  + currentElement->getID());
         }

         currentElement = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if (! Program::getApp()->getConfig()->getReadOnly())
      {
         FsckTkEx::fsckOutput("");
         FsckTkEx::fsckOutput("Repairing now...");
      }

      // repair
      repairWrongInodeOwners();

      FsckTkEx::fsckOutput("");
   }
   return errorCount;
}

int64_t ModeCheckFS::checkAndRepairWrongOwnerInDentry()
{
   //FsckErrorCode_WRONGOWNERINDENTRY
   FsckErrorCode errorCode = FsckErrorCode_WRONGOWNERINDENTRY;

   // check
   FsckTkEx::fsckOutput("* " + FsckTkEx::getErrorDesc(errorCode) + "... ", OutputOptions_FLUSH);

   if ( this->database->checkForAndInsertDirEntriesWithWrongOwner() )
      FsckTkEx::fsckOutput("Finished", OutputOptions_LINEBREAK);
   else
      FsckTkEx::fsckOutput("Error", OutputOptions_LINEBREAK);

   // set repair action
   std::vector<FsckRepairAction> possibleActions;
   possibleActions.push_back(FsckRepairAction_NOTHING);
   possibleActions.push_back(FsckRepairAction_CORRECTOWNER);

   int64_t errorCount = this->database->countErrors(errorCode);

   if ( errorCount > 0 )
   {
      bool askForAction = true;
      FsckRepairAction repairAction = FsckRepairAction_UNDEFINED;

      if ( Program::getApp()->getConfig()->getReadOnly() )
      {
         askForAction = false;
      }
      else
      if ( Program::getApp()->getConfig()->getAutomatic() ) // repairmode is automatic
      {
         askForAction = false;
         repairAction = REPAIRACTION_DEF_WRONGOWNERINDENTRY;
      }

      PRINT_ERROR_COUNT(errorCount);

      // get only those with FsckRepairAction_UNDEFINED to prevent fsck from showing them in a
      // second run, if user chose to ignore them
      DBCursor<FsckDirEntry>* cursor = this->database->getDirEntriesWithWrongOwner(
         FsckRepairAction_UNDEFINED);

      FsckDirEntry* currentElement = cursor->open();

      while ( currentElement )
      {
         std::string filePath;
         DatabaseTk::getFullPath(this->database, currentElement, filePath);

         PRINT_ERROR(
            "File ID: " + currentElement->getID() + ";File Name: " + currentElement->getName()
            + "; Path: " + filePath, askForAction);

         if ( askForAction )
            repairAction = chooseAction(errorCode, possibleActions, askForAction);

         PRINT_REPAIR_ACTION(repairAction);

         // we need to reuse the already acquired handle; otherwise sqlite will deadlock
         // because the handle of the cursor already holds a lock
         if ( !this->database->setRepairAction(*currentElement, errorCode, repairAction,
            cursor->getHandle()) )
         {
            this->log.logErr(
               "Unable to set repair action for directory entry with ID: "
                  + currentElement->getID());
         }

         currentElement = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if (! Program::getApp()->getConfig()->getReadOnly())
      {
         FsckTkEx::fsckOutput("");
         FsckTkEx::fsckOutput("Repairing now...");
      }

      // repair
      repairWrongInodeOwnersInDentry();

      FsckTkEx::fsckOutput("");
   }
   return errorCount;
}

int64_t ModeCheckFS::checkAndRepairOrphanedContDir()
{
   // FsckErrorCode_ORPHANEDCONTDIR
   FsckErrorCode errorCode = FsckErrorCode_ORPHANEDCONTDIR;

   // check
   FsckTkEx::fsckOutput("* " + FsckTkEx::getErrorDesc(errorCode) + "... ", OutputOptions_FLUSH);

   if ( this->database->checkForAndInsertOrphanedContDirs() )
      FsckTkEx::fsckOutput("Finished", OutputOptions_LINEBREAK);
   else
      FsckTkEx::fsckOutput("Error", OutputOptions_LINEBREAK);

   // set repair action
   std::vector<FsckRepairAction> possibleActions;
   possibleActions.push_back(FsckRepairAction_NOTHING);
   possibleActions.push_back(FsckRepairAction_CREATEDEFAULTDIRINODE);

   int64_t errorCount = this->database->countErrors(errorCode);

   if ( errorCount > 0 )
   {
      bool askForAction = true;
      FsckRepairAction repairAction = FsckRepairAction_UNDEFINED;

      if ( Program::getApp()->getConfig()->getReadOnly() )
      {
         askForAction = false;
      }
      else
      if ( Program::getApp()->getConfig()->getAutomatic() ) // repairmode is automatic
      {
         askForAction = false;
         repairAction = REPAIRACTION_DEF_ORPHANEDCONTDIR;
      }

      PRINT_ERROR_COUNT(errorCount);

      // get only those with FsckRepairAction_UNDEFINED to prevent fsck from showing them in a
      // second run, if user chose to ignore them
      DBCursor<FsckContDir>* cursor = this->database->getOrphanedContDirs(
         FsckRepairAction_UNDEFINED);

      FsckContDir* currentElement = cursor->open();

      while ( currentElement )
      {
         PRINT_ERROR("Directory ID: " + currentElement->getID(), askForAction);

         if ( askForAction )
            repairAction = chooseAction(errorCode, possibleActions, askForAction);

         PRINT_REPAIR_ACTION(repairAction);

         // we need to reuse the already aquired handle; otherwise sqlite will deadlock
         // because the handle of the cursor already holds a lock
         if ( !this->database->setRepairAction(*currentElement, errorCode, repairAction,
            cursor->getHandle()) )
         {
            this->log.logErr(
               "Unable to set repair action for content directory with ID: "
                  + currentElement->getID());
         }

         currentElement = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if (! Program::getApp()->getConfig()->getReadOnly())
      {
         FsckTkEx::fsckOutput("");
         FsckTkEx::fsckOutput("Repairing now...");
      }

      // repair
      repairOrphanedContDirs();

      FsckTkEx::fsckOutput("");
   }
   return errorCount;
}

int64_t ModeCheckFS::checkAndRepairOrphanedDirInode()
{
   // FsckErrorCode_ORPHANEDDIRINODE
   FsckErrorCode errorCode = FsckErrorCode_ORPHANEDDIRINODE;

   // check
   FsckTkEx::fsckOutput("* " + FsckTkEx::getErrorDesc(errorCode) + "... ", OutputOptions_FLUSH);

   if ( this->database->checkForAndInsertOrphanedDirInodes() )
      FsckTkEx::fsckOutput("Finished", OutputOptions_LINEBREAK);
   else
      FsckTkEx::fsckOutput("Error", OutputOptions_LINEBREAK);

   // set repair action
   std::vector<FsckRepairAction> possibleActions;
   possibleActions.push_back(FsckRepairAction_NOTHING);
   possibleActions.push_back(FsckRepairAction_LOSTANDFOUND);

   int64_t errorCount = this->database->countErrors(errorCode);

   if ( errorCount > 0 )
   {
      bool askForAction = true;
      FsckRepairAction repairAction = FsckRepairAction_UNDEFINED;

      if ( Program::getApp()->getConfig()->getReadOnly() )
      {
         askForAction = false;
      }
      else
      if ( Program::getApp()->getConfig()->getAutomatic() ) // repairmode is automatic
      {
         askForAction = false;
         repairAction = REPAIRACTION_DEF_ORPHANEDDIRINODE;
      }

      PRINT_ERROR_COUNT(errorCount);

      // get only those with FsckRepairAction_UNDEFINED to prevent fsck from showing them in a
      // second run, if user chose to ignore them
      DBCursor<FsckDirInode>* cursor = this->database->getOrphanedDirInodes(
         FsckRepairAction_UNDEFINED);

      FsckDirInode* currentElement = cursor->open();

      while ( currentElement )
      {
         PRINT_ERROR("Directory ID: " + currentElement->getID(), askForAction);

         if ( askForAction )
            repairAction = chooseAction(errorCode, possibleActions, askForAction);

         PRINT_REPAIR_ACTION(repairAction);

         // we need to reuse the already aquired handle; otherwise sqlite will deadlock
         // because the handle of the cursor already holds a lock
         if ( !this->database->setRepairAction(*currentElement, errorCode, repairAction,
            cursor->getHandle()) )
         {
            this->log.logErr(
               "Unable to set repair action for directory inode with ID: "
                  + currentElement->getID());
         }

         currentElement = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if (! Program::getApp()->getConfig()->getReadOnly())
      {
         FsckTkEx::fsckOutput("");
         FsckTkEx::fsckOutput("Repairing now...");
      }

      // repair
      repairOrphanedDirInodes();

      FsckTkEx::fsckOutput("");
   }
   return errorCount;
}

int64_t ModeCheckFS::checkAndRepairOrphanedFileInode()
{
   // FsckErrorCode_ORPHANEDFILEINODE
   FsckErrorCode errorCode = FsckErrorCode_ORPHANEDFILEINODE;

   // check
   FsckTkEx::fsckOutput("* " + FsckTkEx::getErrorDesc(errorCode) + "... ", OutputOptions_FLUSH);

   if ( this->database->checkForAndInsertOrphanedFileInodes() )
      FsckTkEx::fsckOutput("Finished", OutputOptions_LINEBREAK);
   else
      FsckTkEx::fsckOutput("Error", OutputOptions_LINEBREAK);

   // set repair action
   std::vector<FsckRepairAction> possibleActions;
   possibleActions.push_back(FsckRepairAction_NOTHING);
   possibleActions.push_back(FsckRepairAction_DELETEINODE);
//   possibleActions.push_back(FsckRepairAction_LOSTANDFOUND);

   int64_t errorCount = this->database->countErrors(errorCode);

   if ( errorCount > 0 )
   {
      bool askForAction = true;
      FsckRepairAction repairAction = FsckRepairAction_UNDEFINED;

      if ( Program::getApp()->getConfig()->getReadOnly() )
      {
         askForAction = false;
      }
      else
      if ( Program::getApp()->getConfig()->getAutomatic() ) // repairmode is automatic
      {
         askForAction = false;
         repairAction = REPAIRACTION_DEF_ORPHANEDFILEINODE;
      }

      PRINT_ERROR_COUNT(errorCount);

      // get only those with FsckRepairAction_UNDEFINED to prevent fsck from showing them in a
      // second run, if user chose to ignore them
      DBCursor<FsckFileInode>* cursor = this->database->getOrphanedFileInodes(
         FsckRepairAction_UNDEFINED);

      FsckFileInode* currentElement = cursor->open();

      while ( currentElement )
      {
         PRINT_ERROR("File ID: " + currentElement->getID(), askForAction);

         if ( askForAction )
            repairAction = chooseAction(errorCode, possibleActions, askForAction);

         PRINT_REPAIR_ACTION(repairAction);

         // we need to reuse the already aquired handle; otherwise sqlite will deadlock
         // because the handle of the cursor already holds a lock
         if ( !this->database->setRepairAction(*currentElement, errorCode, repairAction,
            cursor->getHandle()) )
         {
            this->log.logErr(
               "Unable to set repair action for file inode with ID: " + currentElement->getID());
         }

         currentElement = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if (! Program::getApp()->getConfig()->getReadOnly())
      {
         FsckTkEx::fsckOutput("");
         FsckTkEx::fsckOutput("Repairing now...");
      }

      // repair
      repairOrphanedFileInodes();

      FsckTkEx::fsckOutput("");
   }
   return errorCount;
}

int64_t ModeCheckFS::checkAndRepairOrphanedChunk()
{
   // FsckErrorCode_ORPHANEDCHUNK
   FsckErrorCode errorCode = FsckErrorCode_ORPHANEDCHUNK;

   // check
   FsckTkEx::fsckOutput("* " + FsckTkEx::getErrorDesc(errorCode) + "... ", OutputOptions_FLUSH);

   if ( this->database->checkForAndInsertOrphanedChunks() )
      FsckTkEx::fsckOutput("Finished", OutputOptions_LINEBREAK);
   else
      FsckTkEx::fsckOutput("Error", OutputOptions_LINEBREAK);

   // set repair action
   std::vector<FsckRepairAction> possibleActions;
   possibleActions.push_back(FsckRepairAction_NOTHING);
   possibleActions.push_back(FsckRepairAction_DELETECHUNK);

   int64_t errorCount = this->database->countErrors(errorCode);

   if ( errorCount > 0 )
   {
      bool askForAction = true;
      FsckRepairAction repairAction = FsckRepairAction_UNDEFINED;

      if ( Program::getApp()->getConfig()->getReadOnly() )
      {
         askForAction = false;
      }
      else
      if ( Program::getApp()->getConfig()->getAutomatic() ) // repairmode is automatic
      {
         askForAction = false;
         repairAction = REPAIRACTION_DEF_ORPHANEDCHUNK;
      }

      PRINT_ERROR_COUNT(errorCount);

      // get only those with FsckRepairAction_UNDEFINED to prevent fsck from showing them in a
      // second run, if user chose to ignore them
      DBCursor<FsckChunk>* cursor = this->database->getOrphanedChunks(FsckRepairAction_UNDEFINED,
         true);

      FsckChunk* currentElement = cursor->open();

      std::string lastID;
      FsckChunkList chunksToUpdate;

      // ask only once for each chunk
      while ( true )
      {
         // if
         // 1. we did not get a next element, OR
         // 2. if the ID of the current element is a new one, AND
         // 3. there is something set in chunks to update,
         // then set the repair action for the chunks

         if ( ((!currentElement) || (lastID.compare(currentElement->getID()) != 0))
            && (!chunksToUpdate.empty()) )
         {
            PRINT_ERROR(
               "Chunk ID: " + lastID + "; # of targets: "
               + StringTk::uintToStr(chunksToUpdate.size()), askForAction);

            if ( askForAction )
               repairAction = chooseAction(errorCode, possibleActions, askForAction);

            PRINT_REPAIR_ACTION(repairAction);

            for ( FsckChunkListIter iter = chunksToUpdate.begin(); iter != chunksToUpdate.end();
               iter++ )
            {
               // we need to reuse the already aquired handle; otherwise sqlite will deadlock
               // because the handle of the cursor already holds a lock
               if ( !this->database->setRepairAction(*iter, errorCode, repairAction,
                  cursor->getHandle()) )
               {
                  this->log.logErr(
                     "Unable to set repair action for file chunk. chunkID: " + iter->getID()
                        + " targetID: " + StringTk::uintToStr(iter->getTargetID()));
               }
            }
            chunksToUpdate.clear();
         }

         // cppcheck-suppress nullPointer [special comment to mute false cppcheck alarm]
         if ( !currentElement )
            break;

         lastID = currentElement->getID();
         chunksToUpdate.push_back(*currentElement);
         currentElement = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if (! Program::getApp()->getConfig()->getReadOnly())
      {
         FsckTkEx::fsckOutput("");
         FsckTkEx::fsckOutput("Repairing now...");
      }

      // repair
      repairOrphanedChunks();

      FsckTkEx::fsckOutput("");
   }

   return errorCount;
}

int64_t ModeCheckFS::checkAndRepairMissingContDir()
{
   // FsckErrorCode_MISSINGCONTDIR
   FsckErrorCode errorCode = FsckErrorCode_MISSINGCONTDIR;

   // check
   FsckTkEx::fsckOutput("* " + FsckTkEx::getErrorDesc(errorCode) + "... ", OutputOptions_FLUSH);

   if ( this->database->checkForAndInsertInodesWithoutContDir() )
      FsckTkEx::fsckOutput("Finished", OutputOptions_LINEBREAK);
   else
      FsckTkEx::fsckOutput("Error", OutputOptions_LINEBREAK);

   // set repair action
   std::vector<FsckRepairAction> possibleActions;
   possibleActions.push_back(FsckRepairAction_NOTHING);

   possibleActions.push_back(FsckRepairAction_CREATECONTDIR);

   int64_t errorCount = this->database->countErrors(errorCode);

   if ( errorCount > 0 )
   {
      bool askForAction = true;
      FsckRepairAction repairAction = FsckRepairAction_UNDEFINED;

      if ( Program::getApp()->getConfig()->getReadOnly() )
      {
         askForAction = false;
      }
      else
      if ( Program::getApp()->getConfig()->getAutomatic() ) // repairmode is automatic
      {
         askForAction = false;
         repairAction = REPAIRACTION_DEF_MISSINGCONTDIR;
      }

      PRINT_ERROR_COUNT(errorCount);

      // get only those with FsckRepairAction_UNDEFINED to prevent fsck from showing them in a
      // second run, if user chose to ignore them
      DBCursor<FsckDirInode>* cursor = this->database->getInodesWithoutContDir(
         FsckRepairAction_UNDEFINED);

      FsckDirInode* currentElement = cursor->open();

      while ( currentElement )
      {
         std::string filePath;
         DatabaseTk::getFullPath(this->database, currentElement->getID(), filePath);

         PRINT_ERROR(
            "Directory ID: " + currentElement->getID() + "; Path: " + filePath, askForAction);

         if ( askForAction )
            repairAction = chooseAction(errorCode, possibleActions, askForAction);

         PRINT_REPAIR_ACTION(repairAction);

         // we need to reuse the already aquired handle; otherwise sqlite will deadlock
         // because the handle of the cursor already holds a lock
         if ( !this->database->setRepairAction(*currentElement, errorCode, repairAction,
            cursor->getHandle()) )
         {
            this->log.logErr(
               "Unable to set repair action for file directory with ID: "
                  + currentElement->getID());
         }

         currentElement = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if (! Program::getApp()->getConfig()->getReadOnly())
      {
         FsckTkEx::fsckOutput("");
         FsckTkEx::fsckOutput("Repairing now...");
      }

      // repair
      repairMissingContDirs();

      FsckTkEx::fsckOutput("");
   }
   return errorCount;
}

int64_t ModeCheckFS::checkAndRepairWrongFileAttribs()
{
   // FsckErrorCode_WRONGFILEATTRIBS
   FsckErrorCode errorCode = FsckErrorCode_WRONGFILEATTRIBS;

   // check
   FsckTkEx::fsckOutput("* " + FsckTkEx::getErrorDesc(errorCode) + "... ", OutputOptions_FLUSH);

   if ( this->database->checkForAndInsertWrongInodeFileAttribs() )
      FsckTkEx::fsckOutput("Finished", OutputOptions_LINEBREAK);
   else
      FsckTkEx::fsckOutput("Error", OutputOptions_LINEBREAK);

   // set repair action
   std::vector<FsckRepairAction> possibleActions;
   possibleActions.push_back(FsckRepairAction_NOTHING);
   possibleActions.push_back(FsckRepairAction_UPDATEATTRIBS);

   int64_t errorCount = this->database->countErrors(errorCode);

   if ( errorCount > 0 )
   {
      bool askForAction = true;
      FsckRepairAction repairAction = FsckRepairAction_UNDEFINED;

      if ( Program::getApp()->getConfig()->getReadOnly() )
      {
         askForAction = false;
      }
      else
      if ( Program::getApp()->getConfig()->getAutomatic() ) // repairmode is automatic
      {
         askForAction = false;
         repairAction = REPAIRACTION_DEF_WRONGFILEATTRIBS;
      }

      PRINT_ERROR_COUNT(errorCount);

      // get only those with FsckRepairAction_UNDEFINED to prevent fsck from showing them in a
      // second run, if user chose to ignore them
      DBCursor<FsckFileInode>* cursor = this->database->getInodesWithWrongFileAttribs(
         FsckRepairAction_UNDEFINED);

      FsckFileInode* currentElement = cursor->open();

      while ( currentElement )
      {
         std::string filePath;
         DatabaseTk::getFullPath(this->database, currentElement->getID(), filePath);

         PRINT_ERROR(
            "File ID: " + currentElement->getID() + "; Path: " + filePath, askForAction);

         if ( askForAction )
            repairAction = chooseAction(errorCode, possibleActions, askForAction);

         PRINT_REPAIR_ACTION(repairAction);

         // we need to reuse the already aquired handle; otherwise sqlite will deadlock because
         // the handle of the cursor already holds a lock
         if ( !this->database->setRepairAction(*currentElement, errorCode, repairAction,
            cursor->getHandle()) )
         {
            this->log.logErr(
               "Unable to set repair action for file inode with ID: " + currentElement->getID());
         }

         currentElement = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if (! Program::getApp()->getConfig()->getReadOnly())
      {
         FsckTkEx::fsckOutput("");
         FsckTkEx::fsckOutput("Repairing now...");
      }

      // repair
      repairWrongFileAttribs();

      FsckTkEx::fsckOutput("");
   }
   return errorCount;
}

int64_t ModeCheckFS::checkAndRepairWrongDirAttribs()
{
   // FsckErrorCode_WRONGDIRATTRIBS
   FsckErrorCode errorCode = FsckErrorCode_WRONGDIRATTRIBS;

   // check
   FsckTkEx::fsckOutput("* " + FsckTkEx::getErrorDesc(errorCode) + "... ", OutputOptions_FLUSH);

   if ( this->database->checkForAndInsertWrongInodeDirAttribs() )
      FsckTkEx::fsckOutput("Finished", OutputOptions_LINEBREAK);
   else
      FsckTkEx::fsckOutput("Error", OutputOptions_LINEBREAK);

   // set repair action
   std::vector<FsckRepairAction> possibleActions;
   possibleActions.push_back(FsckRepairAction_NOTHING);
   possibleActions.push_back(FsckRepairAction_UPDATEATTRIBS);

   int64_t errorCount = this->database->countErrors(errorCode);

   if ( errorCount > 0 )
   {
      bool askForAction = true;
      FsckRepairAction repairAction = FsckRepairAction_UNDEFINED;

      if ( Program::getApp()->getConfig()->getReadOnly() )
      {
         askForAction = false;
      }
      else
      if ( Program::getApp()->getConfig()->getAutomatic() ) // repairmode is automatic
      {
         askForAction = false;
         repairAction = REPAIRACTION_DEF_WRONGDIRATTRIBS;
      }

      PRINT_ERROR_COUNT(errorCount);

      // get only those with FsckRepairAction_UNDEFINED to prevent fsck from showing them in a
      // second run, if user chose to ignore them
      DBCursor<FsckDirInode>* cursor = this->database->getInodesWithWrongDirAttribs(
         FsckRepairAction_UNDEFINED);

      FsckDirInode* currentElement = cursor->open();

      while ( currentElement )
      {
         std::string filePath;
         DatabaseTk::getFullPath(this->database, currentElement->getID(), filePath);

         PRINT_ERROR(
            "Directory ID: " + currentElement->getID() + "; Path: " + filePath, askForAction);

         if ( askForAction )
            repairAction = chooseAction(errorCode, possibleActions, askForAction);

         PRINT_REPAIR_ACTION(repairAction);

         // we need to reuse the already aquired handle; otherwise sqlite will deadlock because
         // the handle of the cursor already holds a lock
         if ( !this->database->setRepairAction(*currentElement, errorCode, repairAction,
            cursor->getHandle()) )
         {
            this->log.logErr(
               "Unable to set repair action for directory inode with ID: "
                  + currentElement->getID());
         }

         currentElement = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if (! Program::getApp()->getConfig()->getReadOnly())
      {
         FsckTkEx::fsckOutput("");
         FsckTkEx::fsckOutput("Repairing now...");
      }

      // repair
      repairWrongDirAttribs();

      FsckTkEx::fsckOutput("");
   }
   return errorCount;
}

int64_t ModeCheckFS::checkAndRepairMissingTargets()
{
   // FsckErrorCode_MISSINGTARGET
   FsckErrorCode errorCode = FsckErrorCode_MISSINGTARGET;

   // check
   FsckTkEx::fsckOutput("* " + FsckTkEx::getErrorDesc(errorCode) + "... ", OutputOptions_FLUSH);

   TargetMapper* targetMapper = Program::getApp()->getTargetMapper();
   MirrorBuddyGroupMapper* buddyGroupMapper = Program::getApp()->getMirrorBuddyGroupMapper();
   if ( this->database->checkForAndInsertMissingStripeTargets(targetMapper, buddyGroupMapper) )
      FsckTkEx::fsckOutput("Finished", OutputOptions_LINEBREAK);
   else
      FsckTkEx::fsckOutput("Error", OutputOptions_LINEBREAK);

   // set repair action
   std::vector<FsckRepairAction> possibleActions;
   possibleActions.push_back(FsckRepairAction_NOTHING);
   possibleActions.push_back(FsckRepairAction_CHANGETARGET);

   int64_t errorCount = this->database->countErrors(errorCode);

   if ( errorCount > 0 )
   {
      bool askForAction = true;
      FsckRepairAction repairAction = FsckRepairAction_UNDEFINED;

      if ( Program::getApp()->getConfig()->getReadOnly() )
      {
         askForAction = false;
      }
      else
      if ( Program::getApp()->getConfig()->getAutomatic() ) // repairmode is automatic
      {
         askForAction = false;
         repairAction = REPAIRACTION_DEF_MISSINGTARGET;
      }

      PRINT_ERROR_COUNT(errorCount);

      // get only those with FsckRepairAction_UNDEFINED to prevent fsck from showing them in a
      // second run, if user chose to ignore them
      DBCursor<FsckTargetID>* cursor = this->database->getMissingStripeTargets(
         FsckRepairAction_UNDEFINED);

      FsckTargetID* currentElement = cursor->open();

      while ( currentElement )
      {
         uint16_t id = currentElement->getID();
         FsckTargetIDType fsckTargetIDType = currentElement->getTargetIDType();

         if (fsckTargetIDType == FsckTargetIDType_BUDDYGROUP)
         {
            PRINT_ERROR(
               "Buddy Group ID: " + StringTk::uintToStr(id), askForAction);
         }
         else
         {
            PRINT_ERROR(
               "Target ID: " + StringTk::uintToStr(id), askForAction);
         }

         if ( askForAction )
            repairAction = chooseAction(errorCode, possibleActions, askForAction);

         PRINT_REPAIR_ACTION(repairAction);

         // we need to reuse the already aquired handle; otherwise sqlite will deadlock because
         // the handle of the cursor already holds a lock
         if ( !this->database->setRepairAction(*currentElement, errorCode, repairAction,
            cursor->getHandle()) )
         {
            this->log.logErr(
               "Unable to set repair action for target with ID: "
                  + StringTk::uintToStr(currentElement->getID()));
         }

         currentElement = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if (! Program::getApp()->getConfig()->getReadOnly())
      {
         FsckTkEx::fsckOutput("");
         FsckTkEx::fsckOutput("Repairing now...");
      }

      // repair
      repairMissingTargets();

      FsckTkEx::fsckOutput("");
   }

   return errorCount;
}

int64_t ModeCheckFS::checkAndRepairFilesWithMissingTargets()
{
   // FsckErrorCode_FILEWITHMISSINGTARGET
   FsckErrorCode errorCode = FsckErrorCode_FILEWITHMISSINGTARGET;

   // check for errors
   FsckTkEx::fsckOutput("* " + FsckTkEx::getErrorDesc(errorCode) + "... ", OutputOptions_FLUSH);

   if ( this->database->checkForAndInsertFilesWithMissingStripeTargets() )
      FsckTkEx::fsckOutput("Finished", OutputOptions_LINEBREAK);
   else
      FsckTkEx::fsckOutput("Error", OutputOptions_LINEBREAK);

   // set repair action
   std::vector<FsckRepairAction> possibleActions;
   possibleActions.push_back(FsckRepairAction_NOTHING);
   possibleActions.push_back(FsckRepairAction_DELETEFILE);

   int64_t errorCount = this->database->countErrors(errorCode);

   if ( errorCount > 0 )
   {
      PRINT_ERROR_COUNT(errorCount);

      // first take only the dir entries representing a directory
      bool askForAction = true;
      FsckRepairAction repairAction = FsckRepairAction_UNDEFINED;

      if ( Program::getApp()->getConfig()->getReadOnly() )
      {
         askForAction = false;
      }
      else
      if ( Program::getApp()->getConfig()->getAutomatic() ) // repairmode is automatic
      {
         askForAction = false;
         repairAction = REPAIRACTION_DEF_FILEWITHMISSINGTARGET;
      }

      // get only those with FsckRepairAction_UNDEFINED to prevent fsck from showing them in a
      // second run, if user chose to ignore them
      DBCursor<FsckDirEntry>* cursor = this->database->getFilesWithMissingStripeTargets(
         FsckRepairAction_UNDEFINED);

      FsckDirEntry* currentElement = cursor->open();

      while ( currentElement )
      {
         std::string filePath;
         DatabaseTk::getFullPath(this->database, currentElement, filePath);

         PRINT_ERROR(
            "Entry ID: " + currentElement->getID() + "; Entry Name: " + currentElement->getName()
            + "; Path: " + filePath, askForAction);

         if ( askForAction )
            repairAction = chooseAction(errorCode, possibleActions, askForAction);

         PRINT_REPAIR_ACTION(repairAction);

         // we need to reuse the already acquired handle; otherwise sqlite will deadlock
         // because the handle of the cursor already holds a lock
         if ( !this->database->setRepairAction(*currentElement, errorCode, repairAction,
            cursor->getHandle()) )
         {
            this->log.logErr(
               "Unable to set repair action for directory entry with ID: "
                  + currentElement->getID());
         }

         currentElement = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if (! Program::getApp()->getConfig()->getReadOnly())
      {
         FsckTkEx::fsckOutput("");
         FsckTkEx::fsckOutput("Repairing now...");
      }

      // repair
      repairFilesWithMissingTargets();

      FsckTkEx::fsckOutput("");
   }
   return errorCount;
}

int64_t ModeCheckFS::checkAndRepairDirEntriesWithBrokeByIDFile()
{
   // FsckErrorCode_BROKEFSID
   FsckErrorCode errorCode = FsckErrorCode_BROKENFSID;

   // check for errors
   FsckTkEx::fsckOutput("* " + FsckTkEx::getErrorDesc(errorCode) + "... ", OutputOptions_FLUSH);

   if ( this->database->checkForAndInsertDirEntriesWithBrokenByIDFile() )
      FsckTkEx::fsckOutput("Finished", OutputOptions_LINEBREAK);
   else
      FsckTkEx::fsckOutput("Error", OutputOptions_LINEBREAK);

   // set repair action
   std::vector<FsckRepairAction> possibleActions;
   possibleActions.push_back(FsckRepairAction_NOTHING);
   possibleActions.push_back(FsckRepairAction_RECREATEFSID);

   int64_t errorCount = this->database->countErrors(errorCode);

   if ( errorCount > 0 )
   {
      PRINT_ERROR_COUNT(errorCount);

      bool askForAction = true;
      FsckRepairAction repairAction = FsckRepairAction_UNDEFINED;

      if ( Program::getApp()->getConfig()->getReadOnly() )
      {
         askForAction = false;
      }
      else
      if ( Program::getApp()->getConfig()->getAutomatic() ) // repairmode is automatic
      {
         askForAction = false;
         repairAction = REPAIRACTION_DEF_BROKENFSID;
      }

      // get only those with FsckRepairAction_UNDEFINED to prevent fsck from showing them in a
      // second run, if user chose to ignore them
      DBCursor<FsckDirEntry>* cursor = this->database->getDirEntriesWithBrokenByIDFile(
         FsckRepairAction_UNDEFINED);

      FsckDirEntry* currentElement = cursor->open();

      while ( currentElement )
      {
         PRINT_ERROR(
            "Entry ID: " + currentElement->getID() + "; Entry Name: " + currentElement->getName(),
            askForAction);

         if ( askForAction )
            repairAction = chooseAction(errorCode, possibleActions, askForAction);

         PRINT_REPAIR_ACTION(repairAction);

         // we need to reuse the already acquired handle; otherwise sqlite will deadlock
         // because the handle of the cursor already holds a lock
         if ( !this->database->setRepairAction(*currentElement, errorCode, repairAction,
            cursor->getHandle()) )
         {
            this->log.logErr(
               "Unable to set repair action for directory entry with ID: "
                  + currentElement->getID());
         }

         currentElement = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if (! Program::getApp()->getConfig()->getReadOnly())
      {
         FsckTkEx::fsckOutput("");
         FsckTkEx::fsckOutput("Repairing now...");
      }

      // repair
      repairDirEntriesWithBrokenByIDFile();

      FsckTkEx::fsckOutput("");
   }

   return errorCount;
}

int64_t ModeCheckFS::checkAndRepairOrphanedDentryByIDFiles()
{
   // FsckErrorCode_ORPHANEDFSID
   FsckErrorCode errorCode = FsckErrorCode_ORPHANEDFSID;

   // check for errors
   FsckTkEx::fsckOutput("* " + FsckTkEx::getErrorDesc(errorCode) + "... ", OutputOptions_FLUSH);

   if ( this->database->checkForAndInsertOrphanedDentryByIDFiles() )
      FsckTkEx::fsckOutput("Finished", OutputOptions_LINEBREAK);
   else
      FsckTkEx::fsckOutput("Error", OutputOptions_LINEBREAK);

   // set repair action
   std::vector<FsckRepairAction> possibleActions;
   possibleActions.push_back(FsckRepairAction_NOTHING);
   possibleActions.push_back(FsckRepairAction_RECREATEDENTRY);

   int64_t errorCount = this->database->countErrors(errorCode);

   if ( errorCount > 0 )
   {
      PRINT_ERROR_COUNT(errorCount);

      bool askForAction = true;
      FsckRepairAction repairAction = FsckRepairAction_UNDEFINED;

      if ( Program::getApp()->getConfig()->getReadOnly() )
      {
         askForAction = false;
      }
      else
      if ( Program::getApp()->getConfig()->getAutomatic() ) // repairmode is automatic
      {
         askForAction = false;
         repairAction = REPAIRACTION_DEF_ORPHANEDFSID;
      }

      PRINT_REPAIR_ACTION(repairAction);

      // get only those with FsckRepairAction_UNDEFINED to prevent fsck from showing them in a
      // second run, if user chose to ignore them
      DBCursor<FsckFsID>* cursor = this->database->getOrphanedDentryByIDFiles(
         FsckRepairAction_UNDEFINED);

      FsckFsID* currentElement = cursor->open();

      while ( currentElement )
      {
         std::string filePath;
         DatabaseTk::getFullPath(this->database, currentElement->getID(), filePath);

         PRINT_ERROR("Entry ID: " + currentElement->getID() + "; Path: " + filePath, askForAction);

         if ( askForAction )
            repairAction = chooseAction(errorCode, possibleActions, askForAction);

         // we need to reuse the already acquired handle; otherwise sqlite will deadlock
         // because the handle of the cursor already holds a lock
         if ( !this->database->setRepairAction(*currentElement, errorCode, repairAction,
            cursor->getHandle()) )
         {
            this->log.logErr(
               "Unable to set repair action for dentry-by-ID file with ID: "
                  + currentElement->getID());
         }

         currentElement = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if (! Program::getApp()->getConfig()->getReadOnly())
      {
         FsckTkEx::fsckOutput("");
         FsckTkEx::fsckOutput("Repairing now...");
      }

      // repair
      repairOrphanedDentryByIDFiles();

      FsckTkEx::fsckOutput("");
   }

   return errorCount;
}

int64_t ModeCheckFS::checkAndRepairChunksWithWrongPermissions()
{
   // FsckErrorCode_CHUNKWITHWRONGPERM
   FsckErrorCode errorCode = FsckErrorCode_CHUNKWITHWRONGPERM;

   // check for errors
   FsckTkEx::fsckOutput("* " + FsckTkEx::getErrorDesc(errorCode) + "... ", OutputOptions_FLUSH);

   if ( this->database->checkForAndInsertChunksWithWrongPermissions() )
      FsckTkEx::fsckOutput("Finished", OutputOptions_LINEBREAK);
   else
      FsckTkEx::fsckOutput("Error", OutputOptions_LINEBREAK);

   // set repair action
   std::vector<FsckRepairAction> possibleActions;
   possibleActions.push_back(FsckRepairAction_NOTHING);
   possibleActions.push_back(FsckRepairAction_FIXPERMISSIONS);

   int64_t errorCount = this->database->countErrors(errorCode);

   if ( errorCount > 0 )
   {
      PRINT_ERROR_COUNT(errorCount);

      bool askForAction = true;
      FsckRepairAction repairAction = FsckRepairAction_UNDEFINED;

      if ( Program::getApp()->getConfig()->getReadOnly() )
      {
         askForAction = false;
      }
      else
      if ( Program::getApp()->getConfig()->getAutomatic() ) // repairmode is automatic
      {
         askForAction = false;
         repairAction = REPAIRACTION_DEF_CHUNKWITHWRONGPERM;
      }

      // get only those with FsckRepairAction_UNDEFINED to prevent fsck from showing them in a
      // second run, if user chose to ignore them
      DBCursor<FsckChunk>* cursor = this->database->getChunksWithWrongPermissions(
         FsckRepairAction_UNDEFINED);

      FsckChunk* currentElement = cursor->open();

      while ( currentElement )
      {
         std::string filePath;
         DatabaseTk::getFullPath(this->database, currentElement->getID(), filePath);

         PRINT_ERROR(
            "Chunk ID: " + currentElement->getID() + "; Target ID: "
            + StringTk::uintToStr(currentElement->getTargetID()) + "; File path: " + filePath,
            askForAction);

         if ( askForAction )
            repairAction = chooseAction(errorCode, possibleActions, askForAction);

         PRINT_REPAIR_ACTION(repairAction);

         // we need to reuse the already acquired handle; otherwise sqlite will deadlock
         // because the handle of the cursor already holds a lock
         if ( !this->database->setRepairAction(*currentElement, errorCode, repairAction,
            cursor->getHandle()) )
         {
            this->log.logErr(
               "Unable to set repair action for chunk with ID: " + currentElement->getID());
         }

         currentElement = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

      if (! Program::getApp()->getConfig()->getReadOnly())
      {
         FsckTkEx::fsckOutput("");
         FsckTkEx::fsckOutput("Repairing now...");
      }

      // repair
      repairChunksWithWrongPermissions();

      FsckTkEx::fsckOutput("");
   }

   return errorCount;
}

// no repair at the moment
int64_t ModeCheckFS::checkAndRepairChunksInWrongPath()
{
   // FsckErrorCode_CHUNKINWRONGPATH;

   FsckErrorCode errorCode = FsckErrorCode_CHUNKINWRONGPATH;

   // check for errors
   FsckTkEx::fsckOutput("* " + FsckTkEx::getErrorDesc(errorCode) + "... ", OutputOptions_FLUSH);

   if ( this->database->checkForAndInsertChunksInWrongPath() )
      FsckTkEx::fsckOutput("Finished", OutputOptions_LINEBREAK);
   else
      FsckTkEx::fsckOutput("Error", OutputOptions_LINEBREAK);

   // set repair action
   std::vector<FsckRepairAction> possibleActions;
   possibleActions.push_back(FsckRepairAction_NOTHING);
   possibleActions.push_back(FsckRepairAction_MOVECHUNK);

   int64_t errorCount = this->database->countErrors(errorCode);

   if ( errorCount > 0 )
   {
      PRINT_ERROR_COUNT(errorCount);

      bool askForAction = true;
      FsckRepairAction repairAction = FsckRepairAction_UNDEFINED;

      if ( Program::getApp()->getConfig()->getReadOnly() )
      {
         askForAction = false;
      }
      else
      if ( Program::getApp()->getConfig()->getAutomatic() ) // repairmode is automatic
      {
         askForAction = false;
         repairAction = REPAIRACTION_DEF_CHUNKINWRONGPATH;
      }

      // get only those with FsckRepairAction_UNDEFINED to prevent fsck from showing them in a
      // second run, if user chose to ignore them
      DBCursor<FsckChunk>* cursor = this->database->getChunksInWrongPath(
         FsckRepairAction_UNDEFINED);

      FsckChunk* currentElement = cursor->open();

      while ( currentElement )
      {
         std::string filePath;
         DatabaseTk::getFullPath(this->database, currentElement->getID(), filePath);

         PRINT_ERROR(
            "Entry ID: " + currentElement->getID() + "; File path: " + filePath, askForAction);

         if ( askForAction )
            repairAction = chooseAction(errorCode, possibleActions, askForAction);

         PRINT_REPAIR_ACTION(repairAction);

         // we need to reuse the already acquired handle; otherwise sqlite will deadlock
         // because the handle of the cursor already holds a lock
         if ( !this->database->setRepairAction(*currentElement, errorCode, repairAction,
            cursor->getHandle()) )
         {
            this->log.logErr(
               "Unable to set repair action for chunk with ID: " + currentElement->getID());
         }

         currentElement = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);

       if (! Program::getApp()->getConfig()->getReadOnly())
      {
         FsckTkEx::fsckOutput("");
         FsckTkEx::fsckOutput("Repairing now...");
      }

      // repair
      repairWrongChunkPaths();

      FsckTkEx::fsckOutput("");
   }

   return errorCount;
}

void ModeCheckFS::checkAndRepair()
{
   FsckTkEx::fsckOutput("Step 3: Check for errors... ", OutputOptions_DOUBLELINEBREAK);

   Config* cfg = Program::getApp()->getConfig();

   // we want to check more than once if errors in the DB were fixed, because this could lead to
   // new errros in the checks we already did before
   // BUT: to prevent endless loops (in case we missed something) we try a maximum of
   // MAX_CHECK_CYCLES
   int64_t errorCount;
   int maxRetries = MAX_CHECK_CYCLES;
   int retry = 0;

   do
   {
      retry++;
      errorCount = 0;
      // for each error code present the errors and repair

      checkAndRepairMissingTargets();

      errorCount += checkAndRepairFilesWithMissingTargets();
      errorCount += checkAndRepairOrphanedDentryByIDFiles();
      errorCount += checkAndRepairDirEntriesWithBrokeByIDFile();
      errorCount += checkAndRepairChunksInWrongPath();
      errorCount += checkAndRepairWrongInodeOwner();
      errorCount += checkAndRepairWrongOwnerInDentry();
      errorCount += checkAndRepairOrphanedContDir();
      errorCount += checkAndRepairOrphanedDirInode();
      errorCount += checkAndRepairOrphanedFileInode();
      errorCount += checkAndRepairOrphanedChunk();
      errorCount += checkAndRepairDanglingDentry();
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
         break;
      }

      if ( errorCount > 0 )
      {
         if ( retry < maxRetries )
         {
            FsckTkEx::fsckOutput(
               ">>> Found " + StringTk::int64ToStr(errorCount)
            + " errors; Re-running check phase <<< ",
            OutputOptions_ADDLINEBREAKBEFORE | OutputOptions_LINEBREAK);
         }
         else
         {
            FsckTkEx::fsckOutput(
               "Found " + StringTk::int64ToStr(errorCount)
                  + " errors, which could not be repaired now. We recommend running Fsck again.",
               OutputOptions_ADDLINEBREAKBEFORE);
         }
      }
   } while ( (errorCount > 0) && (retry < maxRetries) );
}

//////////////////////////
// internals      ///////
/////////////////////////
FsckRepairAction ModeCheckFS::chooseAction(FsckErrorCode errorCode,
   std::vector<FsckRepairAction> possibleActions, bool& outAskNextTime)
{
   while ( true )
   {
      for ( size_t i = 0; i < possibleActions.size(); i++ )
      {
         FsckTkEx::fsckOutput(
            "   " + StringTk::uintToStr(i + 1) + ") "
               + FsckTkEx::getRepairActionDesc(possibleActions[i]), OutputOptions_NOLOG |
               OutputOptions_LINEBREAK);
      }

      for ( size_t i = 0; i < possibleActions.size(); i++ )
      {
         FsckTkEx::fsckOutput(
            "   " + StringTk::uintToStr(i + possibleActions.size() + 1) + ") "
               + FsckTkEx::getRepairActionDesc(possibleActions[i]) + " (apply for all)",
               OutputOptions_NOLOG | OutputOptions_LINEBREAK);
      }

      char charInput;
      std::cin >> charInput;

      unsigned input = StringTk::strToUInt(std::string(&charInput, 1));

      if ( (input > 0) && (input <= possibleActions.size()) )
      {
         // user chose for this error
         return possibleActions[input - 1];
      }
      else
         if ( (input > possibleActions.size()) && (input <= possibleActions.size() * 2) )
         {
            // user chose for all errors => do not ask again
            outAskNextTime = false;
            FsckRepairAction repairAction = possibleActions[input - possibleActions.size() - 1];
            this->repairActions[errorCode] = repairAction;
            return repairAction;
         }
   }

   return FsckRepairAction_UNDEFINED;
}

void ModeCheckFS::repairDanglingDirEntries()
{
   {
      //if the user choose not to do anything ignore the entries for future iterations of fsck
      DBCursor<FsckDirEntry>* cursor = this->database->getDanglingDirEntries(
         FsckRepairAction_NOTHING);

      FsckDirEntry* currentEntry = cursor->open();

      while ( currentEntry )
      {
         this->database->addIgnoreErrorCode(*currentEntry, FsckErrorCode_DANGLINGDENTRY,
            cursor->getHandle());
         currentEntry = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);
   }

   // now take care of the other repair actions
   NodeStore* metaNodeStore = Program::getApp()->getMetaNodes();

   Node* currentNode = metaNodeStore->referenceFirstNode();

   // delete dentries if FsckRepairAction_DELETEDENTRY is set
   while ( currentNode )
   {
      uint16_t nodeID = currentNode->getNumID();

      DBCursor<FsckDirEntry>* cursor = this->database->getDanglingDirEntries(
         FsckRepairAction_DELETEDENTRY, nodeID);

      FsckDirEntryList dentries;

      FsckDirEntry* currentEntry = cursor->open();

      while ( currentEntry )
      {
         dentries.push_back(*currentEntry);

         currentEntry = cursor->step();

         // if the list exceeds MAX_DELETE_DENTRIES or if this was the last entry, send a message
         // to the metadata server and correct the database
         if ( (dentries.size() >= MAX_DELETE_DENTRIES) || (!currentEntry) )
         {
            FsckDirEntryList failedDeletes;
            MsgHelperRepair::deleteDanglingDirEntries(currentNode, &dentries, &failedDeletes);

            FsckTkEx::removeFromList(dentries, failedDeletes);

            this->deleteDanglingDirEntriesFromDB(dentries, cursor->getHandle());
            this->deleteDirEntriesFromDB(dentries, cursor->getHandle());
            this->deleteFsIDsFromDB(dentries, cursor->getHandle());
            dentries.clear();
         }
      }

      cursor->close();
      SAFE_DELETE(cursor);

      metaNodeStore->releaseNode(&currentNode);
      currentNode = metaNodeStore->referenceNextNode(nodeID);
   }

   currentNode = metaNodeStore->referenceFirstNode();

   // create default dir inode if FsckRepairAction_CREATEDEFAULTDIRINODE is set
   while ( currentNode )
   {
      uint16_t nodeID = currentNode->getNumID();

      DBCursor<FsckDirEntry>* cursor = this->database->getDanglingDirEntriesByInodeOwner(
         FsckRepairAction_CREATEDEFAULTDIRINODE, nodeID);

      StringList inodeIDs;
      FsckDirEntryList dentries;

      FsckDirEntry* currentEntry = cursor->open();

      while ( currentEntry )
      {
         inodeIDs.push_back(currentEntry->getID());
         dentries.push_back(*currentEntry);

         currentEntry = cursor->step();

         // if the list exceeds MAX_CREATE_DIR_INODES or if this was the last entry, send a message
         // to the metadata server and correct the database
         if ( (inodeIDs.size() >= MAX_CREATE_DIR_INODES) || (!currentEntry) )
         {
            StringList failedCreates;
            FsckDirInodeList createdInodes;
            MsgHelperRepair::createDefDirInodes(currentNode, &inodeIDs, &createdInodes);

            this->insertCreatedDirInodesToDB(createdInodes, cursor->getHandle());
            this->deleteDanglingDirEntriesFromDB(dentries, cursor->getHandle());

            inodeIDs.clear();
         }
      }

      cursor->close();
      SAFE_DELETE(cursor);

      metaNodeStore->releaseNode(&currentNode);
      currentNode = metaNodeStore->referenceNextNode(nodeID);
   }
}

void ModeCheckFS::repairWrongInodeOwners()
{
   {
      //if the user choose not to do anything ignore the entries for future iterations of fsck
      DBCursor<FsckDirInode>* cursor = this->database->getInodesWithWrongOwner(
         FsckRepairAction_NOTHING);

      FsckDirInode* currentEntry = cursor->open();

      while ( currentEntry )
      {
         this->database->addIgnoreErrorCode(*currentEntry, FsckErrorCode_WRONGINODEOWNER,
            cursor->getHandle());
         currentEntry = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);
   }

   // now take care of the other repair actions
   NodeStore* metaNodeStore = Program::getApp()->getMetaNodes();

   Node* currentNode = metaNodeStore->referenceFirstNode();

   // correct the owner if FsckRepairAction_CORRECTOWNER is set
   while ( currentNode )
   {
      uint16_t nodeID = currentNode->getNumID();

      FsckDirInodeList inodes;

      DBCursor<FsckDirInode>* cursor = this->database->getInodesWithWrongOwner(
         FsckRepairAction_CORRECTOWNER, nodeID);

      FsckDirInode* currentEntry = cursor->open();

      while ( currentEntry )
      {
         // update the inode to have the right owner
         currentEntry->setOwnerNodeID(currentEntry->getSaveNodeID());
         inodes.push_back(*currentEntry);

         currentEntry = cursor->step();

         // if the list exceeds MAX_CORRECT_OWNER or if this was the last entry, send a message
         // to the metadata server and correct the database
         if ( (inodes.size() >= MAX_CORRECT_OWNER) || (!currentEntry) )
         {
            FsckDirInodeList failedCorrections;

            MsgHelperRepair::correctInodeOwners(currentNode, &inodes, &failedCorrections);

            FsckTkEx::removeFromList(inodes, failedCorrections);

            this->correctAndDeleteWrongOwnerInodesFromDB(inodes, cursor->getHandle());
            inodes.clear();
         }
      }

      cursor->close();
      SAFE_DELETE(cursor);

      metaNodeStore->releaseNode(&currentNode);
      currentNode = metaNodeStore->referenceNextNode(nodeID);
   }
}

void ModeCheckFS::repairWrongInodeOwnersInDentry()
{
   {
      //if the user choose not to do anything ignore the entries for future iterations of fsck
      DBCursor<FsckDirEntry>* cursor = this->database->getDirEntriesWithWrongOwner(
         FsckRepairAction_NOTHING);

      FsckDirEntry* currentEntry = cursor->open();

      while ( currentEntry )
      {
         this->database->addIgnoreErrorCode(*currentEntry, FsckErrorCode_WRONGOWNERINDENTRY,
            cursor->getHandle());
         currentEntry = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);
   }

   // now take care of the other repair actions
   NodeStore* metaNodeStore = Program::getApp()->getMetaNodes();

   Node* currentNode = metaNodeStore->referenceFirstNode();

   // correct the owner if FsckRepairAction_CORRECTOWNER is set
   while ( currentNode )
   {
      uint16_t nodeID = currentNode->getNumID();

      DBCursor<FsckDirEntry>* cursor = this->database->getDirEntriesWithWrongOwner(
         FsckRepairAction_CORRECTOWNER, nodeID);

      FsckDirEntryList dentries;

      FsckDirEntry* currentEntry = cursor->open();

      while ( currentEntry )
      {
         dentries.push_back(*currentEntry);

         currentEntry = cursor->step();

         // if the list exceeds MAX_CORRECT_OWNER or if this was the last entry, send a message
         // to the metadata server and correct the database
         if ( (dentries.size() >= MAX_CORRECT_OWNER) || (!currentEntry) )
         {
            FsckDirEntryList failedCorrections;

            this->setCorrectInodeOwnersInDentry(dentries);

            MsgHelperRepair::correctInodeOwnersInDentry(currentNode, &dentries, &failedCorrections);

            FsckTkEx::removeFromList(dentries, failedCorrections);

            this->correctAndDeleteWrongOwnerEntriesFromDB(dentries, cursor->getHandle());
            dentries.clear();
         }
      }

      cursor->close();
      SAFE_DELETE(cursor);

      metaNodeStore->releaseNode(&currentNode);
      currentNode = metaNodeStore->referenceNextNode(nodeID);
   }
}

void ModeCheckFS::repairOrphanedDirInodes()
{
   {
      //if the user choose not to do anything ignore the entries for future iterations of fsck
      DBCursor<FsckDirInode>* cursor = this->database->getOrphanedDirInodes(
         FsckRepairAction_NOTHING);

      FsckDirInode* currentEntry = cursor->open();

      while ( currentEntry )
      {
         this->database->addIgnoreErrorCode(*currentEntry, FsckErrorCode_ORPHANEDDIRINODE,
            cursor->getHandle());
         currentEntry = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);
   }

   // now take care of the other repair actions
   NodeStore* metaNodeStore = Program::getApp()->getMetaNodes();

   // link to l+f if FsckRepairAction_LOSTANDFOUND is set

   // get the node, where the lost and found directory is located
   EntryInfo lostAndFoundInfo;
   Node* lostAndFoundNode = MsgHelperRepair::referenceLostAndFoundOwner(&lostAndFoundInfo);

   if ( !lostAndFoundNode )
   {
      // l+f doesn't seem to exist yet => create it
      if ( !MsgHelperRepair::createLostAndFound(&lostAndFoundNode, lostAndFoundInfo) )
      {
         log.logErr("Orphaned dir inodes could not be linked to lost+found, because lost+found "
            "directory could not be created");
         return;
      }
   }

   FsckDirInodeList inodes;

   DBCursor<FsckDirInode>* cursor = this->database->getOrphanedDirInodes(
      FsckRepairAction_LOSTANDFOUND);

   FsckDirInode* currentEntry = cursor->open();

   while ( currentEntry )
   {
      inodes.push_back(*currentEntry);

      currentEntry = cursor->step();

      // if the list exceeds MAX_LOST_AND_FOUND or if this was the last entry, send a message
      // to the metadata server and correct the database
      if ( (inodes.size() >= MAX_LOST_AND_FOUND) || (!currentEntry) )
      {
         FsckDirEntryList createdDentries;
         FsckDirInodeList failedInodes;

         MsgHelperRepair::linkToLostAndFound(lostAndFoundNode, &lostAndFoundInfo, &inodes,
            &failedInodes, &createdDentries);

         FsckTkEx::removeFromList(inodes, failedInodes);

         this->deleteOrphanedDirInodesFromDB(inodes, cursor->getHandle());
         this->insertCreatedDirEntriesToDB(createdDentries, cursor->getHandle());

         // on server side, each dentry also created a dentry-by-ID file
         FsckFsIDList createdFsIDs;
         for ( FsckDirEntryListIter iter = createdDentries.begin(); iter != createdDentries.end();
            iter++ )
         {
            FsckFsID fsID(iter->getID(), iter->getParentDirID(), iter->getSaveNodeID(),
               iter->getSaveDevice(), iter->getSaveInode());
            createdFsIDs.push_back(fsID);
         }

         this->insertCreatedFsIDsToDB(createdFsIDs, cursor->getHandle());

         std::string lostFoundEntryID = lostAndFoundInfo.getEntryID();

         // update lost+found dir inode in DB
         FsckDirInode* lostAndFoundInode = this->database->getDirInode(lostFoundEntryID);
         if ( likely(lostAndFoundInode) )
         {
            FsckDirInodeList updateList;
            updateList.push_back(*lostAndFoundInode);
            this->setUpdatedDirAttribs(updateList);
            this->updateDirInodesInDB(updateList, cursor->getHandle());

            SAFE_DELETE(lostAndFoundInode);
         }
         else
            log.logErr("Failed to get lostAndFoundInode: " + lostFoundEntryID);

         inodes.clear();
      }

   }

   cursor->close();
   SAFE_DELETE(cursor);

   metaNodeStore->releaseNode(&lostAndFoundNode);
}

void ModeCheckFS::repairOrphanedFileInodes()
{
   {
      //if the user choose not to do anything ignore the entries for future iterations of fsck
      DBCursor<FsckFileInode>* cursor = this->database->getOrphanedFileInodes(
         FsckRepairAction_NOTHING);

      FsckFileInode* currentEntry = cursor->open();

      while ( currentEntry )
      {
         this->database->addIgnoreErrorCode(*currentEntry, FsckErrorCode_ORPHANEDFILEINODE,
            cursor->getHandle());
         currentEntry = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);
   }

   // now take care of the other repair actions
   NodeStore* metaNodeStore = Program::getApp()->getMetaNodes();

   // delete the inode if FsckRepairAction_DELETEINODE is set
   // do this for each metadata node
   Node* currentNode = metaNodeStore->referenceFirstNode();

   while ( currentNode )
   {
      uint16_t nodeID = currentNode->getNumID();
      FsckFileInodeList inodes;

      DBCursor<FsckFileInode>* cursor = this->database->getOrphanedFileInodes(
         FsckRepairAction_DELETEINODE, nodeID);

      FsckFileInode* currentEntry = cursor->open();

      while ( currentEntry )
      {
         inodes.push_back(*currentEntry);

         currentEntry = cursor->step();

         // if the list exceeds MAX_DELETE_INODE or if this was the last entry, send a message
         // to the metadata server and correct the database
         if ( (inodes.size() >= MAX_DELETE_FILE_INODE) || (!currentEntry) )
         {
            StringList failedIDs;

            MsgHelperRepair::deleteFileInodes(currentNode, inodes, failedIDs);

            FsckTkEx::removeFromList(inodes, failedIDs);

            this->deleteOrphanedFileInodesFromDB(inodes, cursor->getHandle());
            this->deleteFileInodesFromDB(inodes, cursor->getHandle());
            inodes.clear();
         }
      }

      cursor->close();
      SAFE_DELETE(cursor);

      currentNode = metaNodeStore->referenceNextNode(nodeID);
   }



   // link to l+f if FsckRepairAction_LOSTANDFOUND is set

   // get the node, where the lost and found directory is located
 /*  EntryInfo lostAndFoundInfo;
   Node* lostAndFoundNode = MsgHelperRepair::referenceLostAndFoundOwner(&lostAndFoundInfo);

   if ( !lostAndFoundNode )
   {
      // l+f doesn't seem to exist yet => create it
      if ( !MsgHelperRepair::createLostAndFound(&lostAndFoundNode, lostAndFoundInfo) )
      {
         log.logErr("Orphaned file inodes could not be linked to lost+found, because lost+found "
            "directory could not be created");
         return;
      }
   }

   FsckFileInodeList inodes;

   DBCursor<FsckFileInode>* cursor = this->database->getOrphanedFileInodes(
      FsckRepairAction_LOSTANDFOUND);

   FsckFileInode* currentEntry = cursor->open();

   while ( currentEntry )
   {
      inodes.push_back(*currentEntry);

      currentEntry = cursor->step();

      // if the list exceeds MAX_LOST_AND_FOUND or if this was the last entry, send a message
      // to the metadata server and correct the database
      if ( (inodes.size() >= MAX_LOST_AND_FOUND) || (!currentEntry) )
      {
         FsckDirEntryList createdDentries;
         FsckFileInodeList failedInodes;

         MsgHelperRepair::linkToLostAndFound(lostAndFoundNode, &lostAndFoundInfo, &inodes,
            &failedInodes, &createdDentries);

         FsckTkEx::removeFromList(inodes, failedInodes);

         this->deleteOrphanedFileInodesFromDB(inodes, cursor->getHandle());
         this->insertCreatedDirEntriesToDB(createdDentries, cursor->getHandle());
         inodes.clear();
      }
   }

   cursor->close();
   SAFE_DELETE(cursor);

   metaNodeStore->releaseNode(&lostAndFoundNode); */
}

void ModeCheckFS::repairOrphanedChunks()
{
   {
      //if the user choose not to do anything ignore the entries for future iterations of fsck
      DBCursor<FsckChunk>* cursor = this->database->getOrphanedChunks(FsckRepairAction_NOTHING);

      FsckChunk* currentEntry = cursor->open();

      while ( currentEntry )
      {
         this->database->addIgnoreErrorCode(*currentEntry, FsckErrorCode_ORPHANEDCHUNK,
            cursor->getHandle());
         currentEntry = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);
   }

   // now take care of the other repair actions
   App* app = Program::getApp();
   NodeStore* storageNodeStore = app->getStorageNodes();

   Node* currentNode = storageNodeStore->referenceFirstNode();

   // delete the chunk if FsckRepairAction_DELETECHUNK is set
   while ( currentNode )
   {
      uint16_t nodeID = currentNode->getNumID();

      UInt16List targetIDs;
      app->getTargetMapper()->getTargetsByNode(nodeID, targetIDs);

      for ( UInt16ListIter targetIter = targetIDs.begin(); targetIter != targetIDs.end();
         targetIter++ )
      {

         DBCursor<FsckChunk>* cursor = this->database->getOrphanedChunks(
            FsckRepairAction_DELETECHUNK, *targetIter);

         FsckChunkList chunks;

         FsckChunk* currentEntry = cursor->open();

         while ( currentEntry )
         {
            chunks.push_back(*currentEntry);

            currentEntry = cursor->step();

            // if the list exceeds MAX_DELETE_CHUNK or if this was the last entry, send a message
            // to the metadata server and correct the database
            if ( (chunks.size() >= MAX_DELETE_CHUNK) || (!currentEntry) )
            {
               FsckChunkList failedDeletes;

               MsgHelperRepair::deleteChunks(currentNode, &chunks, &failedDeletes);

               FsckTkEx::removeFromList(chunks, failedDeletes);

               this->deleteOrphanedChunksFromDB(chunks, cursor->getHandle());
               this->deleteChunksFromDB(chunks, cursor->getHandle());
               chunks.clear();
            }
         }

         cursor->close();
         SAFE_DELETE(cursor);

      }

      storageNodeStore->releaseNode(&currentNode);
      currentNode = storageNodeStore->referenceNextNode(nodeID);
   }
}

void ModeCheckFS::repairMissingContDirs()
{
   {
      //if the user choose not to do anything ignore the entries for future iterations of fsck
      DBCursor<FsckDirInode>* cursor = this->database->getInodesWithoutContDir(
         FsckRepairAction_NOTHING);

      FsckDirInode* currentEntry = cursor->open();

      while ( currentEntry )
      {
         this->database->addIgnoreErrorCode(*currentEntry, FsckErrorCode_MISSINGCONTDIR,
            cursor->getHandle());
         currentEntry = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);
   }

   // now take care of the other repair actions
   NodeStore* metaNodeStore = Program::getApp()->getMetaNodes();

   Node* node = metaNodeStore->referenceFirstNode();

   while ( node )
   {
      uint16_t nodeID = node->getNumID();

      { // FsckRepairAction_CREATECONTDIR
        // create an empty content directory if FsckRepairAction_CREATECONTDIR is set
         FsckDirInodeList inodes;

         DBCursor<FsckDirInode>* cursor = this->database->getInodesWithoutContDir(
            FsckRepairAction_CREATECONTDIR, nodeID);

         FsckDirInode* currentEntry = cursor->open();

         while ( currentEntry )
         {
            inodes.push_back(*currentEntry);

            currentEntry = cursor->step();

            // if the list exceeds MAX_CREATE_CONT_DIR or if this was the last entry, send a message
            // to the metadata server and correct the database
            if ( (inodes.size() >= MAX_CREATE_CONT_DIR) || (!currentEntry) )
            {
               StringList failedCreates;

               MsgHelperRepair::createContDirs(node, &inodes, &failedCreates);

               FsckTkEx::removeFromList(inodes, failedCreates);

               this->insertCreatedContDirsToDB(inodes, cursor->getHandle());
               this->deleteMissingContDirsFromDB(inodes, cursor->getHandle());

               inodes.clear();
            }
         }

         cursor->close();
         SAFE_DELETE(cursor);
      }

      metaNodeStore->releaseNode(&node);
      node = metaNodeStore->referenceNextNode(nodeID);
   }
}

void ModeCheckFS::repairOrphanedContDirs()
{
   {
      //if the user choose not to do anything ignore the entries for future iterations of fsck
      DBCursor<FsckContDir>* cursor = this->database->getOrphanedContDirs(FsckRepairAction_NOTHING);

      FsckContDir* currentEntry = cursor->open();

      while ( currentEntry )
      {
         this->database->addIgnoreErrorCode(*currentEntry, FsckErrorCode_ORPHANEDCONTDIR,
            cursor->getHandle());
         currentEntry = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);
   }

   // now take care of the other repair actions
   App* app = Program::getApp();
   NodeStore* metaNodeStore = app->getMetaNodes();

   Node* node = metaNodeStore->referenceFirstNode();

   while ( node )
   {
      uint16_t nodeID = node->getNumID();

      // create default dir inode if FsckRepairAction_CREATEDEFAULTDIRINODE is set
      DBCursor<FsckContDir>* cursor = this->database->getOrphanedContDirs(
         FsckRepairAction_CREATEDEFAULTDIRINODE, nodeID);

      FsckContDirList contDirs;
      StringList inodeIDs;

      FsckContDir* currentEntry = cursor->open();

      while ( currentEntry )
      {
         contDirs.push_back(*currentEntry);
         inodeIDs.push_back(currentEntry->getID());

         currentEntry = cursor->step();

         // if the list exceeds MAX_CREATE_DIR_INODES or if this was the last entry, send a
         // message to the metadata server and correct the database
         if ( (contDirs.size() >= MAX_CREATE_DIR_INODES) || (!currentEntry) )
         {
            StringList failedCreates;
            FsckDirInodeList createdInodes;
            MsgHelperRepair::createDefDirInodes(node, &inodeIDs, &createdInodes);

            this->insertCreatedDirInodesToDB(createdInodes, cursor->getHandle());
            this->deleteOrphanedContDirsFromDB(contDirs, cursor->getHandle());

            contDirs.clear();
         }
      }

      cursor->close();
      SAFE_DELETE(cursor);

      metaNodeStore->releaseNode(&node);
      node = metaNodeStore->referenceNextNode(nodeID);
   }
}

void ModeCheckFS::repairWrongFileAttribs()
{
   {
      //if the user choose not to do anything ignore the entries for future iterations of fsck
      DBCursor<FsckFileInode>* cursor = this->database->getInodesWithWrongFileAttribs(
         FsckRepairAction_NOTHING);

      FsckFileInode* currentEntry = cursor->open();

      while ( currentEntry )
      {
         this->database->addIgnoreErrorCode(*currentEntry, FsckErrorCode_WRONGFILEATTRIBS,
            cursor->getHandle());
         currentEntry = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);
   }

   // now take care of the other repair actions
   App* app = Program::getApp();
   NodeStore* metaNodeStore = app->getMetaNodes();

   Node* node = metaNodeStore->referenceFirstNode();

   while ( node )
   {
      uint16_t nodeID = node->getNumID();

      // update attribs if FsckRepairAction_UPDATEATTRIBS is set
      DBCursor<FsckFileInode>* cursor = this->database->getInodesWithWrongFileAttribs(
         FsckRepairAction_UPDATEATTRIBS, nodeID);

      FsckFileInodeList inodes;

      FsckFileInode* currentEntry = cursor->open();

      while ( currentEntry )
      {
         inodes.push_back(*currentEntry);

         currentEntry = cursor->step();

         // if the list exceeds MAX_UPDATE_FILE_ATTRIBS or if this was the last entry, send a
         // message to the metadata server and correct the database
         if ( (inodes.size() >= MAX_UPDATE_FILE_ATTRIBS) || (!currentEntry) )
         {
            FsckFileInodeList failedUpdates;

            // update the file attribs in the inode objects (even though they may not be used
            // on the server side, we need updated values here to update the DB
            this->setUpdatedFileAttribs(inodes);

            MsgHelperRepair::updateFileAttribs(node, &inodes, &failedUpdates);

            FsckTkEx::removeFromList(inodes, failedUpdates);

            this->updateFileInodesInDB(inodes, cursor->getHandle());
            this->deleteFileInodesWithWrongAttribsFromDB(inodes, cursor->getHandle());

            inodes.clear();
         }
      }

      cursor->close();
      SAFE_DELETE(cursor);

      metaNodeStore->releaseNode(&node);
      node = metaNodeStore->referenceNextNode(nodeID);
   }
}

void ModeCheckFS::repairWrongDirAttribs()
{
   {
      //if the user choose not to do anything ignore the entries for future iterations of fsck
      DBCursor<FsckDirInode>* cursor = this->database->getInodesWithWrongDirAttribs(
         FsckRepairAction_NOTHING);

      FsckDirInode* currentEntry = cursor->open();

      while ( currentEntry )
      {
         this->database->addIgnoreErrorCode(*currentEntry, FsckErrorCode_WRONGDIRATTRIBS,
            cursor->getHandle());
         currentEntry = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);
   }

   // now take care of the other repair actions
   App* app = Program::getApp();
   NodeStore* metaNodeStore = app->getMetaNodes();

   Node* node = metaNodeStore->referenceFirstNode();

   while ( node )
   {
      uint16_t nodeID = node->getNumID();

      // update attribs if FsckRepairAction_UPDATEATTRIBS is set
      DBCursor<FsckDirInode>* cursor = this->database->getInodesWithWrongDirAttribs(
         FsckRepairAction_UPDATEATTRIBS, nodeID);

      FsckDirInodeList inodes;

      FsckDirInode* currentEntry = cursor->open();

      while ( currentEntry )
      {
         inodes.push_back(*currentEntry);

         currentEntry = cursor->step();

         // if the list exceeds MAX_UPDATE_FILE_ATTRIBS or if this was the last entry, send a
         // message to the metadata server and correct the database
         if ( (inodes.size() >= MAX_UPDATE_FILE_ATTRIBS) || (!currentEntry) )
         {
            FsckDirInodeList failedUpdates;

            // update the file attribs in the inode objects (even though they may not be used
            // on the server side, we need updated values here to update the DB
            this->setUpdatedDirAttribs(inodes);

            MsgHelperRepair::updateDirAttribs(node, &inodes, &failedUpdates);

            FsckTkEx::removeFromList(inodes, failedUpdates);

            this->updateDirInodesInDB(inodes, cursor->getHandle());
            this->deleteDirInodesWithWrongAttribsFromDB(inodes, cursor->getHandle());

            inodes.clear();
         }
      }

      cursor->close();
      SAFE_DELETE(cursor);

      metaNodeStore->releaseNode(&node);
      node = metaNodeStore->referenceNextNode(nodeID);
   }
}

void ModeCheckFS::repairMissingTargets()
{
   {
      //if the user choose not to do anything ignore the entries for future iterations of fsck
      DBCursor<FsckTargetID>* cursor = this->database->getMissingStripeTargets(
         FsckRepairAction_NOTHING);

      FsckTargetID* currentEntry = cursor->open();

      while ( currentEntry )
      {
         this->database->addIgnoreErrorCode(*currentEntry, FsckErrorCode_MISSINGTARGET,
            cursor->getHandle());
         currentEntry = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);
   }

   // now take care of the other repair actions
   // change the target IDs if FsckRepairAction_CHANGETARGET is set

   // get a list of existing target IDs
   App* app = Program::getApp();
   TargetMapper* targetMapper = app->getTargetMapper();
   UInt16List existingTargets;
   UInt16List mappedNodes;
   targetMapper->getMappingAsLists(existingTargets, mappedNodes);

   // get a list of existing buddy group IDs
   MirrorBuddyGroupMapper* buddyGroupMapper = app->getMirrorBuddyGroupMapper();
   UInt16List existingBuddyGroups;
   MirrorBuddyGroupList mappedGroups;
   buddyGroupMapper->getMappingAsLists(existingBuddyGroups, mappedGroups);

   // put together maps, from which the user can later choose the targets and buddy groups
   std::map<unsigned, uint16_t> possibleTargetChoices;
   int index = 1;
   for ( UInt16ListIter existingTargetsIter = existingTargets.begin();
      existingTargetsIter != existingTargets.end(); existingTargetsIter++, index++ )
   {
      possibleTargetChoices[index] = *existingTargetsIter;
   }

   std::map<unsigned, uint16_t> possibleBuddyGroupChoices;
   index = 1;
   for ( UInt16ListIter existingBuddyGroupsIter = existingBuddyGroups.begin();
      existingBuddyGroupsIter != existingBuddyGroups.end(); existingBuddyGroupsIter++, index++ )
   {
      possibleBuddyGroupChoices[index] = *existingBuddyGroupsIter;
   }

   // get all targets with this repair action set
   FsckTargetIDList missingTargetIDs;
   this->database->getMissingStripeTargets(&missingTargetIDs, FsckRepairAction_CHANGETARGET);

   // for each target, ask the user for an "exchange target"
   for ( FsckTargetIDListIter targetIDIter = missingTargetIDs.begin();
      targetIDIter != missingTargetIDs.end(); targetIDIter++ )
   {
      uint16_t id = targetIDIter->getID();
      FsckTargetIDType  fsckTargetIDType = targetIDIter->getTargetIDType();

      std::map<unsigned, uint16_t>* possibleChoicesPtr;
      if (fsckTargetIDType == FsckTargetIDType_BUDDYGROUP)
      {
         FsckTkEx::fsckOutput("Change buddy group ID: " + StringTk::uintToStr(id));
         FsckTkEx::fsckOutput("Please choose from the following existing buddy group IDs :");

         possibleChoicesPtr = &possibleBuddyGroupChoices;
      }
      else
      {
         FsckTkEx::fsckOutput("Change target ID: " + StringTk::uintToStr(id));
         FsckTkEx::fsckOutput("Please choose from the following existing target IDs :");

         possibleChoicesPtr = &possibleTargetChoices;
      }

      unsigned input = '0';

      while ( possibleChoicesPtr->find(input) == possibleChoicesPtr->end() )
      {
         for ( std::map<unsigned, uint16_t>::iterator iter = possibleChoicesPtr->begin();
            iter != possibleChoicesPtr->end(); iter++, index++ )
         {
            FsckTkEx::fsckOutput(
               StringTk::uintToStr(iter->first) + ") " + StringTk::uintToStr(iter->second));
         }

         char charInput;
         std::cin >> charInput;

         input = StringTk::strToUInt(std::string(&charInput, 1));
      }

      uint16_t newTargetID = possibleChoicesPtr->at(input);

      // do the actual work; for each MDS, get all inodes
      NodeStoreServers* metaNodes = app->getMetaNodes();
      Node* node = metaNodes->referenceFirstNode();

      while ( node )
      {
         uint16_t nodeID = node->getNumID();

         DBCursor<FsckFileInode>* inodeCursor = this->database->getFileInodesWithTarget(nodeID,
            *targetIDIter);

         FsckFileInode* currentInode = inodeCursor->open();

         FsckFileInodeList inodes;
         while ( currentInode )
         {
            inodes.push_back(*currentInode);
            currentInode = inodeCursor->step();

            // if MAX_CHANGE_STRIPE_TARGETS is reached send the message to MDS (because we do not
            // want to exhaust message limits)
            if ( (inodes.size() == MAX_CHANGE_STRIPE_TARGET) || (!currentInode) )
            {
               FsckFileInodeList failedInodes;
               MsgHelperRepair::changeStripeTarget(node, &inodes, targetIDIter->getID(),
                  newTargetID, &failedInodes);
               // change the values in DB
               FsckTkEx::removeFromList(inodes, failedInodes);
               this->database->replaceStripeTarget(targetIDIter->getID(), newTargetID, &inodes,
                  inodeCursor->getHandle());

               // clear the inodes for next loop
               inodes.clear();
            }
         }

         // TODO: do the same with dirInodes
         inodeCursor->close();
         SAFE_DELETE(inodeCursor);

         metaNodes->releaseNode(&node);
         node = metaNodes->referenceNextNode(nodeID);
      }
   }
}

void ModeCheckFS::repairFilesWithMissingTargets()
{
   {
      //if the user choose not to do anything ignore the entries for future iterations of fsck
      DBCursor<FsckDirEntry>* cursor = this->database->getFilesWithMissingStripeTargets(
         FsckRepairAction_NOTHING);

      FsckDirEntry* currentEntry = cursor->open();

      while ( currentEntry )
      {
         this->database->addIgnoreErrorCode(*currentEntry, FsckErrorCode_FILEWITHMISSINGTARGET,
            cursor->getHandle());
         currentEntry = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);
   }

   // now take care of the other repair actions
   NodeStore* metaNodeStore = Program::getApp()->getMetaNodes();

   Node* currentNode = metaNodeStore->referenceFirstNode();

   // delete the file if FsckRepairAction_CORRECTOWNER is set
   while ( currentNode )
   {
      uint16_t nodeID = currentNode->getNumID();

      DBCursor<FsckDirEntry>* cursor = this->database->getFilesWithMissingStripeTargets(
         FsckRepairAction_DELETEFILE, nodeID);

      FsckDirEntryList dentries;

      FsckDirEntry* currentEntry = cursor->open();

      while ( currentEntry )
      {
         dentries.push_back(*currentEntry);

         currentEntry = cursor->step();

         // if the list exceeds MAX_DELETE_FILES or if this was the last entry, send a message
         // to the metadata server and correct the database
         if ( (dentries.size() >= MAX_DELETE_FILES) || (!currentEntry) )
         {
            FsckDirEntryList failedDeletes;

            MsgHelperRepair::deleteFiles(currentNode, &dentries, &failedDeletes);

            FsckTkEx::removeFromList(dentries, failedDeletes);

            this->deleteFilesWithMissingTargetFromDB(dentries, cursor->getHandle());
            this->deleteFilesFromDB(dentries, cursor->getHandle());
            dentries.clear();
         }
      }

      cursor->close();
      SAFE_DELETE(cursor);

      metaNodeStore->releaseNode(&currentNode);
      currentNode = metaNodeStore->referenceNextNode(nodeID);
   }
}

void ModeCheckFS::repairDirEntriesWithBrokenByIDFile()
{
   {
      //if the user choose not to do anything ignore the entries for future iterations of fsck
      DBCursor<FsckDirEntry>* cursor = this->database->getDirEntriesWithBrokenByIDFile(
         FsckRepairAction_NOTHING);

      FsckDirEntry* currentEntry = cursor->open();

      while ( currentEntry )
      {
         this->database->addIgnoreErrorCode(*currentEntry, FsckErrorCode_BROKENFSID,
            cursor->getHandle());
         currentEntry = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);
   }

   // now take care of the other repair actions
   NodeStore* metaNodeStore = Program::getApp()->getMetaNodes();

   Node* currentNode = metaNodeStore->referenceFirstNode();

   // recreate the file if FsckRepairAction_RECREATEFSID is set
   while ( currentNode )
   {
      uint16_t nodeID = currentNode->getNumID();

      DBCursor<FsckDirEntry>* cursor = this->database->getDirEntriesWithBrokenByIDFile(
         FsckRepairAction_RECREATEFSID, nodeID);

      FsckDirEntryList dentries;

      FsckDirEntry* currentEntry = cursor->open();

      while ( currentEntry )
      {
         dentries.push_back(*currentEntry);

         currentEntry = cursor->step();

         // if the list exceeds MAX_RECREATEFSIDS or if this was the last entry, send a message
         // to the metadata server and correct the database
         if ( (dentries.size() >= MAX_RECREATEFSIDS) || (!currentEntry) )
         {
            FsckDirEntryList failedEntries;

            MsgHelperRepair::recreateFsIDs(currentNode, &dentries, &failedEntries);

            FsckTkEx::removeFromList(dentries, failedEntries);

            this->deleteDirEntriesWithBrokenByIDFileFromDB(dentries, cursor->getHandle());
            this->updateFsIDsInDB(dentries, cursor->getHandle());
            dentries.clear();
         }
      }

      cursor->close();
      SAFE_DELETE(cursor);

      metaNodeStore->releaseNode(&currentNode);
      currentNode = metaNodeStore->referenceNextNode(nodeID);
   }
}

void ModeCheckFS::repairOrphanedDentryByIDFiles()
{
   {
      //if the user choose not to do anything ignore the entries for future iterations of fsck
      DBCursor<FsckFsID>* cursor = this->database->getOrphanedDentryByIDFiles(
         FsckRepairAction_NOTHING);

      FsckFsID* currentEntry = cursor->open();

      while ( currentEntry )
      {
         this->database->addIgnoreErrorCode(*currentEntry, FsckErrorCode_ORPHANEDFSID,
            cursor->getHandle());
         currentEntry = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);
   }

   // now take care of the other repair actions
   NodeStore* metaNodeStore = Program::getApp()->getMetaNodes();

   Node* currentNode = metaNodeStore->referenceFirstNode();

   // FsckRepairAction_RECREATEDENTRY
   while ( currentNode )
   {
      uint16_t nodeID = currentNode->getNumID();

      DBCursor<FsckFsID>* cursor = this->database->getOrphanedDentryByIDFiles(
         FsckRepairAction_RECREATEDENTRY, nodeID);

      FsckFsIDList fsIDs;

      FsckFsID* currentEntry = cursor->open();

      while ( currentEntry )
      {
         fsIDs.push_back(*currentEntry);

         currentEntry = cursor->step();

         // if the list exceeds MAX_RECREATEDENTRIES or if this was the last entry, send a message
         // to the metadata server and correct the database
         if ( (fsIDs.size() >= MAX_RECREATEDENTRIES) || (!currentEntry) )
         {
            FsckFsIDList failedEntries;
            FsckDirEntryList createdDentries;
            FsckFileInodeList createdInodes;

            MsgHelperRepair::recreateDentries(currentNode, &fsIDs, &failedEntries, &createdDentries,
               &createdInodes);

            ListTk::removeFromList(fsIDs, failedEntries);

            this->deleteOrphanedDentryByIDFilesFromDB(fsIDs, cursor->getHandle());
            this->insertCreatedDirEntriesToDB(createdDentries, cursor->getHandle());
            this->insertCreatedFileInodesToDB(createdInodes, cursor->getHandle());
            fsIDs.clear();
         }
      }

      cursor->close();
      SAFE_DELETE(cursor);

      metaNodeStore->releaseNode(&currentNode);
      currentNode = metaNodeStore->referenceNextNode(nodeID);
   }
}

void ModeCheckFS::repairChunksWithWrongPermissions()
{
   {
      //if the user choose not to do anything ignore the entries for future iterations of fsck
      DBCursor<FsckChunk>* cursor = this->database->getChunksWithWrongPermissions(
         FsckRepairAction_NOTHING);

      FsckChunk* currentChunk = cursor->open();

      while ( currentChunk )
      {
         this->database->addIgnoreErrorCode(*currentChunk, FsckErrorCode_CHUNKWITHWRONGPERM,
            cursor->getHandle());
         currentChunk = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);
   }

   // now take care of the other repair actions
   NodeStore* nodeStore = Program::getApp()->getStorageNodes();
   TargetMapper* targetMapper = Program::getApp()->getTargetMapper();
   UInt16List targetIDs;
   UInt16List nodeIDs;

   targetMapper->getMappingAsLists(targetIDs, nodeIDs);

   // FsckRepairAction_FIXPERMISSIONS
   UInt16ListIter targetIDsIter = targetIDs.begin();
   UInt16ListIter nodeIDsIter = nodeIDs.begin();
   for ( ; targetIDsIter != targetIDs.end(); targetIDsIter++, nodeIDsIter++ )
   {
      uint16_t nodeID = *nodeIDsIter;
      uint16_t targetID = *targetIDsIter;

      DBCursor<FsckChunk>* cursor = this->database->getChunksWithWrongPermissions(
         FsckRepairAction_FIXPERMISSIONS, targetID);

      FsckChunkList chunkList;
      // we will need the PathInfo later to send the SetAttr message and we don't have it in the
      // chunk
      PathInfoList pathInfoList;

      FsckChunk* currentChunk = cursor->open();

      while ( currentChunk )
      {
         FsckFileInode* inode = this->database->getFileInode(currentChunk->getID());

         // set correct permissions
         if ( inode )
         {
            currentChunk->setUserID(inode->getUserID());
            currentChunk->setGroupID(inode->getGroupID());

            chunkList.push_back(*currentChunk);

            // we will need the PathInfo later to send the SetAttr message
            pathInfoList.push_back(*(inode->getPathInfo()));

            SAFE_DELETE(inode);
         }
         else
         {
            log.logErr(
               "Cannot fix chunk permissions. Unable to find inode for chunk. chunkID: "
                  + currentChunk->getID());
         }

         currentChunk = cursor->step();

         // if the list exceeds MAX_FIXPERMISSIONS or if this was the last entry, send a message
         // to the metadata server and correct the database
         if ( (chunkList.size() >= MAX_FIXPERMISSIONS) || (!currentChunk) )
         {
            FsckChunkList failedChunks;

            Node* storageNode = nodeStore->referenceNode(nodeID);
            MsgHelperRepair::fixChunkPermissions(storageNode, chunkList, pathInfoList,
               failedChunks);
            nodeStore->releaseNode(&storageNode);

            ListTk::removeFromList(chunkList, failedChunks);

            this->deleteChunksWithWrongPermissionsFromDB(chunkList, cursor->getHandle());
            this->updateChunksInDB(chunkList, cursor->getHandle());
            chunkList.clear();
            pathInfoList.clear();
         }
      }

      cursor->close();
      SAFE_DELETE(cursor);
   }
}

void ModeCheckFS::repairWrongChunkPaths()
{
   {
      //if the user choose not to do anything ignore the entries for future iterations of fsck
      DBCursor<FsckChunk>* cursor = this->database->getChunksInWrongPath(
         FsckRepairAction_NOTHING);

      FsckChunk* currentChunk = cursor->open();

      while ( currentChunk )
      {
         this->database->addIgnoreErrorCode(*currentChunk, FsckErrorCode_CHUNKINWRONGPATH,
            cursor->getHandle());
         currentChunk = cursor->step();
      }

      cursor->close();
      SAFE_DELETE(cursor);
   }

   // now take care of the other repair actions
   NodeStore* nodeStore = Program::getApp()->getStorageNodes();
   TargetMapper* targetMapper = Program::getApp()->getTargetMapper();
   UInt16List targetIDs;
   UInt16List nodeIDs;

   targetMapper->getMappingAsLists(targetIDs, nodeIDs);

   // FsckRepairAction_MOVECHUNK
   UInt16ListIter targetIDsIter = targetIDs.begin();
   UInt16ListIter nodeIDsIter = nodeIDs.begin();
   for ( ; targetIDsIter != targetIDs.end(); targetIDsIter++, nodeIDsIter++ )
   {
      uint16_t nodeID = *nodeIDsIter;
      uint16_t targetID = *targetIDsIter;

      DBCursor<FsckChunk>* cursor = this->database->getChunksInWrongPath(
         FsckRepairAction_MOVECHUNK, targetID);

      FsckChunkList chunkList;
      StringList moveToList;

      FsckChunk* currentChunk = cursor->open();

      while ( currentChunk )
      {
         FsckFileInode* inode = this->database->getFileInode(currentChunk->getID());

         if ( inode )
         {
            std::string moveToPath = DatabaseTk::calculateExpectedChunkPath(currentChunk->getID(),
               inode->getPathInfo()->getOrigUID(), inode->getPathInfo()->getOrigParentEntryID(),
               inode->getPathInfo()->getFlags());
            moveToList.push_back(moveToPath);

            chunkList.push_back(*currentChunk);

            SAFE_DELETE(inode);
         }
         else
         {
            log.logErr(
               "Cannot move chunk. Unable to find inode for chunk. chunkID: "
                  + currentChunk->getID());
         }

         currentChunk = cursor->step();

         // if the list exceeds MAX_MOVECHUNKS or if this was the last entry, send a message
         // to the metadata server and correct the database
         if ( (chunkList.size() >= MAX_MOVECHUNKS) || (!currentChunk) )
         {
            FsckChunkList failedChunks;

            Node* storageNode = nodeStore->referenceNode(nodeID);
            // NOTE: chunkList gets modified inside! (set correct path)
            MsgHelperRepair::moveChunks(storageNode, chunkList, moveToList, failedChunks);
            nodeStore->releaseNode(&storageNode);

            ListTk::removeFromList(chunkList, failedChunks);

            this->deleteChunksInWrongPathFromDB(chunkList, cursor->getHandle());
            this->updateChunksInDB(chunkList, cursor->getHandle());
            chunkList.clear();
            moveToList.clear();
         }
      }

      cursor->close();
      SAFE_DELETE(cursor);
   }
}

void ModeCheckFS::setCorrectInodeOwnersInDentry(FsckDirEntryList& dentries)
{
   for ( FsckDirEntryListIter dentryIter = dentries.begin(); dentryIter != dentries.end();
      dentryIter++ )
   {
      std::string entryID = dentryIter->getID();
      std::string name = dentryIter->getName();
      std::string parentID = dentryIter->getParentDirID();
      uint16_t entryOwner = dentryIter->getEntryOwnerNodeID();
      uint16_t inodeOwner = dentryIter->getInodeOwnerNodeID();
      FsckDirEntryType entryType = dentryIter->getEntryType();
      bool hasInlinedInode = dentryIter->getHasInlinedInode();
      uint16_t saveNodeID = dentryIter->getSaveNodeID();
      int saveDevice = dentryIter->getSaveDevice();
      uint64_t saveInode = dentryIter->getSaveInode();

      // try to get the inode owner
      if ( FsckDirEntryType_ISDIR(entryType))
      {
         FsckDirInode* dirInode = this->database->getDirInode(entryID);

         if ( dirInode )
         {
            inodeOwner = dirInode->getSaveNodeID();
            SAFE_DELETE(dirInode);
         }
         else
            log.log(Log_WARNING,
               "Unable to find inode owner for directory entry; EntryID: " + entryID);
      }
      else
      {
         FsckFileInode* fileInode = this->database->getFileInode(entryID);

         if ( fileInode )
         {
            inodeOwner = fileInode->getSaveNodeID();
            SAFE_DELETE(fileInode);
         }
         else
            log.log(Log_WARNING,
               "Unable to find inode owner for directory entry; EntryID: " + entryID);
      }

      dentryIter->update(entryID, name, parentID, entryOwner, inodeOwner, entryType,
         hasInlinedInode, saveNodeID, saveDevice, saveInode);
   }
}

void ModeCheckFS::correctAndDeleteWrongOwnerInodesFromDB(FsckDirInodeList& inodes,
   DBHandle* dbHandle)
{
   // first correct the inodes in the normal database, correct owner must already be set in
   // inodes!
   this->updateDirInodesInDB(inodes, dbHandle);

   // now delete the errors
   FsckDirInode failedDelete;
   int errorCode;
   bool delRes = this->database->deleteInodesWithWrongOwner(inodes, failedDelete, errorCode,
      dbHandle);
   if ( !delRes )
   {
      log.log(1,
         "Failed to delete directory inode with wrong owner; entryID: " + failedDelete.getID()
            + "; sqlite error code: " + StringTk::intToStr(errorCode));
      /*
       * NOTE: we do not throw an exception here, because fsck run is still correct.
       * An error here will just lead to a faulty error count displayed to the user
       */
   }
}

void ModeCheckFS::correctAndDeleteWrongOwnerEntriesFromDB(FsckDirEntryList& dentries,
   DBHandle* dbHandle)
{
   {
      // first correct the dentries in the normal database, correct owner must already be set in
      // dentries!

      FsckDirEntry failedUpdate;
      int errorCode;
      bool updateRes = this->database->updateDirEntries(dentries, failedUpdate, errorCode,
         dbHandle);

      if ( !updateRes )
      {
         log.log(1,
            "Failed to update dirEntry; name: " + failedUpdate.getName() + "; parentID: "
               + failedUpdate.getParentDirID() + "; sqlite error code: "
               + StringTk::intToStr(errorCode));
         throw FsckDBException(
            "Error while inserting dir entries to database. Please see log for more information.");
      }
   }

   {
      // now delete the errors
      FsckDirEntry failedDelete;
      int errorCode;
      bool delRes = this->database->deleteDirEntriesWithWrongOwner(dentries, failedDelete,
         errorCode, dbHandle);
      if ( !delRes )
      {
         log.log(1,
            "Failed to delete directory entry with wrong owner; name: " + failedDelete.getName()
               + "; parentID: " + failedDelete.getParentDirID() + "; sqlite error code: "
               + StringTk::intToStr(errorCode));
         /*
          * NOTE: we do not throw an exception here, because fsck run is still correct.
          * An error here will just lead to a faulty error count displayed to the user
          */
      }
   }
}

void ModeCheckFS::setUpdatedFileAttribs(FsckFileInodeList& inodes)
{
   for ( FsckFileInodeListIter inodeIter = inodes.begin(); inodeIter != inodes.end(); inodeIter++ )
   {
      { // update fileSize
         int64_t fileSize = 0;

         DBCursor<FsckChunk>* cursor = this->database->getChunks(inodeIter->getID(), true);
         FsckChunk* currentChunk = cursor->open();

         // set filesize
         // TODO: this does not work for sparse files!
         while ( currentChunk )
         {
            fileSize += currentChunk->getFileSize();
            currentChunk = cursor->step();
         }

         cursor->close();
         SAFE_DELETE(cursor);

         inodeIter->setFileSize(fileSize);

      }

      { // set number of hardlinks
         DBCursor<FsckDirEntry>* cursor = this->database->getDirEntries(inodeIter->getID());
         unsigned numLinks = 0;

         FsckDirEntry* currentEntry = cursor->open();

         while ( currentEntry )
         {
            numLinks++;
            currentEntry = cursor->step();
         }

         inodeIter->setNumHardLinks(numLinks);

         cursor->close();
         SAFE_DELETE(cursor);
      }
   }
}

void ModeCheckFS::setUpdatedDirAttribs(FsckDirInodeList& inodes)
{
   for ( FsckDirInodeListIter inodeIter = inodes.begin(); inodeIter != inodes.end(); inodeIter++ )
   {
      { // update size (i.e. subentry count) + number of links (do this in one step, because
        // otherwise 2 iterations over nearly the same result set were needed
         DBCursor<FsckDirEntry>* cursor = this->database->getDirEntriesByParentID(
            inodeIter->getID());
         int64_t size = 0;
         unsigned numLinks = 2; // num links is at least 2

         FsckDirEntry* currentEntry = cursor->open();

         while ( currentEntry )
         {
            size++;

            // if entry is a directory, also increment numLinks
            if ( FsckDirEntryType_ISDIR(currentEntry->getEntryType()))
               numLinks++;

            currentEntry = cursor->step();
         }

         inodeIter->setNumHardLinks(numLinks);
         inodeIter->setSize(size);

         cursor->close();
         SAFE_DELETE(cursor);
      }

      { // set number of hardlinks

      }
   }
}

void ModeCheckFS::deleteDirEntriesFromDB(FsckDirEntryList& dentries, DBHandle* dbHandle)
{
   FsckDirEntry failedDelete;
   int errorCode;
   bool delRes = this->database->deleteDirEntries(dentries, failedDelete, errorCode, dbHandle);
   if ( !delRes )
   {
      log.log(1,
         "Failed to delete directory entry; name: " + failedDelete.getName() + "; parentID: "
            + failedDelete.getParentDirID() + "; sqlite error code: "
            + StringTk::intToStr(errorCode));
      throw FsckDBException(
         "Error while delete directory entries from database. Please see log for more "
            "information.");
   }
}

void ModeCheckFS::deleteFsIDsFromDB(FsckDirEntryList& dentries, DBHandle* dbHandle)
{
   FsckFsID failedDelete;
   int errorCode;

   // create the fsID list
   FsckFsIDList fsIDs;

   for ( FsckDirEntryListIter iter = dentries.begin(); iter != dentries.end(); iter++ )
   {
      FsckFsID fsID(iter->getID(), iter->getParentDirID(), iter->getSaveNodeID(),
         iter->getSaveInode(), iter->getSaveInode());
      fsIDs.push_back(fsID);
   }

   bool delRes = this->database->deleteFsIDs(fsIDs, failedDelete, errorCode, dbHandle);
   if ( !delRes )
   {
      log.log(1,
         "Failed to delete fs-id file; entryID: " + failedDelete.getID() + "; sqlite error code: "
            + StringTk::intToStr(errorCode));
      throw FsckDBException(
         "Error while deleting fs-id files from database. Please see log for more "
            "information.");
   }
}

void ModeCheckFS::deleteFileInodesFromDB(FsckFileInodeList& fileInodes, DBHandle* dbHandle)
{
   FsckFileInode failedDelete;
   int errorCode;
   bool delRes = this->database->deleteFileInodes(fileInodes, failedDelete, errorCode, dbHandle);
   if ( !delRes )
   {
      log.log(1,
         "Failed to delete file inode; entryID: " + failedDelete.getID() + "; sqlite error code: "
            + StringTk::intToStr(errorCode));
      throw FsckDBException(
         "Error while deleting file inodes from database. Please see log for more "
            "information.");
   }
}

void ModeCheckFS::deleteDirInodesFromDB(FsckDirInodeList& dirInodes, DBHandle* dbHandle)
{
   FsckDirInode failedDelete;
   int errorCode;
   bool delRes = this->database->deleteDirInodes(dirInodes, failedDelete, errorCode, dbHandle);
   if ( !delRes )
   {
      log.log(1,
         "Failed to delete dir inode; entryID: " + failedDelete.getID() + "; sqlite error code: "
            + StringTk::intToStr(errorCode));
      throw FsckDBException(
         "Error while deleting dir inodes from database. Please see log for more "
            "information.");
   }
}

void ModeCheckFS::deleteChunksFromDB(FsckChunkList& chunks, DBHandle* dbHandle)
{
   FsckChunk failedDelete;
   int errorCode;
   bool delRes = this->database->deleteChunks(chunks, failedDelete, errorCode, dbHandle);
   if ( !delRes )
   {
      log.log(1,
         "Failed to delete chunk; chunkID: " + failedDelete.getID() + "; targetID: "
            + StringTk::uintToStr(failedDelete.getTargetID()) + "; sqlite error code: "
            + StringTk::intToStr(errorCode));
      throw FsckDBException("Error while deleting chunk from database. Please see log for more "
         "information.");
   }
}

void ModeCheckFS::deleteFilesFromDB(FsckDirEntryList& dentries, DBHandle* dbHandle)
{
   // create inodes an chunks from the dentries
   FsckFileInodeList inodes;
   FsckChunkList chunks;

   for ( FsckDirEntryListIter dentryIter = dentries.begin(); dentryIter != dentries.end();
      dentryIter++ )
   {
      FsckFileInode* inode = this->database->getFileInode(dentryIter->getID());

      if ( inode )
      {
         inodes.push_back(*inode);

         for ( UInt16VectorIter targetIter = inode->getStripeTargets()->begin();
            targetIter != inode->getStripeTargets()->end(); targetIter++ )
         {
            FsckChunk* chunk = this->database->getChunk(inode->getID(), *targetIter);
            if ( chunk )
            {
               chunks.push_back(*chunk);
               SAFE_DELETE(chunk);
            }
         }

         SAFE_DELETE(inode);
      }
   }

   // delete the chunks
   this->deleteChunksFromDB(chunks, dbHandle);

   // delete the inodes
   this->deleteFileInodesFromDB(inodes, dbHandle);

   // delete the dentries
   this->deleteDirEntriesFromDB(dentries, dbHandle);

   // delete the dentry-by-id files
   this->deleteFsIDsFromDB(dentries, dbHandle);
}

void ModeCheckFS::deleteDanglingDirEntriesFromDB(FsckDirEntryList& dentries, DBHandle* dbHandle)
{
   FsckDirEntry failedDelete;
   int errorCode;
   bool delRes = this->database->deleteDanglingDirEntries(dentries, failedDelete, errorCode,
      dbHandle);
   if ( !delRes )
   {
      log.log(1,
         "Failed to delete dangling directory entry; name: " + failedDelete.getName()
            + "; parentID: " + failedDelete.getParentDirID() + "; sqlite error code: "
            + StringTk::intToStr(errorCode));
      /*
       * NOTE: we do not throw an exception here, because fsck run is still correct.
       * An error here will just lead to a faulty error count displayed to the user
       */
   }
}

void ModeCheckFS::deleteOrphanedDirInodesFromDB(FsckDirInodeList& inodes, DBHandle* dbHandle)
{
   FsckDirInode failedDelete;
   int errorCode;
   bool delRes = this->database->deleteOrphanedInodes(inodes, failedDelete, errorCode, dbHandle);
   if ( !delRes )
   {
      log.log(1,
         "Failed to delete orphaned dir inode; entryID: " + failedDelete.getID()
            + "; sqlite error code: " + StringTk::intToStr(errorCode));
      /*
       * NOTE: we do not throw an exception here, because fsck run is still correct.
       * An error here will just lead to a faulty error count displayed to the user
       */
   }
}

void ModeCheckFS::deleteOrphanedFileInodesFromDB(FsckFileInodeList& inodes, DBHandle* dbHandle)
{
   FsckFileInode failedDelete;
   int errorCode;
   bool delRes = this->database->deleteOrphanedInodes(inodes, failedDelete, errorCode, dbHandle);
   if ( !delRes )
   {
      log.log(1,
         "Failed to delete orphaned file inode; entryID: " + failedDelete.getID()
            + "; sqlite error code: " + StringTk::intToStr(errorCode));
      /*
       * NOTE: we do not throw an exception here, because fsck run is still correct.
       * An error here will just lead to a faulty error count displayed to the user
       */
   }
}

void ModeCheckFS::deleteOrphanedChunksFromDB(FsckChunkList& chunks, DBHandle* dbHandle)
{
   FsckChunk failedDelete;
   int errorCode;
   bool delRes = this->database->deleteOrphanedChunks(chunks, failedDelete, errorCode, dbHandle);
   if ( !delRes )
   {
      log.log(1,
         "Failed to delete orphaned chunk; entryID: " + failedDelete.getID() + "; targetID: "
            + StringTk::uintToStr(failedDelete.getTargetID()) + "; sqlite error code: "
            + StringTk::intToStr(errorCode));
      /*
       * NOTE: we do not throw an exception here, because fsck run is still correct.
       * An error here will just lead to a faulty error count displayed to the user
       */
   }
}

void ModeCheckFS::deleteMissingContDirsFromDB(FsckDirInodeList& inodes, DBHandle* dbHandle)
{
   FsckDirInode failedDelete;
   int errorCode;
   bool delRes = this->database->deleteMissingContDirs(inodes, failedDelete, errorCode, dbHandle);
   if ( !delRes )
   {
      log.log(1,
         "Failed to delete missing content directory; entryID: " + failedDelete.getID()
            + "; sqlite error code: " + StringTk::intToStr(errorCode));
      /*
       * NOTE: we do not throw an exception here, because fsck run is still correct.
       * An error here will just lead to a faulty error count displayed to the user
       */
   }
}

void ModeCheckFS::deleteOrphanedContDirsFromDB(FsckContDirList& contDirs, DBHandle* dbHandle)
{
   FsckContDir failedDelete;
   int errorCode;
   bool delRes = this->database->deleteOrphanedContDirs(contDirs, failedDelete, errorCode,
      dbHandle);
   if ( !delRes )
   {
      log.log(1,
         "Failed to delete orphaned content directory; entryID: " + failedDelete.getID()
            + "; sqlite error code: " + StringTk::intToStr(errorCode));
      /*
       * NOTE: we do not throw an exception here, because fsck run is still correct.
       * An error here will just lead to a faulty error count displayed to the user
       */
   }
}

void ModeCheckFS::deleteFileInodesWithWrongAttribsFromDB(FsckFileInodeList& inodes,
   DBHandle* dbHandle)
{
   FsckFileInode failedDelete;
   int errorCode;
   bool delRes = this->database->deleteInodesWithWrongAttribs(inodes, failedDelete, errorCode,
      dbHandle);
   if ( !delRes )
   {
      log.log(1,
         "Failed to delete file inode with wrong attributes; entryID: " + failedDelete.getID()
            + "; sqlite error code: " + StringTk::intToStr(errorCode));
      /*
       * NOTE: we do not throw an exception here, because fsck run is still correct.
       * An error here will just lead to a faulty error count displayed to the user
       */
   }
}

void ModeCheckFS::deleteDirInodesWithWrongAttribsFromDB(FsckDirInodeList& inodes,
   DBHandle* dbHandle)
{
   FsckDirInode failedDelete;
   int errorCode;
   bool delRes = this->database->deleteInodesWithWrongAttribs(inodes, failedDelete, errorCode,
      dbHandle);
   if ( !delRes )
   {
      log.log(1,
         "Failed to delete directory inode with wrong attributes; entryID: " + failedDelete.getID()
            + "; sqlite error code: " + StringTk::intToStr(errorCode));
      /*
       * NOTE: we do not throw an exception here, because fsck run is still correct.
       * An error here will just lead to a faulty error count displayed to the user
       */
   }
}

void ModeCheckFS::deleteFilesWithMissingTargetFromDB(FsckDirEntryList& dentries, DBHandle* dbHandle)
{
   FsckDirEntry failedDelete;
   int errorCode;
   bool delRes = this->database->deleteFilesWithMissingStripeTargets(dentries, failedDelete,
      errorCode, dbHandle);
   if ( !delRes )
   {
      log.log(1,
         "Failed to delete directory entry with missing target; name: " + failedDelete.getName()
            + "; parentID: " + failedDelete.getParentDirID() + "; sqlite error code: "
            + StringTk::intToStr(errorCode));
      /*
       * NOTE: we do not throw an exception here, because fsck run is still correct.
       * An error here will just lead to a faulty error count displayed to the user
       */
   }
}

void ModeCheckFS::deleteDirEntriesWithBrokenByIDFileFromDB(FsckDirEntryList& dentries,
   DBHandle* dbHandle)
{
   FsckDirEntry failedDelete;
   int errorCode;
   bool delRes = this->database->deleteDirEntriesWithBrokenByIDFile(dentries, failedDelete,
      errorCode, dbHandle);
   if ( !delRes )
   {
      log.log(1,
         "Failed to delete directory entry with broken by-id file; name: " + failedDelete.getName()
            + "; parentID: " + failedDelete.getParentDirID() + "; sqlite error code: "
            + StringTk::intToStr(errorCode));
      /*
       * NOTE: we do not throw an exception here, because fsck run is still correct.
       * An error here will just lead to a faulty error count displayed to the user
       */
   }
}

void ModeCheckFS::deleteOrphanedDentryByIDFilesFromDB(FsckFsIDList& fsIDs, DBHandle* dbHandle)
{
   FsckFsID failedDelete;
   int errorCode;

   bool delRes = this->database->deleteOrphanedDentryByIDFiles(fsIDs, failedDelete, errorCode,
      dbHandle);

   if ( !delRes )
   {
      log.log(1,
         "Failed to delete orphaned by-id file; entryID: " + failedDelete.getID()
            + "; sqlite error code: " + StringTk::intToStr(errorCode));
      /*
       * NOTE: we do not throw an exception here, because fsck run is still correct.
       * An error here will just lead to a faulty error count displayed to the user
       */
   }
}

void ModeCheckFS::deleteChunksWithWrongPermissionsFromDB(FsckChunkList& chunks, DBHandle* dbHandle)
{
   FsckChunk failedDelete;
   int errorCode;

   bool delRes = this->database->deleteChunksWithWrongPermissions(chunks, failedDelete, errorCode,
      dbHandle);
   if ( !delRes )
   {
      log.log(1,
         "Failed to delete chunk with wrong permission; entryID: " + failedDelete.getID()
            + "; targetID: " + StringTk::uintToStr(failedDelete.getTargetID())
            + "; sqlite error code: " + StringTk::intToStr(errorCode));
      /*
       * NOTE: we do not throw an exception here, because fsck run is still correct.
       * An error here will just lead to a faulty error count displayed to the user
       */
   }
}

void ModeCheckFS::deleteChunksInWrongPathFromDB(FsckChunkList& chunks, DBHandle* dbHandle)
{
   FsckChunk failedDelete;
   int errorCode;

   bool delRes = this->database->deleteChunksInWrongPath(chunks, failedDelete, errorCode,
      dbHandle);
   if ( !delRes )
   {
      log.log(1,
         "Failed to delete chunk in wrong path; entryID: " + failedDelete.getID()
            + "; targetID: " + StringTk::uintToStr(failedDelete.getTargetID())
            + "; sqlite error code: " + StringTk::intToStr(errorCode));
      /*
       * NOTE: we do not throw an exception here, because fsck run is still correct.
       * An error here will just lead to a faulty error count displayed to the user
       */
   }
}

void ModeCheckFS::insertCreatedDirInodesToDB(FsckDirInodeList& dirInodes, DBHandle* dbHandle)
{
   FsckDirInode failedInsert;
   int errorCode;

   bool insertRes = this->database->insertDirInodes(dirInodes, failedInsert, errorCode, dbHandle);
   if ( !insertRes )
   {
      log.log(Log_CRITICAL, "Failed to insert newly created dir inode to database. "
         "entryID: " + failedInsert.getID());
      log.log(Log_CRITICAL, "SQLite Error was error code " + StringTk::intToStr(errorCode));

      throw FsckDBException(
         "Could not insert newly created dir inode. Database consistency cannot be "
            "guaranteed.");
   }
}

void ModeCheckFS::insertCreatedFileInodesToDB(FsckFileInodeList& fileInodes, DBHandle* dbHandle)
{
   FsckFileInode failedInsert;
   int errorCode;

   bool insertRes = this->database->insertFileInodes(fileInodes, failedInsert, errorCode, dbHandle);
   if ( !insertRes )
   {
      log.log(Log_CRITICAL, "Failed to insert newly created file inode to database. "
         "entryID: " + failedInsert.getID());
      log.log(Log_CRITICAL, "SQLite Error was error code " + StringTk::intToStr(errorCode));

      throw FsckDBException(
         "Could not insert newly created file inode. Database consistency cannot be "
            "guaranteed.");
   }
}

void ModeCheckFS::insertCreatedDirEntriesToDB(FsckDirEntryList& dirEntries, DBHandle* dbHandle)
{
   FsckDirEntry failedInsert;
   int errorCode;

   bool insertRes = this->database->insertDirEntries(dirEntries, failedInsert, errorCode, dbHandle);
   if ( !insertRes )
   {
      log.log(Log_CRITICAL, "Failed to insert newly created directory entry to database. "
         "entryID: " + failedInsert.getID());
      log.log(Log_CRITICAL, "SQLite Error was error code " + StringTk::intToStr(errorCode));

      throw FsckDBException(
         "Could not insert newly created directory entry. Database consistency cannot be "
            "guaranteed.");
   }
}

void ModeCheckFS::insertCreatedFsIDsToDB(FsckFsIDList& fsIDs, DBHandle* dbHandle)
{
   FsckFsID failedInsert;
   int errorCode;

   bool insertRes = this->database->insertFsIDs(fsIDs, failedInsert, errorCode, dbHandle);
   if ( !insertRes )
   {
      log.log(Log_CRITICAL, "Failed to insert newly created dentry-by-ID file to database. "
         "entryID: " + failedInsert.getID());
      log.log(Log_CRITICAL, "SQLite Error was error code " + StringTk::intToStr(errorCode));

      throw FsckDBException(
         "Could not insert newly created dentry-by-ID file. Database consistency cannot be "
            "guaranteed.");
   }
}

void ModeCheckFS::insertCreatedContDirsToDB(FsckDirInodeList& dirInodes, DBHandle* dbHandle)
{
   FsckContDir failedInsert;
   int errorCode;

   // create a list of cont dirs from dir inode list
   FsckContDirList contDirs;
   for ( FsckDirInodeListIter iter = dirInodes.begin(); iter != dirInodes.end(); iter++ )
   {
      FsckContDir contDir(iter->getID(), iter->getSaveNodeID());
      contDirs.push_back(contDir);
   }

   bool insertRes = this->database->insertContDirs(contDirs, failedInsert, errorCode, dbHandle);
   if ( !insertRes )
   {
      log.log(Log_CRITICAL, "Failed to insert newly created contents directory to database. "
         "entryID: " + failedInsert.getID());
      log.log(Log_CRITICAL, "SQLite Error was error code " + StringTk::intToStr(errorCode));

      throw FsckDBException(
         "Could not insert newly created contents directory. Database consistency cannot be "
            "guaranteed.");
   }
}

void ModeCheckFS::updateFileInodesInDB(FsckFileInodeList& inodes, DBHandle* dbHandle)
{
   FsckFileInode failedUpdate;
   int errorCode;

   bool updateRes = this->database->updateFileInodes(inodes, failedUpdate, errorCode, dbHandle);

   if ( !updateRes )
   {
      log.log(1,
         "Failed to update file inode ; entryID: " + failedUpdate.getID() + "; sqlite error code: "
            + StringTk::intToStr(errorCode));
      throw FsckDBException(
         "Error while updating file inodes to database. Please see log for more information.");
   }
}

void ModeCheckFS::updateDirInodesInDB(FsckDirInodeList& inodes, DBHandle* dbHandle)
{
   FsckDirInode failedUpdate;
   int errorCode;
   bool updateRes = this->database->updateDirInodes(inodes, failedUpdate, errorCode, dbHandle);

   if ( !updateRes )
   {
      log.log(1,
         "Failed to update directory inode ; entryID: " + failedUpdate.getID()
            + "; sqlite error code: " + StringTk::intToStr(errorCode));
      throw FsckDBException(
         "Error while updating directory inodes to database. Please see log for more information.");
   }
}

/*
 * this function does not do an update in the SQL "UPDATE" sense. It deletes entries (if they exist)
 * and creates new ones. This is because we do not know if the entries existed (and were faulty)
 * or not
 */
void ModeCheckFS::updateFsIDsInDB(FsckDirEntryList& dirEntries, DBHandle* dbHandle)
{
   // create a FsID list from the dentries
   FsckFsIDList idList;

   for ( FsckDirEntryListIter iter = dirEntries.begin(); iter != dirEntries.end(); iter++ )
   {
      FsckFsID fsID(iter->getID(), iter->getParentDirID(), iter->getSaveNodeID(),
         iter->getSaveDevice(), iter->getSaveInode());
      idList.push_back(fsID);
   }

   FsckFsID failedDelete; // is ignored
   int errorCode;
   this->database->deleteFsIDs(idList, failedDelete, errorCode, dbHandle);

   FsckFsID failedInsert;
   bool insertRes = this->database->insertFsIDs(idList, failedInsert, errorCode, dbHandle);
   if ( !insertRes )
   {
      log.log(Log_CRITICAL, "Failed to update dentry-by-ID file to database. "
         "entryID: " + failedInsert.getID());
      log.log(Log_CRITICAL, "SQLite Error was error code " + StringTk::intToStr(errorCode));

      throw FsckDBException(
         "Could not update dentry-by-ID file. Database consistency cannot be guaranteed.");
   }
}

void ModeCheckFS::updateChunksInDB(FsckChunkList& chunks, DBHandle* dbHandle)
{
   FsckChunk failedUpdate;
   int errorCode;

   bool updateRes = this->database->updateChunks(chunks, failedUpdate, errorCode, dbHandle);

   if ( !updateRes )
   {
      log.log(1,
         "Failed to update chunk ; entryID: " + failedUpdate.getID() + "; targetID: "
            + StringTk::uintToStr(failedUpdate.getTargetID()) + +"; sqlite error code: "
            + StringTk::intToStr(errorCode));
      throw FsckDBException(
         "Error while updating directory inodes to database. Please see log for more information.");
   }
}
