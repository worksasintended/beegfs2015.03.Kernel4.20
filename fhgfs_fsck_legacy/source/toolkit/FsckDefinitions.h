#ifndef FSCKDBTYPES_H_
#define FSCKDBTYPES_H_

#include <common/Common.h>
#include <common/storage/striping/StripePattern.h>
#include <common/storage/StorageDefinitions.h>

#define TABLE_NAME_DIR_ENTRIES "dirEntries";
#define TABLE_NAME_FILE_INODES "fileInodes";
#define TABLE_NAME_DIR_INODES "dirInodes";
#define TABLE_NAME_CHUNKS "chunks";
#define TABLE_NAME_STRIPE_PATTERNS "stripePatterns"
#define TABLE_NAME_CONT_DIRS "contDirs";
#define TABLE_NAME_FSIDS "fsIDs";
#define TABLE_NAME_USED_TARGETS "usedTargets";

#define TABLE_NAME_MODIFICATION_EVENTS "modificationEvents";

#define IGNORE_ERRORS_FIELD "ignoreErrors"

#define REPAIRACTION_DEF_DANGLINGFILEDENTRY FsckRepairAction_DELETEDENTRY
#define REPAIRACTION_DEF_DANGLINGDIRDENTRY FsckRepairAction_CREATEDEFAULTDIRINODE
#define REPAIRACTION_DEF_WRONGINODEOWNER FsckRepairAction_CORRECTOWNER
#define REPAIRACTION_DEF_WRONGOWNERINDENTRY FsckRepairAction_CORRECTOWNER
#define REPAIRACTION_DEF_ORPHANEDDIRINODE FsckRepairAction_LOSTANDFOUND
#define REPAIRACTION_DEF_ORPHANEDFILEINODE FsckRepairAction_DELETEINODE
#define REPAIRACTION_DEF_ORPHANEDCHUNK FsckRepairAction_DELETECHUNK
#define REPAIRACTION_DEF_MISSINGCONTDIR FsckRepairAction_CREATECONTDIR
#define REPAIRACTION_DEF_ORPHANEDCONTDIR FsckRepairAction_CREATEDEFAULTDIRINODE
#define REPAIRACTION_DEF_WRONGFILEATTRIBS FsckRepairAction_UPDATEATTRIBS
#define REPAIRACTION_DEF_WRONGDIRATTRIBS FsckRepairAction_UPDATEATTRIBS
#define REPAIRACTION_DEF_MISSINGTARGET FsckRepairAction_NOTHING
#define REPAIRACTION_DEF_FILEWITHMISSINGTARGET FsckRepairAction_NOTHING
#define REPAIRACTION_DEF_BROKENFSID FsckRepairAction_RECREATEFSID
#define REPAIRACTION_DEF_ORPHANEDFSID FsckRepairAction_RECREATEDENTRY
#define REPAIRACTION_DEF_CHUNKWITHWRONGPERM FsckRepairAction_FIXPERMISSIONS
#define REPAIRACTION_DEF_CHUNKINWRONGPATH FsckRepairAction_MOVECHUNK

enum FsckRepairAction
{
   FsckRepairAction_DELETEDENTRY = 0,
   FsckRepairAction_DELETEFILE = 1,
   FsckRepairAction_CREATEDEFAULTDIRINODE = 2,
   FsckRepairAction_CORRECTOWNER = 3,
   FsckRepairAction_LOSTANDFOUND = 4,
   FsckRepairAction_CREATECONTDIR = 5,
   FsckRepairAction_DELETEINODE = 7,
   FsckRepairAction_DELETECHUNK = 8,
   FsckRepairAction_DELETECONTDIR = 9,
   FsckRepairAction_UPDATEATTRIBS = 10,
   FsckRepairAction_CHANGETARGET = 11,
   FsckRepairAction_RECREATEFSID = 12,
   FsckRepairAction_RECREATEDENTRY = 13,
   FsckRepairAction_FIXPERMISSIONS = 14,
   FsckRepairAction_MOVECHUNK = 15,
   FsckRepairAction_NOTHING = 16,
   FsckRepairAction_UNDEFINED = 17
};

struct FsckRepairActionElem
{
   const char* actionShortDesc;
   const char* actionDesc;
   enum FsckRepairAction action;
};

// this must be suitable for the ignoreErrors field in the DB (which is saved as bitmap)
enum FsckErrorCode
{
   FsckErrorCode_DANGLINGDENTRY = 1,
   FsckErrorCode_WRONGOWNERINDENTRY = 1 << 1,
   FsckErrorCode_WRONGINODEOWNER = 1 << 2,
   FsckErrorCode_BROKENFSID = 1 << 3,
   FsckErrorCode_ORPHANEDFSID = 1 << 4,
   FsckErrorCode_ORPHANEDDIRINODE = 1 << 5,
   FsckErrorCode_ORPHANEDFILEINODE = 1 << 6,
   FsckErrorCode_ORPHANEDCHUNK = 1 << 7,
   FsckErrorCode_MISSINGCONTDIR = 1 << 8,
   FsckErrorCode_ORPHANEDCONTDIR = 1 << 9,
   FsckErrorCode_WRONGFILEATTRIBS = 1 << 10,
   FsckErrorCode_WRONGDIRATTRIBS = 1 << 11,
   FsckErrorCode_MISSINGTARGET = 1 << 12,
   FsckErrorCode_FILEWITHMISSINGTARGET = 1 << 13,
   FsckErrorCode_MISSINGMIRRORCHUNK = 1 << 14,
   FsckErrorCode_MISSINGPRIMARYCHUNK = 1 << 15,
   FsckErrorCode_DIFFERINGCHUNKATTRIBS = 1 << 16,
   FsckErrorCode_CHUNKWITHWRONGPERM = 1 << 17,
   FsckErrorCode_CHUNKINWRONGPATH = 1 << 18,
   FsckErrorCode_UNDEFINED = 1 << 19
};

