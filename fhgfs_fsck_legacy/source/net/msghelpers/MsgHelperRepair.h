#ifndef MSGHELPERREPAIR_H_
#define MSGHELPERREPAIR_H_

#include <common/Common.h>

class MsgHelperRepair
{
   public:
      static void deleteDanglingDirEntries(Node* node, FsckDirEntryList* dentries,
         FsckDirEntryList* failedDeletes);

      static void createDefDirInodes(Node* node, StringList* inodeIDs,
         FsckDirInodeList* createdInodes);

      static void correctInodeOwnersInDentry(Node* node, FsckDirEntryList* dentries,
         FsckDirEntryList* failedCorrections);
      static void correctInodeOwners(Node* node, FsckDirInodeList* dirInodes,
         FsckDirInodeList* failedCorrections);

      static void deleteFiles(Node* node, FsckDirEntryList* dentries,
         FsckDirEntryList* failedDeletes);
      static void deleteChunks(Node* node, FsckChunkList* chunks, FsckChunkList* failedDeletes);

      static Node* referenceLostAndFoundOwner(EntryInfo* outLostAndFoundEntryInfo);
      static bool createLostAndFound(Node** outReferencedNode, EntryInfo& outLostAndFoundEntryInfo);
      static void linkToLostAndFound(Node* lostAndFoundNode, EntryInfo* lostAndFoundInfo,
         FsckDirInodeList* dirInodes, FsckDirInodeList* failedInodes,
         FsckDirEntryList* createdDentries);
      static void linkToLostAndFound(Node* lostAndFoundNode, EntryInfo* lostAndFoundInfo,
         FsckFileInodeList* fileInodes, FsckFileInodeList* failedInodes,
         FsckDirEntryList* createdDentries);
      static void createContDirs(Node* node, FsckDirInodeList* inodes, StringList* failedCreates);
      static void updateFileAttribs(Node* node, FsckFileInodeList* inodes,
         FsckFileInodeList* failedUpdates);
      static void updateDirAttribs(Node* node, FsckDirInodeList* inodes,
         FsckDirInodeList* failedUpdates);
      static void changeStripeTarget(Node* node, FsckFileInodeList* inodes, uint16_t targetID,
         uint16_t newTargetID, FsckFileInodeList* failedInodes);
      static void recreateFsIDs(Node* node,  FsckDirEntryList* dentries,
         FsckDirEntryList* failedEntries);
      static void recreateDentries(Node* node, FsckFsIDList* fsIDs, FsckFsIDList* failedCreates,
         FsckDirEntryList* createdDentries, FsckFileInodeList* createdInodes);
      static void fixChunkPermissions(Node *node, FsckChunkList& chunkList,
         PathInfoList& pathInfoList, FsckChunkList& failedChunks);
      static void moveChunks(Node* node, FsckChunkList& chunkList, StringList& moveToList,
         FsckChunkList& failedChunks);
      static void deleteFileInodes(Node* node, FsckFileInodeList& inodes,
         StringList& failedDeletes);


   private:
      MsgHelperRepair() {}

   public:
      // inliners
};

#endif /* MSGHELPERREPAIR_H_ */
