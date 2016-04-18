/*
 * This class is intended to be the interface to the sqlite DB. It can be created once in the
 * applicationm, and can then be used in all threads, as sqlite claims to be thread-safe in
 * "serialized" mode (which is the default, http://sqlite.org/threadsafe.html).
 */

#ifndef FSCKDB_H_
#define FSCKDB_H_

#include <common/app/log/LogContext.h>
#include <common/Common.h>
#include <common/fsck/FsckChunk.h>
#include <common/fsck/FsckContDir.h>
#include <common/fsck/FsckDirEntry.h>
#include <common/fsck/FsckDirInode.h>
#include <common/fsck/FsckFileInode.h>
#include <common/fsck/FsckTargetID.h>
#include <common/nodes/TargetMapper.h>
#include <common/storage/striping/StripePattern.h>
#include <common/storage/StorageDefinitions.h>
#include <common/threading/RWLock.h>
#include <database/DBCursor.h>
#include <database/DBHandlePool.h>
#include <database/DBTable.h>
#include <toolkit/FsckDefinitions.h>

class FsckDB
{
   friend class TestDatabase;

   public:
      // FsckDB.cpp
      FsckDB(std::string dbFilename);
      virtual ~FsckDB();

      void init(bool clearDB = true);

      // FsckDBGetContents.cpp
      DBCursor<FsckDirEntry>* getDirEntries();
      void getDirEntries(FsckDirEntryList* outDentries);
      DBCursor<FsckDirEntry>* getDirEntries(std::string id, uint maxCount = 0);
      void getDirEntries(std::string id, FsckDirEntryList* outDentries);
      DBCursor<FsckDirEntry>* getDirEntriesByParentID(std::string parentID);
      int64_t countDirEntries();

      DBCursor<FsckFileInode>* getFileInodes();
      void getFileInodes(FsckFileInodeList* outFileInodes);
      DBCursor<FsckFileInode>* getFileInodesWithTarget(uint16_t nodeID, FsckTargetID& fsckTargetID);
      FsckFileInode* getFileInode(std::string id);
      int64_t countFileInodes();

      DBCursor<FsckDirInode>* getDirInodes();
      void getDirInodes(FsckDirInodeList* outDirInodes);
      FsckDirInode* getDirInode(std::string id);
      int64_t countDirInodes();

      DBCursor<FsckChunk>* getChunks();
      DBCursor<FsckChunk>* getChunks(std::string chunkID, bool onlyPrimary=false);
      void getChunks(FsckChunkList* outChunks);
      FsckChunk* getChunk(std::string chunkID, uint16_t targetID);
      FsckChunk* getChunk(std::string chunkID, uint16_t targetID, uint16_t buddyGroupID);
      int64_t countChunks();

      DBCursor<FsckContDir>* getContDirs();
      void getContDirs(FsckContDirList* outContDirs);
      int64_t countContDirs();

      DBCursor<FsckFsID>* getFsIDs();
      void getFsIDs(FsckFsIDList* outContDirs);
      int64_t countFsIDs();

      DBCursor<FsckTargetID>* getUsedTargetIDs();
      void getUsedTargetIDs(FsckTargetIDList* outTargetIDs);
      int64_t countUsedTargetIDs();

      DBCursor<FsckModificationEvent>* getModificationEvents();
      void getModificationEvents(FsckModificationEventList* outEvents);
      int64_t countModificationEvents();

      DBCursor<FsckDirEntry>* getDanglingDirEntries();
      DBCursor<FsckDirEntry>* getDanglingDirEntries(FsckRepairAction repairAction);
      DBCursor<FsckDirEntry>* getDanglingDirEntries(FsckRepairAction repairAction,
         uint16_t nodeID);
      DBCursor<FsckDirEntry>* getDanglingDirEntriesByInodeOwner(FsckRepairAction repairAction,
         uint16_t inodeOwnerID);
      void getDanglingDirEntries(FsckDirEntryList* outDentries);

      DBCursor<FsckDirEntry>* getDanglingDirectoryDirEntries();
      DBCursor<FsckDirEntry>* getDanglingDirectoryDirEntries(FsckRepairAction repairAction);
      DBCursor<FsckDirEntry>* getDanglingFileDirEntries();
      DBCursor<FsckDirEntry>* getDanglingFileDirEntries(FsckRepairAction repairAction);

