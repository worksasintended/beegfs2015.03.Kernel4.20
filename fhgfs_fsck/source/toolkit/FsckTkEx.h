#ifndef FSCKTKEX_H_
#define FSCKTKEX_H_

#include <common/app/log/LogContext.h>
#include <common/fsck/FsckDirEntry.h>
#include <common/fsck/FsckChunk.h>
#include <common/fsck/FsckContDir.h>
#include <common/fsck/FsckDirEntry.h>
#include <common/fsck/FsckDirInode.h>
#include <common/fsck/FsckFileInode.h>
#include <common/fsck/FsckFsID.h>
#include <common/net/message/nodes/HeartbeatRequestMsg.h>
#include <common/net/message/nodes/HeartbeatMsg.h>
#include <common/nodes/NodeStore.h>
#include <common/threading/PThread.h>
#include <common/toolkit/NodesTk.h>
#include <common/toolkit/MessagingTk.h>
#include <database/DBHandle.h>
#include <toolkit/FsckDefinitions.h>

#include <sqlite3.h>

/* OutputOption Flags for fsckOutput */
#define OutputOptions_NONE                0
#define OutputOptions_LINEBREAK           1
#define OutputOptions_DOUBLELINEBREAK     1 << 1
#define OutputOptions_HEADLINE            1 << 2
#define OutputOptions_FLUSH               1 << 3
#define OutputOptions_ADDLINEBREAKBEFORE  1 << 4
#define OutputOptions_COLORGREEN          1 << 5
#define OutputOptions_COLORRED            1 << 6
#define OutputOptions_LINEDELETE          1 << 7
#define OutputOptions_NOSTDOUT            1 << 8
#define OutputOptions_NOLOG               1 << 9
#define OutputOptions_STDERR              1 << 10

#define OutputColor_NORMAL "\033[0m";
#define OutputColor_GREEN "\033[32m";
#define OutputColor_RED "\033[31m";

#define SAFE_FPRINTF(stream, fmt, args...) \
   do{ if(stream) {fprintf(stream, fmt, ##args);} } while(0)

/*
 * calculating with:
 * DirEntry    265 Byte
 * FileInode   410 Byte
 * DirInode    85 Byte
 * ContDir     30 Byte
 * chunk       60 Byte
 *
 */
#define NEEDED_DISKSPACE_META_INODE 675
#define NEEDED_DISKSPACE_STORAGE_INODE 60

#define TABLE_NAME_ERROR(errorCode) \
   ( std::string("error_") + FsckTkEx::getErrorDesc(errorCode, true) )

#define TABLE_NAME_REPAIR(repairAction) \
   ( std::string("repair_") + FsckTkEx::getRepairActionDesc(repairAction, true) )

class FsckTkEx
{
   public:
      // check the reachability of all nodes
      static bool checkReachability();
      // check the reachability of a given node by sending a heartbeat message
      static bool checkReachability(Node* node, NodeType nodetype);

      /*
       * a formatted output for fsck
       *
       * @param text The text to be printed
       * @param optionFlags OutputOptions_... flags (mainly for formatiing)
       */
      static void fsckOutput(std::string text, int options);
      // just print a formatted header with the version to the console
      static void printVersionHeader(bool toStdErr = false, bool noLogFile = false);
      // print the progress meter which goes round and round (-\|/-)
      static void progressMeter();

      static bool compareFsckDirEntryLists(FsckDirEntryList* listA, FsckDirEntryList* listB);
      static bool compareFsckFileInodeLists(FsckFileInodeList* listA, FsckFileInodeList* listB);
      static bool compareFsckDirInodeLists(FsckDirInodeList* listA, FsckDirInodeList* listB);
      static bool compareFsckChunkLists(FsckChunkList* listA, FsckChunkList* listB);
      static bool compareFsckContDirLists(FsckContDirList* listA, FsckContDirList* listB);
      static bool compareFsckFsIDLists(FsckFsIDList* listA, FsckFsIDList* listB);
      static bool compareUInt16Lists(UInt16List* listA, UInt16List* listB);

      static void removeFromList(FsckDirEntryList& list, FsckDirEntryList& removeList);
      static void removeFromList(FsckDirInodeList& list, FsckDirInodeList& removeList);
      static void removeFromList(FsckDirInodeList& list, StringList& removeList);
      static void removeFromList(FsckFileInodeList& list, FsckFileInodeList& removeList);
      static void removeFromList(FsckFileInodeList& list, StringList& removeList);
      static void removeFromList(FsckChunkList& list, FsckChunkList& removeList);
      static void removeFromList(FsckChunkList& list, StringList& removeList);
      static void removeFromList(StringList& list, StringList& removeList);

      static int64_t calcNeededSpace();
      static bool checkDiskSpace(Path& dbPath);

      static std::string getErrorDesc(FsckErrorCode errorCode, bool shortDesc = false);
      static std::string getRepairActionDesc(FsckRepairAction repairAction, bool shortDesc = false);

      static bool startModificationLogging(NodeStore* metaNodes, Node* localNode);
      static bool stopModificationLogging(NodeStore* metaNodes);

      static bool testVersions(NodeStore* metaNodes, NodeStore* storageNodes);

   private:
      FsckTkEx() {}

      // saves the last char output by the progress meter
      static char progressChar;
      // a mutex that is locked by output functions to make sure the output does not get messed up
      // by two threads doing output at the same time
      static Mutex outputMutex;


   public:
      // inliners
      static void fsckOutput(std::string text)
      {
         fsckOutput(text, OutputOptions_LINEBREAK);
      }

      /*
       * remove all elements from removeList in list by ID
       */
      template<typename T>
      static void removeFromListByID(std::list<T>& list, StringList removeList)
      {
         for ( StringListIter removeIter = removeList.begin(); removeIter != removeList.end();
            removeIter++ )
         {
            typename std::list<T>::iterator iter;
            for ( iter = list.begin(); iter != list.end(); iter++ )
            {
               if ( removeIter->compare(iter->getID()) == 0 )
               {
                  list.erase(iter);
                  break;
               }
            }
         }
      }
};

#endif /* FSCKTKEX_H_ */
