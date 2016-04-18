#ifndef DATABASETK_H_
#define DATABASETK_H_

#include <common/fsck/FsckChunk.h>
#include <common/fsck/FsckContDir.h>
#include <common/fsck/FsckDirEntry.h>
#include <common/fsck/FsckDirInode.h>
#include <common/fsck/FsckFileInode.h>
#include <common/fsck/FsckFsID.h>
#include <common/fsck/FsckModificationEvent.h>
#include <common/fsck/FsckTargetID.h>

#include <sqlite3.h>

class FsckDB;

class DatabaseTk
{
   public:
      DatabaseTk();
      virtual ~DatabaseTk();

      static FsckDirEntry createDummyFsckDirEntry(
         FsckDirEntryType entryType = FsckDirEntryType_REGULARFILE);
      static void createDummyFsckDirEntries(uint amount, FsckDirEntryList* outList,
         FsckDirEntryType entryType = FsckDirEntryType_REGULARFILE);
      static void createDummyFsckDirEntries(uint from, uint amount, FsckDirEntryList* outList,
         FsckDirEntryType entryType = FsckDirEntryType_REGULARFILE);


      static FsckFileInode createDummyFsckFileInode();
      static void createDummyFsckFileInodes(uint amount, FsckFileInodeList* outList);
      static void createDummyFsckFileInodes(uint from, uint amount, FsckFileInodeList* outList);

      static FsckDirInode createDummyFsckDirInode();
      static void createDummyFsckDirInodes(uint amount, FsckDirInodeList* outList);
      static void createDummyFsckDirInodes(uint from, uint amount, FsckDirInodeList* outList);

      static FsckChunk createDummyFsckChunk();
      static void createDummyFsckChunks(uint amount, FsckChunkList* outList);
      static void createDummyFsckChunks(uint amount, uint numTargets, FsckChunkList* outList);
      static void createDummyFsckChunks(uint from, uint amount, uint numTargets,
         FsckChunkList* outList);

      static FsckContDir createDummyFsckContDir();
      static void createDummyFsckContDirs(uint amount, FsckContDirList* outList);
      static void createDummyFsckContDirs(uint from, uint amount, FsckContDirList* outList);

      static FsckFsID createDummyFsckFsID();
      static void createDummyFsckFsIDs(uint amount, FsckFsIDList* outList);
      static void createDummyFsckFsIDs(uint from, uint amount, FsckFsIDList* outList);

      static void createFsckDirEntryFromRow(sqlite3_stmt* stmt, FsckDirEntry *outEntry,
         unsigned colShift=0);
      static void createFsckFileInodeFromRow(sqlite3_stmt* stmt, FsckFileInode *outInode,
         unsigned colShift=0);
      static void createFsckDirInodeFromRow(sqlite3_stmt* stmt, FsckDirInode *outInode,
         unsigned colShift=0);
      static void createFsckChunkFromRow(sqlite3_stmt* stmt, FsckChunk *outChunk,
         unsigned colShift=0);
      static void createFsckContDirFromRow(sqlite3_stmt* stmt, FsckContDir *outContDir,
         unsigned colShift=0);
      static void createFsckFsIDFromRow(sqlite3_stmt* stmt, FsckFsID *outFsID,
         unsigned colShift=0);
      static void createFsckModificationEventFromRow(sqlite3_stmt* stmt,
         FsckModificationEvent *outModificationEvent, unsigned colShift=0);
      static void createFsckTargetIDFromRow(sqlite3_stmt* stmt, FsckTargetID *outTargetID,
         unsigned colShift);

      static void createObjectFromRow(sqlite3_stmt* stmt, FsckDirEntry* outObject,
         unsigned colShift=0);
      static void createObjectFromRow(sqlite3_stmt* stmt, FsckFileInode* outObject,
         unsigned colShift=0);
      static void createObjectFromRow(sqlite3_stmt* stmt, FsckDirInode* outObject,
         unsigned colShift=0);
      static void createObjectFromRow(sqlite3_stmt* stmt, FsckChunk* outObject,
         unsigned colShift=0);
      static void createObjectFromRow(sqlite3_stmt* stmt, FsckContDir* outObject,
         unsigned colShift=0);
      static void createObjectFromRow(sqlite3_stmt* stmt, FsckFsID* outObject,
         unsigned colShift=0);
      static void createObjectFromRow(sqlite3_stmt* stmt, FsckModificationEvent* outObject,
         unsigned colShift=0);
      static void createObjectFromRow(sqlite3_stmt* stmt, FsckTargetID* outObject,
         unsigned colShift=0);

      static bool getFullPath(FsckDB* db, FsckDirEntry* dirEntry, std::string& outPath);
      static bool getFullPath(FsckDB* db, std::string id, std::string& outPath);

      static std::string calculateExpectedChunkPath(std::string entryID, unsigned origParentUID,
         std::string origParentEntryID, int pathInfoFlags);
};

#endif /* DATABASETK_H_ */