      DBCursor<FsckDirInode>* getInodesWithWrongOwner();
      DBCursor<FsckDirInode>* getInodesWithWrongOwner(FsckRepairAction repairAction);
      void getInodesWithWrongOwner(FsckDirInodeList* outDirInodes);
      DBCursor<FsckDirInode>* getInodesWithWrongOwner(FsckRepairAction repairAction,
         uint16_t nodeID);

      DBCursor<FsckDirEntry>* getDirEntriesWithWrongOwner();
      DBCursor<FsckDirEntry>* getDirEntriesWithWrongOwner(FsckRepairAction repairAction);
      DBCursor<FsckDirEntry>* getDirEntriesWithWrongOwner(FsckRepairAction repairAction,
         uint16_t nodeID);
      void getDirEntriesWithWrongOwner(FsckDirEntryList* outDentries);

      DBCursor<FsckDirEntry>* getDirEntriesWithBrokenByIDFile();
      DBCursor<FsckDirEntry>* getDirEntriesWithBrokenByIDFile(FsckRepairAction repairAction);
      DBCursor<FsckDirEntry>* getDirEntriesWithBrokenByIDFile(FsckRepairAction repairAction,
         uint16_t nodeID);
      void getDirEntriesWithBrokenByIDFile(FsckDirEntryList* outDentries);

      DBCursor<FsckFsID>* getOrphanedDentryByIDFiles();
      DBCursor<FsckFsID>* getOrphanedDentryByIDFiles(FsckRepairAction repairAction);
      DBCursor<FsckFsID>* getOrphanedDentryByIDFiles(FsckRepairAction repairAction,
         uint16_t nodeID);
      void getOrphanedDentryByIDFiles(FsckFsIDList* outIDs);

      DBCursor<FsckFileInode>* getOrphanedFileInodes();
      DBCursor<FsckFileInode>* getOrphanedFileInodes(FsckRepairAction repairAction);
      DBCursor<FsckFileInode>* getOrphanedFileInodes(FsckRepairAction repairAction,
         uint16_t saveNodeID);
      void getOrphanedFileInodes(FsckFileInodeList* outFileInodes);

      DBCursor<FsckDirInode>* getOrphanedDirInodes();
      DBCursor<FsckDirInode>* getOrphanedDirInodes(FsckRepairAction repairAction);
      void getOrphanedDirInodes(FsckDirInodeList* outDirInodes);

      DBCursor<FsckChunk>* getOrphanedChunks(bool orderedByID = false);
      DBCursor<FsckChunk>* getOrphanedChunks(FsckRepairAction repairAction,
         bool orderedByID = false);
      DBCursor<FsckChunk>* getOrphanedChunks(FsckRepairAction repairAction,
         uint16_t targetID);
      void getOrphanedChunks(FsckChunkList* outChunks);

      DBCursor<FsckDirInode>* getInodesWithoutContDir();
      DBCursor<FsckDirInode>* getInodesWithoutContDir(FsckRepairAction repairAction);
      DBCursor<FsckDirInode>* getInodesWithoutContDir(FsckRepairAction repairAction,
         uint16_t nodeID);
      void getInodesWithoutContDir(FsckDirInodeList* outInodes);

      DBCursor<FsckContDir>* getOrphanedContDirs();
      DBCursor<FsckContDir>* getOrphanedContDirs(FsckRepairAction repairAction);
      DBCursor<FsckContDir>* getOrphanedContDirs(FsckRepairAction repairAction,
         uint16_t nodeID);
      void getOrphanedContDirs(FsckContDirList* outContDirs);

      DBCursor<FsckFileInode>* getInodesWithWrongFileAttribs();
      DBCursor<FsckFileInode>* getInodesWithWrongFileAttribs(FsckRepairAction repairAction);
      DBCursor<FsckFileInode>* getInodesWithWrongFileAttribs(FsckRepairAction repairAction,
         uint16_t nodeID);
      void getInodesWithWrongFileAttribs(FsckFileInodeList* outFileInodes);

      DBCursor<FsckDirInode>* getInodesWithWrongDirAttribs();
      DBCursor<FsckDirInode>* getInodesWithWrongDirAttribs(FsckRepairAction repairAction);
      DBCursor<FsckDirInode>* getInodesWithWrongDirAttribs(FsckRepairAction repairAction,
         uint16_t nodeID);
      void getInodesWithWrongDirAttribs(FsckDirInodeList* outDirInodes);

