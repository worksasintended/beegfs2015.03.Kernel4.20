#ifndef MODECHECKFS_H
#define MODECHECKFS_H

#include <app/config/Config.h>
#include <database/FsckDB.h>
#include <database/FsckDBException.h>
#include <modes/Mode.h>


#define MAX_DELETE_DENTRIES 500
#define MAX_CREATE_DIR_INODES 500
#define MAX_CORRECT_OWNER 500
#define MAX_LOST_AND_FOUND 500
#define MAX_DELETE_CHUNK 500
#define MAX_CREATE_CONT_DIR 500
#define MAX_DELETE_DIR_INODE 500
#define MAX_DELETE_FILE_INODE 500
#define MAX_UPDATE_FILE_ATTRIBS 500
#define MAX_CHANGE_STRIPE_TARGET 4
#define MAX_DELETE_FILES 500
#define MAX_RECREATEFSIDS 500
#define MAX_RECREATEDENTRIES 500
#define MAX_FIXPERMISSIONS 500
#define MAX_MOVECHUNKS 500

#define MAX_CHECK_CYCLES 3

class ModeCheckFS : public Mode
{
   public:
      ModeCheckFS();
      virtual ~ModeCheckFS();

      static void printHelp();

      virtual int execute();

   private:
      FsckDB* database;
      std::map<FsckErrorCode, FsckRepairAction> repairActions;

      LogContext log;

      int initDatabase();
      void printHeaderInformation();
      bool gatherData();

      FsckRepairAction chooseAction(FsckErrorCode errorCode,
         std::vector<FsckRepairAction> possibleActions, bool& outAskNextTime);

      int64_t checkAndRepairDanglingDentry();
      int64_t checkAndRepairWrongInodeOwner();
      int64_t checkAndRepairWrongOwnerInDentry();
      int64_t checkAndRepairOrphanedContDir();
      int64_t checkAndRepairOrphanedDirInode();
      int64_t checkAndRepairOrphanedFileInode();
      int64_t checkAndRepairOrphanedChunk();
      int64_t checkAndRepairMissingContDir();
      int64_t checkAndRepairWrongFileAttribs();
      int64_t checkAndRepairWrongDirAttribs();
      int64_t checkAndRepairMissingTargets();
      int64_t checkAndRepairFilesWithMissingTargets();
      int64_t checkAndRepairDirEntriesWithBrokeByIDFile();
      int64_t checkAndRepairOrphanedDentryByIDFiles();
      int64_t checkAndRepairChunksWithWrongPermissions();
      int64_t checkMissingMirrorChunks();
      int64_t checkMissingPrimaryChunks();
      int64_t checkDifferingChunkAttribs();
      int64_t checkAndRepairChunksInWrongPath();
      void checkAndRepair();

      void repairDanglingDirEntries();
      void repairWrongInodeOwners();
      void repairWrongInodeOwnersInDentry();
      void repairOrphanedDirInodes();
      void repairOrphanedFileInodes();
      void repairOrphanedChunks();
      void repairMissingContDirs();
      void repairOrphanedContDirs();
      void repairWrongFileAttribs();
      void repairWrongDirAttribs();
      void repairMissingTargets();
      void repairFilesWithMissingTargets();
      void repairDirEntriesWithBrokenByIDFile();
      void repairOrphanedDentryByIDFiles();
      void repairChunksWithWrongPermissions();
      void repairWrongChunkPaths();

      void setCorrectInodeOwnersInDentry(FsckDirEntryList& dentries);

      void correctAndDeleteWrongOwnerInodesFromDB(FsckDirInodeList& inodes, DBHandle* dbHandle);
      void correctAndDeleteWrongOwnerEntriesFromDB(FsckDirEntryList& dentries, DBHandle* dbHandle);
      void setUpdatedFileAttribs(FsckFileInodeList& inodes);
      void setUpdatedDirAttribs(FsckDirInodeList& inodes);

      void deleteDirEntriesFromDB(FsckDirEntryList& dentries, DBHandle* dbHandle);
      void deleteFsIDsFromDB(FsckDirEntryList& dentries, DBHandle* dbHandle);
      void deleteFileInodesFromDB(FsckFileInodeList& fileInodes, DBHandle* dbHandle);
      void deleteDirInodesFromDB(FsckDirInodeList& dirInodes, DBHandle* dbHandle);
      void deleteChunksFromDB(FsckChunkList& chunks, DBHandle* dbHandle);
      void deleteFilesFromDB(FsckDirEntryList& dentries, DBHandle* dbHandle);

      void deleteDanglingDirEntriesFromDB(FsckDirEntryList& dentries, DBHandle* dbHandle);
      void deleteOrphanedDirInodesFromDB(FsckDirInodeList& inodes, DBHandle* dbHandle);
      void deleteOrphanedFileInodesFromDB(FsckFileInodeList& inodes, DBHandle* dbHandle);
      void deleteOrphanedChunksFromDB(FsckChunkList& chunks, DBHandle* dbHandle);
      void deleteMissingContDirsFromDB(FsckDirInodeList& inodes, DBHandle* dbHandle);
      void deleteOrphanedContDirsFromDB(FsckContDirList& contDirs, DBHandle* dbHandle);
      void deleteFileInodesWithWrongAttribsFromDB(FsckFileInodeList& inodes, DBHandle* dbHandle);
      void deleteDirInodesWithWrongAttribsFromDB(FsckDirInodeList& inodes, DBHandle* dbHandle);
      void deleteFilesWithMissingTargetFromDB(FsckDirEntryList& dentries, DBHandle* dbHandle);
      void deleteDirEntriesWithBrokenByIDFileFromDB(FsckDirEntryList& dentries, DBHandle* dbHandle);
      void deleteOrphanedDentryByIDFilesFromDB(FsckFsIDList& fsIDs, DBHandle* dbHandle);
      void deleteChunksWithWrongPermissionsFromDB(FsckChunkList& chunks, DBHandle* dbHandle);
      void deleteChunksInWrongPathFromDB(FsckChunkList& chunks, DBHandle* dbHandle);

      void insertCreatedDirInodesToDB(FsckDirInodeList& dirInodes, DBHandle* dbHandle);
      void insertCreatedFileInodesToDB(FsckFileInodeList& fileInodes, DBHandle* dbHandle);
      void insertCreatedDirEntriesToDB(FsckDirEntryList& dirEntries, DBHandle* dbHandle);
      void insertCreatedFsIDsToDB(FsckFsIDList& fsIDs, DBHandle* dbHandle);
      void insertCreatedContDirsToDB(FsckDirInodeList& dirInodes, DBHandle* dbHandle);

      void updateFileInodesInDB(FsckFileInodeList& inodes, DBHandle* dbHandle);
      void updateDirInodesInDB(FsckDirInodeList& inodes, DBHandle* dbHandle);
      void updateFsIDsInDB(FsckDirEntryList& dirEntries, DBHandle* dbHandle);
      void updateChunksInDB(FsckChunkList& chunks, DBHandle* dbHandle);
};

#endif /* MODECHECKFS_H */