struct FsckErrorCodeElem
{
   const char* errorShortDesc;
   const char* errorDesc;
   enum FsckErrorCode errorCode;
};

// Note: Keep in sync with enum FsckErrorCode
FsckErrorCodeElem const __FsckErrorCodes[] =
{
      {"DanglingDentry", "Dangling directory entry", FsckErrorCode_DANGLINGDENTRY},
      {"WrongOwnerInDentry", "Dentry points to inode on wrong node",
         FsckErrorCode_WRONGOWNERINDENTRY},
      {"WrongInodeOwner", "Wrong owner node saved in inode", FsckErrorCode_WRONGINODEOWNER},
      {"BrokenFsID", "Dentry-by-ID file is broken or missing", FsckErrorCode_BROKENFSID},
      {"OrphanedFsID", "Dentry-by-ID file is present, but no corresponding dentry",
         FsckErrorCode_ORPHANEDFSID},
      {"OrphanedDirInode", "Dir inode without a dentry pointing to it (orphaned inode)",
         FsckErrorCode_ORPHANEDDIRINODE},
      {"OrphanedFileInode", "File inode without a dentry pointing to it (orphaned inode)",
         FsckErrorCode_ORPHANEDFILEINODE},
      {"OrphanedChunk", "Chunk without an inode pointing to it (orphaned chunk)",
         FsckErrorCode_ORPHANEDCHUNK},
      {"MissingContDir", "Directory inode without a content directory",
         FsckErrorCode_MISSINGCONTDIR},
      {"OrphanedContDir", "Content directory without an inode", FsckErrorCode_ORPHANEDCONTDIR},
      {"WrongFileAttribs", "Attributes of file inode are wrong", FsckErrorCode_WRONGFILEATTRIBS},
      {"WrongDirAttribs", "Attributes of dir inode are wrong", FsckErrorCode_WRONGDIRATTRIBS},
      { "MissingTarget", "Target is used, but does not exist", FsckErrorCode_MISSINGTARGET },
      { "FileWithMissingTarget", "File has a missing target in stripe pattern",
         FsckErrorCode_FILEWITHMISSINGTARGET },
      { "MissingMirrorChunk", "Chunk should be mirrored, but mirrored data is not found",
         FsckErrorCode_MISSINGMIRRORCHUNK },
      { "MissingPrimaryChunk", "Mirrored data for a non-existent chunk was found",
         FsckErrorCode_MISSINGPRIMARYCHUNK },
      { "DifferingChunkAttribs", "Primary and mirrored data have different attributes",
         FsckErrorCode_DIFFERINGCHUNKATTRIBS },
      {"ChunkWithWrongPermissions", "Chunk has wrong permissions",
         FsckErrorCode_CHUNKWITHWRONGPERM },
      {"ChunkInWrongPath", "Chunk is saved in wrong path", FsckErrorCode_CHUNKINWRONGPATH },
      {"Unspecified", "Unspecified error", FsckErrorCode_UNDEFINED}
};

// Note: Keep in sync with enum FsckErrorCode
FsckRepairActionElem const __FsckRepairActions[] =
{
      {"DeleteDentry", "Delete directory entry", FsckRepairAction_DELETEDENTRY},
      {"DeleteFile", "Delete file", FsckRepairAction_DELETEFILE},
      {"CreateDefDirInode", "Create a directory inode with default values",
         FsckRepairAction_CREATEDEFAULTDIRINODE},
      {"CorrectOwner", "Correct owner node", FsckRepairAction_CORRECTOWNER},
      {"LostAndFound", "Link to lost+found", FsckRepairAction_LOSTANDFOUND},
      {"CreateContDir", "Create an empty content directory", FsckRepairAction_CREATECONTDIR},
      {"DeleteInode", "Delete inodes", FsckRepairAction_DELETEINODE},
      {"DeleteChunk", "Delete chunk", FsckRepairAction_DELETECHUNK},
      {"DeleteContDir", "Delete content directory", FsckRepairAction_DELETECONTDIR},
      {"UpdateAttribs", "Update attributes", FsckRepairAction_UPDATEATTRIBS},
      {"ChangeTarget", "Change target ID in stripe patterns", FsckRepairAction_CHANGETARGET},
      {"RecreateFsID", "Recreate dentry-by-id file", FsckRepairAction_RECREATEFSID},
      {"RecreateDentry", "Recreate directory entry file", FsckRepairAction_RECREATEDENTRY},
      {"FixPermissions", "Fix permissions", FsckRepairAction_FIXPERMISSIONS},
      {"MoveChunk", "Move chunk", FsckRepairAction_MOVECHUNK},
      {"Nothing", "Do nothing", FsckRepairAction_NOTHING}
};

#endif /* FSCKDBTYPES_H_ */