      DBCursor<FsckTargetID>* getMissingStripeTargets();
      DBCursor<FsckTargetID>* getMissingStripeTargets(FsckRepairAction repairAction);
      void getMissingStripeTargets(FsckTargetIDList* outStripeTargets);
      void getMissingStripeTargets(FsckTargetIDList* outStripeTargets,
         FsckRepairAction repairAction);

      DBCursor<FsckDirEntry>* getFilesWithMissingStripeTargets();
      DBCursor<FsckDirEntry>* getFilesWithMissingStripeTargets(FsckRepairAction repairAction);
      DBCursor<FsckDirEntry>* getFilesWithMissingStripeTargets(FsckRepairAction repairAction,
         uint16_t dentrySaveNodeID);
      void getFilesWithMissingStripeTargets(FsckDirEntryList* outDirEntries);
      void getFilesWithMissingStripeTargets(FsckDirEntryList* outDirEntries,
         FsckRepairAction repairAction);
      DBCursor<FsckChunk>* getMissingMirrorChunks(bool orderedByID = false);
      DBCursor<FsckChunk>* getMissingMirrorChunks(FsckRepairAction repairAction,
         bool orderedByID = false);
      DBCursor<FsckChunk>* getMissingMirrorChunks(FsckRepairAction repairAction, uint16_t targetID);
      void getMissingMirrorChunks(FsckChunkList* outChunks);
      DBCursor<FsckChunk>* getMissingPrimaryChunks(bool orderedByID = false);
      DBCursor<FsckChunk>* getMissingPrimaryChunks(FsckRepairAction repairAction,
         bool orderedByID = false);
      DBCursor<FsckChunk>* getMissingPrimaryChunks(FsckRepairAction repairAction, uint16_t targetID);
      void getMissingPrimaryChunks(FsckChunkList* outChunks);
      DBCursor<FsckChunk>* getDifferingChunkAttribs(bool orderedByID = false);
      DBCursor<FsckChunk>* getDifferingChunkAttribs(FsckRepairAction repairAction,
         bool orderedByID = false);
      DBCursor<FsckChunk>* getDifferingChunkAttribs(FsckRepairAction repairAction, uint16_t targetID);
      void getDifferingChunkAttribs(FsckChunkList* outChunks);
      DBCursor<FsckChunk>* getChunksWithWrongPermissions(bool orderedByID = false);
      DBCursor<FsckChunk>* getChunksWithWrongPermissions(FsckRepairAction repairAction,
         bool orderedByID = false);
      DBCursor<FsckChunk>* getChunksWithWrongPermissions(FsckRepairAction repairAction,
         uint16_t targetID);
      void getChunksWithWrongPermissions(FsckChunkList* outChunks);
      DBCursor<FsckChunk>* getChunksInWrongPath(bool orderedByID = false);
      DBCursor<FsckChunk>* getChunksInWrongPath(FsckRepairAction repairAction,
         bool orderedByID = false);
      DBCursor<FsckChunk>* getChunksInWrongPath(FsckRepairAction repairAction, uint16_t targetID);

      int64_t countErrors(FsckErrorCode errorCode);

      // FsckDBInsertContents.cpp
      bool insertDirEntries(FsckDirEntryList& dentries, FsckDirEntry& outFailedInsert,
         int& outErrorCode, DBHandle* dbHandle = NULL);
      bool insertFileInodes(FsckFileInodeList& fileInodes, FsckFileInode& outFailedInsert,
         int& outErrorCode, DBHandle* dbHandle = NULL);
      bool insertDirInodes(FsckDirInodeList& fileInodes, FsckDirInode& outFailedInsert,
         int& outErrorCode, DBHandle* dbHandle = NULL);
      bool insertChunks(FsckChunkList& chunks, FsckChunk& outFailedInsert,
         int& outErrorCode, DBHandle* dbHandle = NULL);
      bool insertContDirs(FsckContDirList& contDirs, FsckContDir& outFailedInsert,
         int& outErrorCode, DBHandle* dbHandle = NULL);
      bool insertFsIDs(FsckFsIDList& fsIDs, FsckFsID& outFailedInsert,
         int& outErrorCode, DBHandle* dbHandle = NULL);
      bool insertUsedTargetIDs(FsckTargetIDList& targetIDs, FsckTargetID& outFailedInsert,
         int& outErrorCode, DBHandle* dbHandle = NULL);
      bool insertModificationEvents(FsckModificationEventList& events,
         FsckModificationEvent& outFailedInsert, int& outErrorCode,
         DBHandle* dbHandle = NULL);

