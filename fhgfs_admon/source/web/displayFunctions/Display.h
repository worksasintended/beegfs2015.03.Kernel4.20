#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <common/net/message/storage/attribs/GetEntryInfoMsg.h>
#include <common/net/message/storage/attribs/GetEntryInfoRespMsg.h>
#include <common/net/message/storage/listing/ListDirFromOffsetMsg.h>
#include <common/net/message/storage/listing/ListDirFromOffsetRespMsg.h>
#include <common/net/message/storage/attribs/StatMsg.h>
#include <common/net/message/storage/attribs/StatRespMsg.h>
#include <common/net/message/admon/GetNodeInfoMsg.h>
#include <common/nodes/Node.h>
#include <common/toolkit/Time.h>
#include <common/toolkit/TimeAbs.h>
#include <common/toolkit/StringTk.h>
#include <common/toolkit/HighResolutionStats.h>
#include <common/toolkit/MetadataTk.h>
#include <common/toolkit/NodesTk.h>
#include <common/storage/StatData.h>
#include <components/worker/GetEntryInfoWork.h>
#include <database/Database.h>
#include <nodes/NodeStoreMetaEx.h>
#include <nodes/NodeStoreMgmtEx.h>
#include <nodes/NodeStoreStorageEx.h>
#include <nodes/MetaNodeEx.h>
#include <nodes/StorageNodeEx.h>
#include <nodes/MgmtNodeEx.h>


#define DISPLAY_FLOATING_AVERAGE_ORDER       31 //a odd value is required


struct FileEntry
{
   std::string name;
   StatData statData;
};

typedef std::list<FileEntry> FileEntryList;
typedef FileEntryList::iterator FileEntryListIter;
typedef FileEntryList::const_iterator FileEntryListConstIter;


class FileEntrySort
{
   public:
      FileEntrySort(std::string sortBy)
      {
         this->sortBy = sortBy;
      }

      bool operator()(const FileEntry &x, const FileEntry &y)
      {
         // first of all check if one of the args is a dir => dirs always first
         if(S_ISDIR(x.statData.getMode()) && !S_ISDIR(y.statData.getMode() ) )
            return true;
         else
         if(!S_ISDIR(x.statData.getMode()) && S_ISDIR(x.statData.getMode() ) )
            return false;

         if(sortBy == "filesize")
            return x.statData.getFileSize() < y.statData.getFileSize();
         else
         if(sortBy == "mode")
            return x.statData.getMode() < y.statData.getMode();
         else
         if(sortBy == "ctime")
            return x.statData.getCreationTimeSecs() < y.statData.getCreationTimeSecs();
         else
         if(sortBy == "atime")
            return x.statData.getLastAccessTimeSecs() < y.statData.getLastAccessTimeSecs();
         else
         if(sortBy == "mtime")
            return x.statData.getModificationTimeSecs() < y.statData.getModificationTimeSecs();
         else
         if(sortBy == "userid")
            return x.statData.getUserID() < y.statData.getUserID();
         else
         if(sortBy == "groupid")
            return x.statData.getGroupID() < y.statData.getGroupID();
         else
            return x.name < y.name;
      }


   private:
      std::string sortBy;
};

class Display
{
   public:
      Display(NodeStoreMetaEx *metaNodes, NodeStoreStorageEx *storageNodes,
         NodeStoreMgmtEx *mgmtdNodes)
      {
         this->storageNodes = storageNodes;
         this->metaNodes = metaNodes;
         this->mgmtdNodes = mgmtdNodes;

         log.setContext("Display");
      }


      std::string isRootMetaNode(uint16_t nodeID);
      std::string getRootMetaNode();
      uint16_t getRootMetaNodeNumID();
      std::string getRootMetaNodeIDWithTypeStr();
      std::string getRootMetaNodeAsTypedNodeID();
      bool statusMeta(uint16_t nodeID, std::string *outInfo);
      bool statusStorage(uint16_t nodeID, std::string *outInfo);
      bool statusMgmtd(uint16_t nodeID, std::string *outInfo);
      int countMetaNodes();
      int countStorageNodes();
      std::string diskSpaceTotalSum(std::string *outUnit);
      std::string diskSpaceFreeSum(std::string *outUnit);
      std::string diskSpaceUsedSum(std::string *outUnit);
      std::string timeSinceLastMessageMeta(uint16_t nodeID);
      std::string timeSinceLastMessageStorage(uint16_t nodeID);
      std::string diskRead(uint16_t nodeID, uint timespanM, std::string *outUnit);
      std::string diskWrite(uint16_t nodeID, uint timespanM, std::string *outUnit);
      std::string diskReadSum(uint timespanM, std::string *outUnit);
      std::string diskWriteSum(uint timespanM, std::string *outUnit);
      void diskPerfRead(uint16_t nodeID, uint timespanM, UInt64List *outListTime,
         UInt64List *outListReadPerSec, UInt64List *outListAverageTime,
         UInt64List *outListAverageReadPerSec, uint64_t startTimestampMS, uint64_t endTimestampMS);
      void diskPerfWrite(uint16_t nodeID, uint timespanM, UInt64List *outListTime,
         UInt64List *outListWritePerSec, UInt64List *outListAverageTime,
         UInt64List *outListAverageWritePerSec, uint64_t startTimestampMS, uint64_t endTimestampMS);
      void diskPerfReadSum(uint timespanM, UInt64List *outListTime, UInt64List *outListReadPerSec,
         UInt64List *outListAverageTime, UInt64List *outListAverageReadPerSec,
         uint64_t startTimestampMS, uint64_t endTimestampMS);
      void diskPerfWriteSum(uint timespanM, UInt64List *outListTime, UInt64List *outListWritePerSec,
         UInt64List *outListAverageTime, UInt64List *outListAverageWritePerSec,
         uint64_t startTimestampMS, uint64_t endTimestampMS);