      bool updateDirEntries(FsckDirEntryList& dentries, FsckDirEntry& outFailedUpdates,
         int& outErrorCode, DBHandle* dbHandle = NULL);
      bool updateDirInodes(FsckDirInodeList& inodes, FsckDirInode& outFailedUpdate,
         int& outErrorCode, DBHandle* dbHandle = NULL);
      bool updateFileInodes(FsckFileInodeList& inodes, FsckFileInode& outFailedUpdate,
         int& outErrorCode, DBHandle* dbHandle = NULL);
      bool updateChunks(FsckChunkList& chunks, FsckChunk& outFailedUpdate,
         int& outErrorCode, DBHandle* dbHandle = NULL);
      bool updateFsIDs(FsckFsIDList& fsIDs, FsckFsID& outFailedUpdate,
         int& outErrorCode, DBHandle* dbHandle = NULL);

      bool replaceStripeTarget(uint16_t oldTargetID, uint16_t newTargetID,
         FsckFileInodeList* inodes = NULL, DBHandle* dbHandle = NULL);

      bool processModificationEvents();

      // FsckDBErrorsAndRepair.cpp
      bool addIgnoreErrorCode(FsckDirEntry& dirEntry, FsckErrorCode errorCode, DBHandle* dbHandle =
         NULL);
      bool addIgnoreAllErrorCodes(FsckDirEntry& dirEntry, DBHandle* dbHandle = NULL);

      bool addIgnoreErrorCode(FsckFileInode& fileInode, FsckErrorCode errorCode,
         DBHandle* dbHandle = NULL);
      bool addIgnoreAllErrorCodes(FsckFileInode& fileInode, DBHandle* dbHandle = NULL);

      bool addIgnoreErrorCode(FsckDirInode& dirInode, FsckErrorCode errorCode, DBHandle* dbHandle =
         NULL);
      bool addIgnoreAllErrorCodes(FsckDirInode& dirInode, DBHandle* dbHandle = NULL);

      bool addIgnoreErrorCode(FsckChunk& chunk, FsckErrorCode errorCode, DBHandle* dbHandle = NULL);
      bool addIgnoreAllErrorCodes(FsckChunk& chunk, DBHandle* dbHandle = NULL);

      bool addIgnoreErrorCode(FsckContDir& contDir, FsckErrorCode errorCode, DBHandle* dbHandle =
         NULL);
      bool addIgnoreAllErrorCodes(FsckContDir& contDir, DBHandle* dbHandle = NULL);

      bool addIgnoreErrorCode(FsckFsID& fsID, FsckErrorCode errorCode, DBHandle* dbHandle =
         NULL);
      bool addIgnoreAllErrorCodes(FsckFsID& fsID, DBHandle* dbHandle = NULL);

      bool addIgnoreErrorCode(FsckTargetID& targetID, FsckErrorCode errorCode,
         DBHandle* dbHandle = NULL);
      bool addIgnoreAllErrorCodes(FsckTargetID& targetID, DBHandle* dbHandle = NULL);

      bool setRepairAction(FsckDirEntry& dirEntry, FsckErrorCode errorCode,
         FsckRepairAction repairAction, DBHandle* dbHandle = NULL);
      unsigned setRepairAction(FsckDirEntryList& dentries, FsckErrorCode errorCode,
         FsckRepairAction repairAction, FsckDirEntryList* outFailedUpdates, IntList* outErrorCodes,
         DBHandle* dbHandle = NULL);

      bool setRepairAction(FsckFileInode& fileInode, FsckErrorCode errorCode,
         FsckRepairAction repairAction, DBHandle* dbHandle = NULL);
      unsigned setRepairAction(FsckFileInodeList& dentries, FsckErrorCode errorCode,
         FsckRepairAction repairAction, FsckFileInodeList* outFailedUpdates,
         IntList* outErrorCodes, DBHandle* dbHandle = NULL);

      bool setRepairAction(FsckDirInode& dirInode, FsckErrorCode errorCode,
         FsckRepairAction repairAction, DBHandle* dbHandle = NULL);
      unsigned setRepairAction(FsckDirInodeList& dentries, FsckErrorCode errorCode,
         FsckRepairAction repairAction, FsckDirInodeList* outFailedUpdates, IntList* outErrorCodes,
         DBHandle* dbHandle = NULL);

      bool setRepairAction(FsckChunk& chunk, FsckErrorCode errorCode,
         FsckRepairAction repairAction, DBHandle* dbHandle = NULL);
      unsigned setRepairAction(FsckChunkList& chunks, FsckErrorCode errorCode,
         FsckRepairAction repairAction, FsckChunkList* outFailedUpdates, IntList* outErrorCodes,
         DBHandle* dbHandle = NULL);

      bool setRepairAction(FsckContDir& contDir, FsckErrorCode errorCode,
         FsckRepairAction repairAction, DBHandle* dbHandle = NULL);
      unsigned setRepairAction(FsckContDirList& contDirs, FsckErrorCode errorCode,
         FsckRepairAction repairAction, FsckContDirList* outFailedUpdates, IntList* outErrorCodes,
         DBHandle* dbHandle = NULL);

      bool setRepairAction(FsckFsID& fsID, FsckErrorCode errorCode,
         FsckRepairAction repairAction, DBHandle* dbHandle = NULL);
      unsigned setRepairAction(FsckFsIDList& fsIDs, FsckErrorCode errorCode,
         FsckRepairAction repairAction, FsckFsIDList* outFailedUpdates, IntList* outErrorCodes,
         DBHandle* dbHandle = NULL);

      bool setRepairAction(FsckTargetID& targetID, FsckErrorCode errorCode,
         FsckRepairAction repairAction, DBHandle* dbHandle = NULL);
      unsigned setRepairAction(FsckTargetIDList& targetIDs, FsckErrorCode errorCode,
         FsckRepairAction repairAction, FsckTargetIDList* outFailedUpdates, IntList* outErrorCodes,
         DBHandle* dbHandle = NULL);