      unsigned sessionCountMeta(uint16_t nodeID);
      unsigned sessionCountStorage(uint16_t nodeID);
      bool getEntryInfo(std::string pathStr, std::string *outChunkSize,
         std::string *outNumTargets, UInt16Vector *outPrimaryTargetNumIDs, unsigned* outPatternID,
         UInt16Vector *outSecondaryTargetNumIDs, UInt16Vector *outStorageBMGs,
         uint16_t* outMetaNodeNumID, uint16_t* outMetaMirrorNodeNumID);
      bool setPattern(std::string pathStr, uint chunkSize, uint defaultNumNodes,
         unsigned patternID, bool doMetaMirroring);
      int listDirFromOffset(std::string pathStr, int64_t *offset, FileEntryList *outEntries,
         uint64_t *tmpTotalSize, short count);
      std::string formatDateSec(uint64_t sec);
      std::string formatFileMode(int mode);
      bool getGeneralMetaNodeInfo(uint16_t nodeID, GeneralNodeInfo *outInfo);
      bool getGeneralStorageNodeInfo(uint16_t nodeID, GeneralNodeInfo *outInfo);
      NicAddressList getMetaNodeNicList(uint16_t nodeID);
      NicAddressList getStorageNodeNicList(uint16_t nodeID);
      void metaDataRequests(uint16_t nodeID, uint timespanM, StringList *outListTime,
         StringList *outListQueuedRequests, StringList *outListWorkRequests);
      void metaDataRequestsSum(uint timespanM, StringList *outListTime,
         StringList *outListQueuedRequests, StringList *outListWorkRequests);
      void storageTargetsInfo(uint16_t nodeID, StorageTargetInfoList *outStorageTargets);


   private:
      NodeStoreStorageEx *storageNodes;
      NodeStoreMetaEx *metaNodes;
      NodeStoreMgmtEx *mgmtdNodes;
      LogContext log;

      TabType selectTableType(uint timespanM)
      {
         uint64_t value;

         return selectTableTypeAndTimeRange(timespanM, &value);
      }

      TabType selectTableTypeAndTimeRange(uint timespanM, uint64_t* outValueTimeRange)
      {
         if (timespanM > 7200) // more than 5 days -> daily
         {
            *outValueTimeRange = 57600000; //16 hours before and after the time value
            return TABTYPE_Daily;
         }
         else
         if (timespanM > 1440) // more than 1 day
         {
            *outValueTimeRange = 2520000; //42 minutes before and after the time value
            return TABTYPE_Hourly;
         }
         else
         {
            *outValueTimeRange = 2000; //2 seconds before and after the time value
            return TABTYPE_Normal;
         }
      }

      /**
       * calculates a floating average with slightly changed time values
       *
       * @param order count of values which are used for the calculation of a average value, this
       * value must be a odd value
       */
      void floatingAverage(UInt64List *inList, UInt64List *inTimesList, UInt64List *outList,
         UInt64List *outTimesList, unsigned short order)
      {
         int halfOrder = order/2;

         // if order is smaller than 2 or if inList smaller than order it doesn't make any sense
         if ((order < 2) || (inList->size() < order))
         {
            return;
         }

         uint64_t sum = 0;
         UInt64ListIter addValueIter = inList->begin();
         UInt64ListIter subValueIter = inList->begin();
         UInt64ListIter timesIter = inTimesList->begin();

         // calculate the values which doesn't have the full order at the beginning of the chart
         for (int valueCount = 0; valueCount < order; addValueIter++, timesIter++)
         {
            //sum values for the first average value
            while(valueCount < halfOrder)
            {
               sum += *addValueIter;
               valueCount++;
               addValueIter++;
            }

            sum += *addValueIter;
            valueCount++;

            outList->push_back(sum / valueCount);
            outTimesList->push_back(*timesIter);
         }

         // calculate the values with the the full order
         for(; addValueIter != inList->end(); addValueIter++, subValueIter++, timesIter++)
         {
            sum += *addValueIter;
            sum -= *subValueIter;

            outList->push_back(sum / order);
            outTimesList->push_back(*timesIter);
         }

         // calculate the values which doesn't have the full order at the end of the chart
         for(int valueCount = order; timesIter != inTimesList->end(); subValueIter++, timesIter++)
         {
            sum -= *subValueIter;
            valueCount--;

            outList->push_back(sum / valueCount);
            outTimesList->push_back(*timesIter);
         }
      }
};

#endif /*DISPLAY_H_*/