      // FsckDBDeleteContents.cpp
      bool deleteDirEntries(FsckDirEntryList& dentries, FsckDirEntry& outFailedDelete,
         int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteDirInodes(FsckDirInodeList& dirInodes, FsckDirInode& outFailedDeletes,
         int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteFileInodes(FsckFileInodeList& fileInodes, FsckFileInode& outFailedDeletes,
         int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteChunks(FsckChunkList& chunks, FsckChunk& outFailedDeletes,
         int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteFsIDs(FsckFsIDList& fsIDs, FsckFsID& outFailedDeletes,
         int& outErrorCode, DBHandle* dbHandle = NULL);

      bool deleteDanglingDirEntries(FsckDirEntryList& dentries,
         FsckDirEntry& outFailedDelete, int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteInodesWithWrongOwner(FsckDirInodeList& inodes,
         FsckDirInode& outFailedDelete, int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteDirEntriesWithWrongOwner(FsckDirEntryList& dentries,
         FsckDirEntry& outFailedDelete, int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteOrphanedInodes(FsckDirInodeList& inodes, FsckDirInode& outFailedDelete,
         int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteOrphanedInodes(FsckFileInodeList& inodes, FsckFileInode& outFailedDelete,
         int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteOrphanedChunks(FsckChunkList& inodes, FsckChunk& outFailedDelete,
         int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteMissingContDirs(FsckDirInodeList& inodes,
         FsckDirInode& outFailedDelete, int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteOrphanedContDirs(FsckContDirList& contDirs,
         FsckContDir& outFailedDelete, int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteInodesWithWrongAttribs(FsckFileInodeList& inodes,
         FsckFileInode& outFailedDelete, int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteInodesWithWrongAttribs(FsckDirInodeList& inodes,
         FsckDirInode& outFailedDelete, int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteMissingStripeTargets(FsckTargetIDList& targetIDs, FsckTargetID& outFailedDelete,
         int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteFilesWithMissingStripeTargets(FsckDirEntryList& dentries,
         FsckDirEntry& outFailedDelete, int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteDirEntriesWithBrokenByIDFile(FsckDirEntryList& dentries,
         FsckDirEntry& outFailedDelete, int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteOrphanedDentryByIDFiles(FsckFsIDList& fsIDs, FsckFsID& outFailedDelete,
         int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteChunksWithWrongPermissions(FsckChunkList& chunks,
         FsckChunk& outFailedDelete, int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteChunksInWrongPath(FsckChunkList& chunks,
         FsckChunk& outFailedDelete, int& outErrorCode, DBHandle* dbHandle = NULL);

      // FsckDBChecks.cpp
      bool checkForAndInsertDanglingDirEntries();
      bool checkForAndInsertDirEntriesWithoutFileInode();
      bool checkForAndInsertDirEntriesWithoutDirInode();

      bool checkForAndInsertDirEntriesWithBrokenByIDFile();
      bool checkForAndInsertOrphanedDentryByIDFiles();

      bool checkForAndInsertInodesWithWrongOwner();

      bool checkForAndInsertDirEntriesWithWrongOwner();
      bool checkForAndInsertDirEntriesWithWrongFileOwner();
      bool checkForAndInsertDirEntriesWithWrongDirOwner();

      bool checkForAndInsertOrphanedDirInodes();
      bool checkForAndInsertOrphanedFileInodes();
      bool checkForAndInsertOrphanedChunks();
      bool checkForAndInsertInodesWithoutContDir();
      bool checkForAndInsertOrphanedContDirs();

      bool checkForAndInsertWrongInodeFileAttribs();
      bool checkForAndInsertWrongInodeDirAttribs();

      bool checkForAndInsertMissingStripeTargets(TargetMapper* targetMapper,
         MirrorBuddyGroupMapper* buddyGroupMapper);
      bool checkForAndInsertFilesWithMissingStripeTargets();

      bool checkForAndInsertChunksWithWrongPermissions();
      bool checkForAndInsertChunksInWrongPath();

   private:
      LogContext log;
      std::string dbFilename;
      DBHandlePool* dbHandlePool;
      RWLock rwlock;

      // // FsckDBCreateTables.cpp
      void createTables();

      void createTable(DBTable& tableDefinition);
      void createTableDirEntries();
      void createTableFileInodes();
      void createTableDirInodes();
      void createTableChunks();
      void createTableContDirs();
      void createTableFsIDs();
      void createTableUsedTargets();

      void createTableDanglingDentry();
      void createTableInodesWithWrongOwner();
      void createTableDirEntriesWithWrongOwner();
      void createTableDentriesWithBrokenByIDFile();
      void createTableOrphanedDentryByIDFiles();
      void createTableOrphanedDirInodes();
      void createTableOrphanedFileInodes();
      void createTableOrphanedChunks();
      void createTableInodesWithoutContDir();
      void createTableOrphanedContDirs();
      void createTableWrongFileAttribs();
      void createTableWrongDirAttribs();
      void createTableMissingStorageTargets();
      void createTableFilesWithMissingTargets();
      void createTableMissingMirrorChunks();
      void createTableMissingPrimaryChunks();
      void createTableDifferingChunkAttribs();
      void createTableChunksWithWrongPermissions();
      void createTableChunksInWrongPath();

      void createTableModificationEvents();

      // FsckDBGetContents.cpp
      DBCursor<FsckDirEntry>* getDirEntriesInternal(std::string sqlWhereClause ="1",
         uint maxCount = 0);
      DBCursor<FsckFileInode>* getFileInodesInternal(std::string sqlWhereClause ="1");
      DBCursor<FsckDirInode>* getDirInodesInternal(std::string sqlWhereClause ="1");
      DBCursor<FsckChunk>* getChunksInternal(std::string sqlWhereClause ="1");
      DBCursor<FsckContDir>* getContDirsInternal(std::string sqlWhereClause ="1");
      DBCursor<FsckFsID>* getFsIDsInternal(std::string sqlWhereClause ="1");
      DBCursor<FsckTargetID>* getUsedTargetIDsInternal(std::string sqlWhereClause ="1");
      DBCursor<FsckModificationEvent>* getModificationEventsInternal
      (std::string sqlWhereClause ="1");

      DBCursor<FsckDirEntry>* getDanglingDirEntriesInternal(std::string sqlWhereClause = "1");
      DBCursor<FsckDirEntry>* getDirEntriesWithWrongOwnerInternal(std::string sqlWhereClause = "1");
      DBCursor<FsckDirEntry>* getDirEntriesWithBrokenByIDFileInternal(
         std::string sqlWhereClause = "1");
      DBCursor<FsckFsID>* getOrphanedDentryByIDFilesInternal(std::string sqlWhereClause = "1");
      DBCursor<FsckDirInode>* getInodesWithWrongOwnerInternal(std::string sqlWhereClause = "1");
      DBCursor<FsckDirInode>* getOrphanedDirInodesInternal(std::string sqlWhereClause = "1");
      DBCursor<FsckFileInode>* getOrphanedFileInodesInternal(std::string sqlWhereClause = "1");
      DBCursor<FsckChunk>* getOrphanedChunksInternal(std::string sqlWhereClause = "1",
         bool sortedByID = false);
      DBCursor<FsckDirInode>* getInodesWithoutContDirInternal(std::string sqlWhereClause = "1");
      DBCursor<FsckContDir>* getOrphanedContDirsInternal(std::string sqlWhereClause = "1");
      DBCursor<FsckFileInode>* getInodesWithWrongFileAttribsInternal(
         std::string sqlWhereClause ="1");
      DBCursor<FsckDirInode>* getInodesWithWrongDirAttribsInternal(
         std::string sqlWhereClause ="1");
      DBCursor<FsckTargetID>* getMissingStripeTargetsInternal(std::string sqlWhereClause = "1");
      DBCursor<FsckDirEntry>* getFilesWithMissingStripeTargetsInternal(
         std::string sqlWhereClause = "1");
      DBCursor<FsckChunk>* getMissingMirrorChunksInternal(std::string sqlWhereClause = "1",
         bool sortedByID = false);
      DBCursor<FsckChunk>* getMissingPrimaryChunksInternal(std::string sqlWhereClause = "1",
         bool sortedByID = false);
      DBCursor<FsckChunk>* getDifferingChunkAttribsInternal(std::string sqlWhereClause = "1",
         bool sortedByID = false);
      DBCursor<FsckChunk>* getChunksWithWrongPermissionsInternal(std::string sqlWhereClause = "1",
         bool sortedByID = false);
      DBCursor<FsckChunk>* getChunksInWrongPathInternal(std::string sqlWhereClause = "1",
         bool sortedByID = false);

      int64_t getRowCount(std::string tableName, std::string whereClause = "1");

      // FsckDBInsertContents.cpp
  /*    std::string buildInsertSql(std::string& tableName, StringMap& fields);
      std::string buildUpdateSql(std::string& tableName, StringMap& updateFields,
         StringMap& whereFields); */

      // FsckDBDeleteContents.cpp
      bool deleteDirEntries(std::string tableName, FsckDirEntryList& dentries,
         FsckDirEntry& outFailedDelete, int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteFsIDs(std::string tableName, FsckFsIDList& fsIDs,
         FsckFsID& outFailedDelete, int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteFileInodesByIDAndSaveNode(std::string tableName, FsckFileInodeList& inodes,
         FsckFileInode& outFailedDelete, int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteFileInodesByInternalID(std::string tableName, FsckFileInodeList& inodes,
         FsckFileInode& outFailedDelete, int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteDirInodesByIDAndSaveNode(std::string tableName, FsckDirInodeList& inodes,
         FsckDirInode& outFailedDelete, int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteChunksByIDAndTargetID(std::string tableName, FsckChunkList& chunks,
         FsckChunk& outFailedDelete, int& outErrorCode, DBHandle* dbHandle = NULL);
      bool deleteContDirsByIDAndSaveNode(std::string tableName, FsckContDirList& contDirs,
         FsckContDir& outFailedDelete, int& outErrorCode, DBHandle* dbHandle = NULL);

      // FsckDBErrorsAndRepair.cpp
      bool clearErrorTable(FsckErrorCode errorCode);

      bool addIgnoreErrorCode(std::string tableName, std::string whereClause,
         FsckErrorCode errorCode, DBHandle* dbHandle = NULL);
      bool addIgnoreAllErrorCodes(std::string tableName, std::string whereClause,
         DBHandle* dbHandle = NULL);

      // FsckDB.cpp
      int executeQuery(DBHandle *dbHandle, std::string queryStr, std::string* sqlErrorStr = NULL,
         bool logErrors = true);

   public:
      DBHandlePool* getHandlePool()
      {
         return this->dbHandlePool;
      }
};

#endif /* FSCKDB_H_ */
