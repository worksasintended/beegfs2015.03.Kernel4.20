#include "Display.h"

#include <common/net/message/storage/attribs/SetDirPatternMsg.h>
#include <common/net/message/storage/attribs/SetDirPatternRespMsg.h>
#include <common/net/message/storage/mirroring/SetMetadataMirroringMsg.h>
#include <common/net/message/storage/mirroring/SetMetadataMirroringRespMsg.h>
#include <common/storage/striping/BuddyMirrorPattern.h>
#include <common/storage/striping/Raid0Pattern.h>
#include <common/storage/striping/Raid10Pattern.h>
#include <common/storage/striping/StripePattern.h>
#include <common/toolkit/UnitTk.h>
#include <program/Program.h>

std::string Display::isRootMetaNode(uint16_t nodeID)
{
   std::string res = std::string("No");
   MetaNodeEx *node = (MetaNodeEx*) metaNodes->referenceNode(nodeID);
   if (node != NULL)
   {
      if (node->getContent().isRoot)
      {
         res = std::string("Yes");
      }

      Node *relNode = node;
      metaNodes->releaseNode(&relNode);
   }
   else
   {
      res = std::string("Node not available!");
   }
   return res;
}

uint16_t Display::getRootMetaNodeNumID()
{
   return metaNodes->getRootNodeNumID();
}

std::string Display::getRootMetaNode()
{
   Node* node = metaNodes->referenceNode(metaNodes->getRootNodeNumID());
   std::string retval;

   if (node != NULL)
   {
      retval = node->getID();
      metaNodes->releaseNode(&node);
   }
   else
   {
      retval = "";
   }

   return retval;
}

std::string Display::getRootMetaNodeIDWithTypeStr()
{
   Node* node = metaNodes->referenceNode(metaNodes->getRootNodeNumID());
   std::string retval;

   if (node != NULL)
   {
      retval = node->getNodeIDWithTypeStr();
      metaNodes->releaseNode(&node);
   }
   else
   {
      retval = "";
   }

   return retval;
}

std::string Display::getRootMetaNodeAsTypedNodeID()
{
   Node* node = metaNodes->referenceNode(metaNodes->getRootNodeNumID());
   std::string retval;

   if (node != NULL)
   {
      retval = node->getTypedNodeID();
      metaNodes->releaseNode(&node);
   }
   else
   {
      retval = "";
   }

   return retval;
}

bool Display::statusMeta(uint16_t nodeNumID, std::string *outInfo)
{
   bool res = true;
   *outInfo = "Up";
   MetaNodeEx *node = (MetaNodeEx*) metaNodes->referenceNode(nodeNumID);

   if (node != NULL)
   {
      if (!node->getContent().isResponding)
      {
         res = false;
         *outInfo = "Down";
      }

      Node *relNode = node;
      metaNodes->releaseNode(&relNode);
   }
   else
   {
      res = false;
      *outInfo = "Node not available!";
   }

   return res;
}

bool Display::statusStorage(uint16_t nodeID, std::string *outInfo)
{
   bool res = true;
   *outInfo = "Up";
   StorageNodeEx *node = (StorageNodeEx*) storageNodes->referenceNode(nodeID);

   if (node != NULL)
   {
      if (!node->getContent().isResponding)
      {
         res = false;
         *outInfo = "Down";
      }

      Node *relNode = node;
      storageNodes->releaseNode(&relNode);
   }
   else
   {
      res = false;
      *outInfo = "Node not available!";
   }

   return res;
}

bool Display::statusMgmtd(uint16_t nodeID, std::string *outInfo)
{
   bool res = true;
   *outInfo = "Up";
   MgmtNodeEx *node = (MgmtNodeEx*) mgmtdNodes->referenceNode(nodeID);

   if (node != NULL)
   {
      if (!node->getContent().isResponding)
      {
         res = false;
         *outInfo = "Down";
      }

      Node *relNode = node;
      mgmtdNodes->releaseNode(&relNode);
   }
   else
   {
      res = false;
      *outInfo = "Node not available!";
   }

   return res;
}

int Display::countMetaNodes()
{
   return metaNodes->getSize();
}

int Display::countStorageNodes()
{
   return storageNodes->getSize();
}

std::string Display::diskSpaceTotalSum(std::string *outUnit)
{
   int64_t space = 0;
   StorageNodeEx *node = (StorageNodeEx*) storageNodes->referenceFirstNode();

   while (node != NULL)
   {
      uint16_t nodeID = node->getNumID();
      space += node->getContent().diskSpaceTotal;

      Node *relNode = node;
      storageNodes->releaseNode(&relNode);

      node = (StorageNodeEx*) storageNodes->referenceNextNode(nodeID);
   }

   return StringTk::doubleToStr(UnitTk::mebibyteToXbyte(space, outUnit));
}

std::string Display::diskSpaceFreeSum(std::string *outUnit)
{
   int64_t space = 0;
   StorageNodeEx *node = (StorageNodeEx*) storageNodes->referenceFirstNode();

   while (node != NULL)
   {
      uint16_t nodeID = node->getNumID();
      space += node->getContent().diskSpaceFree;

      Node *relNode = node;
      storageNodes->releaseNode(&relNode);

      node = (StorageNodeEx*) storageNodes->referenceNextNode(nodeID);
   }

   return StringTk::doubleToStr(UnitTk::mebibyteToXbyte(space, outUnit));
}

std::string Display::diskSpaceUsedSum(std::string *outUnit)
{
   int64_t totalSpace = 0;
   int64_t freeSpace = 0;

   StorageNodeEx *node = (StorageNodeEx*) storageNodes->referenceFirstNode();

   while (node != NULL)
   {
      uint16_t nodeID = node->getNumID();
      totalSpace += node->getContent().diskSpaceTotal;
      freeSpace += node->getContent().diskSpaceFree;

      Node *relNode = node;
      storageNodes->releaseNode(&relNode);

      node = (StorageNodeEx*) storageNodes->referenceNextNode(nodeID);
   }

   int64_t space = totalSpace - freeSpace;

   return StringTk::doubleToStr(UnitTk::mebibyteToXbyte(space, outUnit));
}

void Display::diskPerfRead(uint16_t nodeID, uint timespanM, UInt64List *outListTime,
   UInt64List *outListReadPerSec, UInt64List *outListAverageTime,
   UInt64List *outListAverageReadPerSec, uint64_t startTimestampMS, uint64_t endTimestampMS)
{
   HighResStatsList data;
   StorageNodeEx *node = (StorageNodeEx*) storageNodes->referenceNode(nodeID);

   if (node != NULL)
   {
      data = node->getHighResData();

      Node *relNode = node;
      storageNodes->releaseNode(&relNode);
   }

   // timeSpan in minutes, 10 minutes are stored server side, if longer span is
   // requested take averaged values from db
   if ((timespanM <= 10) && (!data.empty()))
   {
      for (HighResStatsListIter iter = data.begin(); iter != data.end(); iter++)
      {
         HighResolutionStats content = *iter;
         uint64_t contentTimeMS = content.rawVals.statsTimeMS;

         if ((contentTimeMS > startTimestampMS) && (contentTimeMS < endTimestampMS) )
         {
            outListTime->push_back(content.rawVals.statsTimeMS);
            double diskReadMiB = UnitTk::byteToMebibyte(content.incVals.diskReadBytes);
            outListReadPerSec->push_back(uint64_t(diskReadMiB));
         }
      }
   }
   else //take db data
   {
      TabType tabType = selectTableType(timespanM);

      Database *db = Program::getApp()->getDB();
      StorageNodeDataContentList data;

      db->getStorageNodeSets(nodeID, tabType, (startTimestampMS / 1000), (endTimestampMS / 1000),
         &data);

      for (StorageNodeDataContentListIter iter = data.begin(); iter != data.end(); iter++)
      {
         StorageNodeDataContent content = *iter;
         outListTime->push_back(content.time * 1000);
         outListReadPerSec->push_back(content.diskReadPerSec);
      }
   }

   floatingAverage(outListReadPerSec, outListTime, outListAverageReadPerSec,
      outListAverageTime, DISPLAY_FLOATING_AVERAGE_ORDER);
}

void Display::diskPerfWrite(uint16_t nodeID, uint timespanM, UInt64List *outListTime,
   UInt64List *outListWritePerSec, UInt64List *outListAverageTime,
   UInt64List *outListAverageWritePerSec, uint64_t startTimestampMS, uint64_t endTimestampMS)
{
   HighResStatsList data;
   StorageNodeEx *node = (StorageNodeEx*) storageNodes->referenceNode(nodeID);
   if (node != NULL)
   {
      data = node->getHighResData();

      Node *relNode = node;
      storageNodes->releaseNode(&relNode);
   }

   // timeSpan in minutes, 10 minutes are stored server side, if longer span is
   // requested take averaged values from db
   if ((timespanM <= 10) && (!data.empty()))
   {
      for (HighResStatsListIter iter = data.begin(); iter != data.end(); iter++)
      {
         HighResolutionStats content = *iter;
         uint64_t contentTimeMS = content.rawVals.statsTimeMS;

         if ((contentTimeMS > startTimestampMS) && (contentTimeMS < endTimestampMS) )
         {
            outListTime->push_back(content.rawVals.statsTimeMS);
            double diskWriteMiB = UnitTk::byteToMebibyte(content.incVals.diskWriteBytes);
            outListWritePerSec->push_back(uint64_t(diskWriteMiB));
         }
      }
   }
   else //take db data
   {
      TabType tabType = selectTableType(timespanM);

      Database *db = Program::getApp()->getDB();
      StorageNodeDataContentList data;

      db->getStorageNodeSets(nodeID, tabType, (startTimestampMS / 1000), (endTimestampMS / 1000),
         &data);

      for (StorageNodeDataContentListIter iter = data.begin(); iter != data.end(); iter++)
      {
         StorageNodeDataContent content = *iter;
         outListTime->push_back(content.time * 1000);
         outListWritePerSec->push_back(content.diskWritePerSec);
      }
   }
   // calculate averages, it's important to use same order values here,
   // otherwise the chart will be screwed
   floatingAverage(outListWritePerSec, outListTime, outListAverageWritePerSec,
      outListAverageTime, DISPLAY_FLOATING_AVERAGE_ORDER);
}

std::string Display::timeSinceLastMessageMeta(uint16_t nodeID)
{
   MetaNodeEx *node = (MetaNodeEx*) metaNodes->referenceNode(nodeID);
   if (node != NULL)
   {
      Time time;
      Time oldTime = node->getLastHeartbeatT();

      Node *relNode = node;
      metaNodes->releaseNode(&relNode);

      return StringTk::intToStr(time.elapsedSinceMS(&oldTime) / 1000) + std::string(" seconds");
   }
   else
   {
      return "Node not available!";
   }
}

std::string Display::timeSinceLastMessageStorage(uint16_t nodeID)
{
   StorageNodeEx *node = (StorageNodeEx*) storageNodes->referenceNode(nodeID);
   if (node != NULL)
   {
      Time time;
      Time oldTime = node->getLastHeartbeatT();

      Node *relNode = node;
      storageNodes->releaseNode(&relNode);

      return StringTk::intToStr(time.elapsedSinceMS(&oldTime) / 1000) + std::string(" seconds");
   }
   else
   {
      return "Node not available!";
   }
}

std::string Display::diskRead(uint16_t nodeID, uint timespanM, std::string *outUnit)
{
   double sum = 0; // sum in MiB
   if (timespanM <= 10)
   {
      StorageNodeEx *node = (StorageNodeEx*) storageNodes->referenceNode(nodeID);
      if (node != NULL)
      {
         std::vector<HighResStatsList> dataSets;
         HighResStatsList data = node->getHighResData();

         Node *relNode = node;
         storageNodes->releaseNode(&relNode);

         for (HighResStatsListIter iter = data.begin(); iter != data.end(); iter++)
         {
            HighResolutionStats s = *iter;
            sum += UnitTk::byteToMebibyte(s.incVals.diskReadBytes);
         }
      }
   }
   else
   {
      TabType tabType = selectTableType(timespanM);

      TimeAbs t;
      t.setToNow();
      long timeNow = (t.getTimeval()->tv_sec);
      uint64_t startTime = t.getTimeMS() - (timespanM * 60 * 1000);

      StorageNodeDataContentList data;

      Database *db = Program::getApp()->getDB();
      db->getStorageNodeSets(nodeID, tabType, (startTime / 1000), timeNow, &data);

      for (StorageNodeDataContentListIter iter = data.begin(); iter != data.end(); iter++)
      {
         StorageNodeDataContent content = *iter;
         sum += content.diskRead; // db stores in MiB;
      }
   }

   return StringTk::doubleToStr(UnitTk::mebibyteToXbyte(int64_t(sum), outUnit));
}

std::string Display::diskWrite(uint16_t nodeID, uint timespanM, std::string *outUnit)
{
   double sum = 0; // sum in MiB
   if (timespanM <= 10)
   {
      StorageNodeEx *node = (StorageNodeEx*) storageNodes->referenceNode(nodeID);
      if (node != NULL)
      {
         std::vector<HighResStatsList> dataSets;
         HighResStatsList data = node->getHighResData();

         Node *relNode = node;
         storageNodes->releaseNode(&relNode);

         for (HighResStatsListIter iter = data.begin(); iter != data.end(); iter++)
         {
            HighResolutionStats s = *iter;
            sum += UnitTk::byteToMebibyte(s.incVals.diskWriteBytes);
         }
      }
   }
   else
   {
      TabType tabType = selectTableType(timespanM);

      TimeAbs t;
      t.setToNow();
      long timeNow = (t.getTimeval()->tv_sec);
      uint64_t startTime = t.getTimeMS() - (timespanM * 60 * 1000);

      StorageNodeDataContentList data;

      Database *db = Program::getApp()->getDB();
      db->getStorageNodeSets(nodeID, tabType, (startTime / 1000), timeNow, &data);

      for (StorageNodeDataContentListIter iter = data.begin(); iter != data.end(); iter++)
      {
         StorageNodeDataContent content = *iter;
         sum += content.diskWrite; // db stores in MiB;
      }
   }

   return StringTk::doubleToStr(UnitTk::mebibyteToXbyte(int64_t(sum), outUnit));
}

std::string Display::diskReadSum(uint timespanM, std::string *outUnit)
{
   double sum = 0; // sum in MiB
   if (timespanM <= 10)
   {
      StorageNodeEx *node = (StorageNodeEx*) storageNodes->referenceFirstNode();

      while (node != NULL)
      {
         std::vector<HighResStatsList> dataSets;
         HighResStatsList data = node->getHighResData();
         uint16_t nodeID = node->getNumID();

         Node *relNode = node;
         storageNodes->releaseNode(&relNode);

         for (HighResStatsListIter iter = data.begin(); iter != data.end(); iter++)
         {
            HighResolutionStats s = *iter;
            sum += UnitTk::byteToMebibyte(s.incVals.diskReadBytes);
         }
         node = (StorageNodeEx*) storageNodes->referenceNextNode(nodeID);
      }
   }
   else
   {
      TabType tabType = selectTableType(timespanM);

      UInt16List nodes;
      Database *db = Program::getApp()->getDB();
      db->getStorageNodes(&nodes);

      TimeAbs t;
      t.setToNow();
      long timeNow = (t.getTimeval()->tv_sec);
      uint64_t startTime = t.getTimeMS() - (timespanM * 60 * 1000);

      for (UInt16ListIter iter = nodes.begin(); iter != nodes.end(); iter++)
      {
         StorageNodeDataContentList data;
         uint16_t nodeID = *iter;
         db->getStorageNodeSets(nodeID, tabType, (startTime / 1000), timeNow, &data);

         for (StorageNodeDataContentListIter contIter = data.begin(); contIter != data.end();
            contIter++)
         {
            StorageNodeDataContent content = *contIter;
            sum += content.diskRead; // db stores in MiB;
         }
      }
   }

   return StringTk::doubleToStr(UnitTk::mebibyteToXbyte(int64_t(sum), outUnit));
}

std::string Display::diskWriteSum(uint timespanM, std::string *outUnit)
{
   double sum = 0; // sum in MiB
   if (timespanM <= 10)
   {
      StorageNodeEx *node = (StorageNodeEx*) storageNodes->referenceFirstNode();

      while (node != NULL)
      {
         std::vector<HighResStatsList> dataSets;
         HighResStatsList data = node->getHighResData();
         uint16_t nodeID = node->getNumID();

         Node *relNode = node;
         storageNodes->releaseNode(&relNode);

         for (HighResStatsListIter iter = data.begin(); iter != data.end(); iter++)
         {
            HighResolutionStats s = *iter;
            sum += UnitTk::byteToMebibyte(s.incVals.diskWriteBytes);
         }
         node = (StorageNodeEx*) storageNodes->referenceNextNode(nodeID);
      }
   }
   else
   {
      TabType tabType = selectTableType(timespanM);

      UInt16List nodes;
      Database *db = Program::getApp()->getDB();
      db->getStorageNodes(&nodes);

      TimeAbs t;
      t.setToNow();
      long timeNow = (t.getTimeval()->tv_sec);
      uint64_t startTime = t.getTimeMS() - (timespanM * 60 * 1000);

      for (UInt16ListIter iter = nodes.begin(); iter != nodes.end(); iter++)
      {
         StorageNodeDataContentList data;
         uint16_t nodeID = *iter;
         db->getStorageNodeSets(nodeID, tabType, (startTime / 1000), timeNow, &data);

         for (StorageNodeDataContentListIter contIter = data.begin(); contIter != data.end();
            contIter++)
         {
            StorageNodeDataContent content = *contIter;
            sum += content.diskWrite; // db stores in MiB
         }
      }
   }

   return StringTk::doubleToStr(UnitTk::mebibyteToXbyte(int64_t(sum), outUnit));
}

void Display::diskPerfWriteSum(uint timespanM, UInt64List *outListTime,
   UInt64List *outListWritePerSec, UInt64List *outListAverageTime,
   UInt64List *outListAverageWritePerSec, uint64_t startTimestampMS, uint64_t endTimestampMS)
{
   // get all high res datasets and store them in a vector (result is a vector of lists)

   uint64_t valueTimeRange; // time range before and after the selected time

   // if timespan not greater than 10 minutes take direct data from servers, otherwise DB
   std::vector<HighResStatsList> dataSets;
   if (timespanM <= 10)
   {
      valueTimeRange = 500; // 500 ms before and after the time value

      StorageNodeEx *node = (StorageNodeEx*) storageNodes->referenceFirstNode();

      while (node != NULL)
      {
         HighResStatsList data = node->getHighResData();
         uint16_t nodeID = node->getNumID();

         Node *relNode = node;
         storageNodes->releaseNode(&relNode);

         dataSets.push_back(data);
         node = (StorageNodeEx*) storageNodes->referenceNextNode(nodeID);
      }
   }
   else
   {
      TabType tabType = selectTableTypeAndTimeRange(timespanM, &valueTimeRange);

      UInt16List nodes;
      Database *db = Program::getApp()->getDB();
      db->getStorageNodes(&nodes);

      for (UInt16ListIter iter = nodes.begin(); iter != nodes.end(); iter++)
      {
         StorageNodeDataContentList data;
         HighResStatsList highResData;

         uint16_t nodeID = *iter;
         db->getStorageNodeSets(nodeID, tabType, (startTimestampMS / 1000), (endTimestampMS / 1000),
            &data);

         for (StorageNodeDataContentListIter contIter = data.begin(); contIter != data.end();
            contIter++)
         {
            StorageNodeDataContent content = *contIter;
            HighResolutionStats stats;
            stats.rawVals.statsTimeMS = content.time * 1000;
            stats.incVals.diskWriteBytes = UnitTk::mebibyteToByte(content.diskWritePerSec);
            highResData.push_back(stats);
         }
         dataSets.push_back(highResData);
      }
   }

   // do as long until all lists are empty
   while (!dataSets.empty())
   {
      int elementCount = dataSets.size();

      // as long as the 'master list' contains elements
      while(!(dataSets[0].empty() ) )
      {
         bool validTimeFound = false;
         double sum = 0;
         uint64_t masterTime = 0;

         while(!validTimeFound)
         {
            masterTime = dataSets[0].front().rawVals.statsTimeMS;
            if(masterTime < startTimestampMS)
            {
               dataSets[0].pop_front();
               if(!(dataSets[0].empty() ) )
               {
                  masterTime = dataSets[0].front().rawVals.statsTimeMS;
               }
               else
               {
                  log.log(Log_ERR, "Didn't find write statistics in the given time frame. "
                     "Please check if the clocks on all servers are in sync.");
                  break;
               }
            }
            else
            {
               validTimeFound = true;
            }
         }

         if(validTimeFound)
         {
            for (int i = 0; i < elementCount; i++)
            {
               while ((!dataSets[i].empty()) &&
                  (dataSets[i].front().rawVals.statsTimeMS < masterTime + valueTimeRange))
               {
                  // add it to sum and pop the element
                  if (dataSets[i].front().rawVals.statsTimeMS >= masterTime - valueTimeRange)
                  {
                     sum += UnitTk::byteToMebibyte(dataSets[i].front().incVals.diskWriteBytes);
                  }
                  dataSets[i].pop_front();
               }
            }
            // add the sum to the out lists (with timestamp of the master list)
            outListTime->push_back(masterTime);
            outListWritePerSec->push_back(uint64_t(sum));
         }
      }
      // the master list was empty => pop it => next loop step will take the
      // next list as master
      dataSets.erase(dataSets.begin());
   }
   // calculate averages, it's important to use same order values here,
   // otherwise the chart will be screwed
   floatingAverage(outListWritePerSec, outListTime, outListAverageWritePerSec,
      outListAverageTime, DISPLAY_FLOATING_AVERAGE_ORDER);
}

void Display::diskPerfReadSum(uint timespanM, UInt64List *outListTime,
   UInt64List *outListReadPerSec, UInt64List *outListAverageTime,
   UInt64List *outListAverageReadPerSec, uint64_t startTimestampMS, uint64_t endTimestampMS)
{
   // get all high res datasets and store them in a vector (result is a vector of lists)

   uint64_t valueTimeRange; // time range before and after the selected time

   // if timespan not greater than 10 minutes take direct data from servers, otherwise DB
   std::vector<HighResStatsList> dataSets;
   if (timespanM <= 10)
   {
      valueTimeRange = 500; // 500 ms before and after the time value

      StorageNodeEx *node = (StorageNodeEx*) storageNodes->referenceFirstNode();

      while (node != NULL)
      {
         HighResStatsList data = node->getHighResData();
         uint16_t nodeID = node->getNumID();

         Node *relNode = node;
         storageNodes->releaseNode(&relNode);

         dataSets.push_back(data);
         node = (StorageNodeEx*) storageNodes->referenceNextNode(nodeID);
      }
   }
   else
   {
      TabType tabType = selectTableTypeAndTimeRange(timespanM, &valueTimeRange);

      UInt16List nodes;
      Database *db = Program::getApp()->getDB();
      db->getStorageNodes(&nodes);

      for (UInt16ListIter iter = nodes.begin(); iter != nodes.end(); iter++)
      {
         StorageNodeDataContentList data;
         HighResStatsList highResData;

         uint16_t nodeID = *iter;
         db->getStorageNodeSets(nodeID, tabType, (startTimestampMS / 1000), (endTimestampMS / 1000),
            &data);

         for (StorageNodeDataContentListIter contIter = data.begin(); contIter != data.end();
            contIter++)
         {
            StorageNodeDataContent content = *contIter;
            HighResolutionStats stats;
            stats.rawVals.statsTimeMS = content.time * 1000;
            // db stores in MiB
            stats.incVals.diskReadBytes = UnitTk::mebibyteToByte(content.diskReadPerSec);
            highResData.push_back(stats);
         }

         dataSets.push_back(highResData);
      }
   }

   // now add up the values for each time
   // timestamps will never be exactly the same => take about 1 second or 5 seconds, add
   // them and divide it by the span of the first and the last time

   // do as long until all lists are emptys
   while (!dataSets.empty()) // no lists remaining => stop
   {
      int elementCount = dataSets.size();

      // as long as the 'master list' contains elements
      while(!(dataSets[0].empty() ) )
      {
         bool validTimeFound = false;
         double sum = 0;
         uint64_t masterTime = 0;

         while(!validTimeFound)
         {
            masterTime = dataSets[0].front().rawVals.statsTimeMS;
            if(masterTime < startTimestampMS)
            {
               dataSets[0].pop_front();
               if(!(dataSets[0].empty() ) )
               {
                  masterTime = dataSets[0].front().rawVals.statsTimeMS;
               }
               else
               {
                  log.log(Log_ERR, "Didn't find read statistics in the given time frame. "
                     "Please check if the clocks on all servers are in sync.");
                  break;
               }
            }
            else
            {
               validTimeFound = true;
            }
         }

         if(validTimeFound)
         {
            for (int i = 0; i < elementCount; i++)
            {
               while ((!dataSets[i].empty()) &&
                  (dataSets[i].front().rawVals.statsTimeMS < masterTime + valueTimeRange))
               {
                  // add it to sum and pop the element
                  if (dataSets[i].front().rawVals.statsTimeMS >= masterTime - valueTimeRange)
                  {
                     sum += UnitTk::byteToMebibyte(dataSets[i].front().incVals.diskReadBytes);
                  }
                  dataSets[i].pop_front();
               }
            }
            // add the sum to the out lists (with timestamp of the master list)
            outListTime->push_back(masterTime);
            outListReadPerSec->push_back(uint64_t(sum));
         }
      }
      // the master list was empty => pop it => next loop step will take the
      // next list as master
      dataSets.erase(dataSets.begin());
   }

   floatingAverage(outListReadPerSec, outListTime, outListAverageReadPerSec,
      outListAverageTime, DISPLAY_FLOATING_AVERAGE_ORDER);
}

unsigned Display::sessionCountMeta(uint16_t nodeID)
{
   unsigned res = 0;
   MetaNodeEx *node = (MetaNodeEx*) metaNodes->referenceNode(nodeID);
   if (node != NULL)
   {
      res = node->getContent().sessionCount;

      Node *relNode = node;
      metaNodes->releaseNode(&relNode);
   }
   return res;
}

unsigned Display::sessionCountStorage(uint16_t nodeID)
{
   unsigned res = 0;
   StorageNodeEx *node = (StorageNodeEx*) storageNodes->referenceNode(nodeID);
   if (node != NULL)
   {
      res = node->getContent().sessionCount;

      Node *relNode = node;
      storageNodes->releaseNode(&relNode);
   }
   return res;
}

/**
 * @return false on error (e.g. communication error or path does not exist)
 */
bool Display::getEntryInfo(std::string pathStr, std::string *outChunkSize,
   std::string *outNumTargets, UInt16Vector *outPrimaryTargetNumIDs, unsigned* outPatternID,
   UInt16Vector *outSecondaryTargetNumIDs,  UInt16Vector *outStorageBMGs,
   uint16_t* outMetaNodeNumID, uint16_t* outMetaMirrorNodeNumID)
{
   Logger* log = Program::getApp()->getLogger();

   bool retVal = false;

   // find owner node
   Path path(pathStr);
   path.setAbsolute(true);

   Node* ownerNode = NULL;
   EntryInfo entryInfo;

   FhgfsOpsErr findRes = MetadataTk::referenceOwner(&path, false, metaNodes, &ownerNode,
      &entryInfo);

   if (findRes != FhgfsOpsErr_SUCCESS)
   {
      log->logErr(__FUNCTION__ , "Unable to find metadata node for path: " + pathStr +
         "Error: " + FhgfsOpsErrTk::toErrString(findRes) );
   }
   else
   {
      bool commRes;
      char* respBuf = NULL;
      NetMessage* respMsg = NULL;
      GetEntryInfoRespMsg* respMsgCast;

      GetEntryInfoMsg getInfoMsg(&entryInfo);

      commRes = MessagingTk::requestResponse(ownerNode, &getInfoMsg, NETMSGTYPE_GetEntryInfoResp,
         &respBuf, &respMsg);

      if (commRes)
      {
         respMsgCast = (GetEntryInfoRespMsg*) respMsg;

         StripePattern* pattern = NULL;
         pattern = respMsgCast->createPattern();

         *outChunkSize = StringTk::uintToStr(pattern->getChunkSize());
         *outNumTargets = StringTk::uintToStr(pattern->getDefaultNumTargets());
         *outPatternID = pattern->getPatternType();

         if(*outPatternID == STRIPEPATTERN_Raid10)
         {
            *outPrimaryTargetNumIDs = *pattern->getStripeTargetIDs();
            *outSecondaryTargetNumIDs = *pattern->getMirrorTargetIDs();
         }
         else if(*outPatternID == STRIPEPATTERN_BuddyMirror)
         {
            *outStorageBMGs = *pattern->getStripeTargetIDs();

            Node *mgmtNode = mgmtdNodes->referenceFirstNode();

            UInt16List buddyGroupIDs;
            UInt16List buddyGroupPrimaryTargetIDs;
            UInt16List buddyGroupSecondaryTargetIDs;
            if(!NodesTk::downloadMirrorBuddyGroups(mgmtNode, NODETYPE_Storage, &buddyGroupIDs,
               &buddyGroupPrimaryTargetIDs, &buddyGroupSecondaryTargetIDs, false) )
            {
               log->logErr(__FUNCTION__ , "Download of mirror buddy groups failed.");
               mgmtdNodes->releaseNode(&mgmtNode);

               return APPCODE_RUNTIME_ERROR;
            }
            mgmtdNodes->releaseNode(&mgmtNode);

            UInt16ListIter buddyGroupIDIter = buddyGroupIDs.begin();
            UInt16ListIter primaryTargetIDIter = buddyGroupPrimaryTargetIDs.begin();
            UInt16ListIter secondaryTargetIDIter = buddyGroupSecondaryTargetIDs.begin();
            for( ; (buddyGroupIDIter != buddyGroupIDs.end() ) &&
                   (primaryTargetIDIter != buddyGroupPrimaryTargetIDs.end() ) &&
                   (secondaryTargetIDIter != buddyGroupSecondaryTargetIDs.end() );
               buddyGroupIDIter++, primaryTargetIDIter++, secondaryTargetIDIter++)
            {
               UInt16VectorIter iter = std::find(outStorageBMGs->begin(), outStorageBMGs->end(),
                  *buddyGroupIDIter);
               if (iter != outStorageBMGs->end() )
               {
                  outPrimaryTargetNumIDs->push_back(*primaryTargetIDIter);
                  outSecondaryTargetNumIDs->push_back(*secondaryTargetIDIter);
               }
            }
         }
         else
         {
            *outPrimaryTargetNumIDs = *pattern->getStripeTargetIDs();
         }

         pattern->getStripeTargetIDs(outPrimaryTargetNumIDs);
         pattern->getMirrorTargetIDs(outSecondaryTargetNumIDs);

         *outMetaNodeNumID = ownerNode->getNumID();
         *outMetaMirrorNodeNumID = respMsgCast->getMirrorNodeID();

         delete (pattern);

         retVal = true;
      }

      Node *relNode = ownerNode;
      metaNodes->releaseNode(&relNode);
      SAFE_DELETE(respMsg);
      SAFE_FREE(respBuf);
   }

   return retVal;
}

/**
 * @param patternID pattern type (STRIPEPATTERN_...)
 * @return false on error
 */
bool Display::setPattern(std::string pathStr, uint chunkSize, uint defaultNumNodes,
   unsigned patternID, bool doMetaMirroring)
{
   bool retVal = false;

   // find the owner node of the path
   Path path(pathStr);
   path.setAbsolute(true);
   Node* ownerNode = NULL;
   EntryInfo entryInfo;

   log.log(Log_SPAM, std::string("Set pattern for path: ") + pathStr );

   FhgfsOpsErr findRes = MetadataTk::referenceOwner(&path, false, metaNodes, &ownerNode,
      &entryInfo);

   if(findRes != FhgfsOpsErr_SUCCESS)
   {
      log.logErr("Unable to find metadata node for path: " + pathStr);
      log.logErr("Error: " + std::string(FhgfsOpsErrTk::toErrString(findRes) ));
      return retVal;
   }

   char* respBuf = NULL;
   char* respBufMirror = NULL;
   NetMessage* respMsg = NULL;

   // generate a new StripePattern with an empty list of stripe nodes (will be
   // ignored on the server side)

   UInt16Vector stripeNodeIDs;
   UInt16Vector mirrorTargetIDs;
   StripePattern* pattern = NULL;

   if (patternID == STRIPEPATTERN_Raid0)
      pattern = new Raid0Pattern(chunkSize, stripeNodeIDs, defaultNumNodes);
   else
   if (patternID == STRIPEPATTERN_Raid10)
      pattern = new Raid10Pattern(chunkSize, stripeNodeIDs, mirrorTargetIDs, defaultNumNodes);
   else
   if (patternID == STRIPEPATTERN_BuddyMirror)
      pattern = new BuddyMirrorPattern(chunkSize, stripeNodeIDs, defaultNumNodes);
   else
   {
      log.logErr("Set pattern: Invalid/unknown pattern type number: " +
         StringTk::uintToStr(patternID) );
   }

   if(pattern)
   {
      // send SetDirPatternMsg

      SetDirPatternMsg setPatternMsg(&entryInfo, pattern);

      bool commRes = MessagingTk::requestResponse(ownerNode, &setPatternMsg,
         NETMSGTYPE_SetDirPatternResp, &respBuf, &respMsg);

      if(!commRes)
      {
         log.logErr("Set pattern: Communication failed with node: " +
            ownerNode->getNodeIDWithTypeStr() );

         goto cleanup;
      }

      SetDirPatternRespMsg *setDirPatternRespMsg = (SetDirPatternRespMsg *)respMsg;
      FhgfsOpsErr result = setDirPatternRespMsg->getResult();

      if (result != FhgfsOpsErr_SUCCESS)
      {
         log.logErr("Modification of stripe pattern failed on server. "
            "Path: " + pathStr + "; "
            "Server: " + ownerNode->getNodeIDWithTypeStr() + "; "
            "Error: " + FhgfsOpsErrTk::toErrString(result) );

         goto cleanup;
      }

      // successfully modified on owner
      retVal = true;
   }

   if(doMetaMirroring)
   {
      NetMessage* respMsgMirror = NULL;
      SetMetadataMirroringRespMsg* respMsgCast;

      FhgfsOpsErr setRemoteRes;

      SetMetadataMirroringMsg setMsg(&entryInfo);

      // request/response
      bool commRes = MessagingTk::requestResponse(
         ownerNode, &setMsg, NETMSGTYPE_SetMetadataMirroringResp, &respBufMirror, &respMsgMirror);
      if(!commRes)
      {
         log.logErr("Communication with server failed: " + ownerNode->getNodeIDWithTypeStr() );
         goto cleanup;
      }

      respMsgCast = (SetMetadataMirroringRespMsg*)respMsgMirror;

      setRemoteRes = respMsgCast->getResult();
      if(setRemoteRes != FhgfsOpsErr_SUCCESS)
      {
         log.logErr("Operation failed on server: " + ownerNode->getNodeIDWithTypeStr() + "; " +
            "Error: " + FhgfsOpsErrTk::toErrString(setRemoteRes) );
         goto cleanup;
      }

      retVal = true;
   }

cleanup:
   Node* relNode = ownerNode;
   metaNodes->releaseNode(&relNode);
   SAFE_DELETE(respMsg);
   SAFE_FREE(respBuf);
   SAFE_DELETE(pattern);
   SAFE_FREE(respBufMirror);

   return retVal;
}

/*
 * list a directory from a given offset (list incrementally)
 *
 * @param pathStr The path to be listed
 * @param offset The starting offset; will be modified to represent ending offset
 * @param outEntries reference to a FileEntryList used as result list
 * @param tmpTotalSize total size of the read entries in byte
 * @param count Amount of entries to read
 */
int Display::listDirFromOffset(std::string pathStr, int64_t *offset,
   FileEntryList *outEntries, uint64_t *tmpTotalSize, short count)
{
   log.log(Log_SPAM, std::string("Listing content of path: ") + pathStr);
   // find owner node
   *tmpTotalSize = 0;
   Path path(pathStr);
   path.setAbsolute(true);

   Node* ownerNode = NULL;
   EntryInfo dirEntryInfo;

   FhgfsOpsErr findRes = MetadataTk::referenceOwner(&path, false, metaNodes, &ownerNode,
      &dirEntryInfo);
   if (findRes != FhgfsOpsErr_SUCCESS)
   {
      log.logErr("Unable to find metadata node for path: " + pathStr + ". " +
         "(Error: " + FhgfsOpsErrTk::toErrString(findRes) + ")");
      return -1;
   }
   else
   {
      // get the contents

      bool commRes;
      char* respListBuf = NULL;
      NetMessage* respListDirMsg = NULL;
      ListDirFromOffsetRespMsg* listDirRespMsg;
      ListDirFromOffsetMsg listDirMsg(&dirEntryInfo, *offset, count, true);

      commRes = MessagingTk::requestResponse(ownerNode, &listDirMsg,
         NETMSGTYPE_ListDirFromOffsetResp, &respListBuf, &respListDirMsg);
      StringList fileNames;

      if (commRes)
      {
         listDirRespMsg = (ListDirFromOffsetRespMsg*) respListDirMsg;
         bool nameRes = listDirRespMsg->parseNames(&fileNames);

         if (nameRes == false)
         {
            SAFE_FREE(respListBuf);
            SAFE_DELETE(respListDirMsg);

            Node *relNode = ownerNode;
            metaNodes->releaseNode(&relNode);
            return -1;
         }

         *offset = listDirRespMsg->getNewServerOffset();
      }
      else
      {
         SAFE_FREE(respListBuf);
         SAFE_DELETE(respListDirMsg);
         Node *relNode = ownerNode;
         metaNodes->releaseNode(&relNode);
         return -1;
      }
      Node *relNode = ownerNode;
      metaNodes->releaseNode(&relNode);

      // get attributes
      NetMessage* respAttrMsg = NULL;
      StringListIter nameIter;

      /* check for equal length of fileNames and fileTypes is
       * in ListDirFromOffsetRespMsg::deserializePayload() */
      for (nameIter = fileNames.begin(); nameIter != fileNames.end();
         nameIter++)
      {
         std::string nameStr = pathStr + "/" + *nameIter;
         Path tmpPath(nameStr);
         EntryInfo entryInfo;
         Node *entryOwnerNode;

         if (MetadataTk::referenceOwner(&tmpPath, false, metaNodes,
            &entryOwnerNode, &entryInfo) == FhgfsOpsErr_SUCCESS)
         {
            StatRespMsg* statRespMsg;

            StatMsg statMsg(&entryInfo);
            char* respAttrBuf = NULL;
            commRes = MessagingTk::requestResponse(entryOwnerNode, &statMsg,
               NETMSGTYPE_StatResp, &respAttrBuf, &respAttrMsg);
            if (commRes)
            {
               statRespMsg = (StatRespMsg*) respAttrMsg;

               StatData* statData = statRespMsg->getStatData();

               FileEntry entry;
               entry.name = *nameIter;
               entry.statData.set(statData);

               outEntries->push_back(entry);
               *tmpTotalSize += entry.statData.getFileSize();
            }
            else
            {
               SAFE_DELETE(respAttrMsg);
               SAFE_FREE(respAttrBuf);
               Node *relNode = entryOwnerNode;
               metaNodes->releaseNode(&relNode);
               SAFE_FREE(respListBuf);
               SAFE_DELETE(respListDirMsg);

               return -1;
            }
            SAFE_DELETE(respAttrMsg);
            SAFE_FREE(respAttrBuf);
            Node *relNode = entryOwnerNode;
            metaNodes->releaseNode(&relNode);
         }
         else
         {
            log.logErr("Metadata for entry "+ *nameIter +" could not be read.");
         }
      }
      SAFE_FREE(respListBuf);
      SAFE_DELETE(respListDirMsg);
   }
   return 0;
}

std::string Display::formatDateSec(uint64_t sec)
{
   time_t ctime = (time_t) sec;
   return asctime(localtime(&(ctime)));
}

std::string Display::formatFileMode(int mode)
{
   const char *rwx = "rwxrwxrwx";
   int bits[] =
      { S_IRUSR, S_IWUSR, S_IXUSR, /*Permissions User*/
      S_IRGRP, S_IWGRP, S_IXGRP, /*Group*/
      S_IROTH, S_IWOTH, S_IXOTH /*All*/
      };

   char res[10];
   res[0] = '\0';
   for (short i = 0; i < 9; i++)
   {
      res[i] = (mode & bits[i]) ? rwx[i] : '-';
   }
   res[9] = '\0';
   return std::string(res);
}

bool Display::getGeneralMetaNodeInfo(uint16_t nodeID, GeneralNodeInfo *outInfo)
{
   MetaNodeEx *node = (MetaNodeEx*) metaNodes->referenceNode(nodeID);

   if (node != NULL)
   {
      *outInfo = node->getGeneralInformation();

      Node *relNode = node;
      metaNodes->releaseNode(&relNode);

      return true;
   }

   return false;
}

bool Display::getGeneralStorageNodeInfo(uint16_t nodeID, GeneralNodeInfo *outInfo)
{
   StorageNodeEx *node = (StorageNodeEx*) storageNodes->referenceNode(nodeID);

   if (node != NULL)
   {
      *outInfo = node->getGeneralInformation();

      Node *relNode = node;
      storageNodes->releaseNode(&relNode);

      return true;
   }

   return false;
}

NicAddressList Display::getMetaNodeNicList(uint16_t nodeID)
{
   MetaNodeEx *node = (MetaNodeEx*) metaNodes->referenceNode(nodeID);
   NicAddressList nicList;
   if (node != NULL)
   {
      nicList = node->getNicList();

      Node *relNode = node;
      metaNodes->releaseNode(&relNode);
   }
   return nicList;
}

NicAddressList Display::getStorageNodeNicList(uint16_t nodeID)
{
   StorageNodeEx *node = (StorageNodeEx*) storageNodes->referenceNode(nodeID);
   NicAddressList nicList;
   if (node != NULL)
   {
      nicList = node->getNicList();

      Node *relNode = node;
      storageNodes->releaseNode(&relNode);
   }
   return nicList;
}

void Display::metaDataRequests(uint16_t nodeID, uint timespanM, StringList
   *outListTime, StringList *outListQueuedRequests, StringList *outListWorkRequests)
{
   HighResStatsList data;
   MetaNodeEx *node = (MetaNodeEx*) metaNodes->referenceNode(nodeID);
   if (node != NULL)
   {
      data = node->getHighResData();

      Node *relNode = node;
      metaNodes->releaseNode(&relNode);
   }

   // timeSpan in minutes, 10 minutes are stored server side, if longer span is
   // requested take averaged values from db
   if ((timespanM <= 10) && (!data.empty()))
   {
      for (HighResStatsListIter iter = data.begin(); iter != data.end(); iter++)
      {
         HighResolutionStats content = *iter;
         outListTime->push_back(StringTk::uint64ToStr(content.rawVals.statsTimeMS));
         unsigned queuedRequests = content.rawVals.queuedRequests;
         unsigned workRequests = content.incVals.workRequests;
         outListQueuedRequests->push_back(StringTk::uintToStr(queuedRequests));
         outListWorkRequests->push_back(StringTk::uintToStr(workRequests));
      }
   }
   else //take db data
   {
      TabType tabType = selectTableType(timespanM);

      TimeAbs t;
      t.setToNow();
      long timeNow = (t.getTimeval()->tv_sec);
      uint64_t startTime = t.getTimeMS() - (timespanM * 60 * 1000);

      MetaNodeDataContentList data;
      Database *db = Program::getApp()->getDB();
      db->getMetaNodeSets(nodeID, tabType, (startTime / 1000), timeNow, &data);

      for (MetaNodeDataContentListIter iter = data.begin(); iter != data.end(); iter++)
      {
         MetaNodeDataContent content = *iter;
         outListTime->push_back(StringTk::uint64ToStr((content.time) * 1000));
         unsigned queuedRequests = content.queuedRequests;
         unsigned workRequests = content.workRequests;
         outListQueuedRequests->push_back(StringTk::uintToStr(queuedRequests));
         outListWorkRequests->push_back(StringTk::uintToStr(workRequests));
      }
   }
}

void Display::metaDataRequestsSum(uint timespanM, StringList *outListTime,
   StringList *outListQueuedRequests, StringList *outListWorkRequests)
{
   uint64_t valueTimeRange; // time range before and after the selected time

   // get all high res datasets and store them in a vector (result is a vector of lists)

   // if timespan not greater than 10 minutes take direct data from servers, otherwise DB
   std::vector<HighResStatsList> dataSets;
   if (timespanM <= 10)
   {
      valueTimeRange = 500; // 500 ms before and after the time value

      MetaNodeEx *node = (MetaNodeEx*) metaNodes->referenceFirstNode();
      while (node != NULL)
      {
         HighResStatsList data = node->getHighResData();
         uint16_t nodeID = node->getNumID();

         Node *relNode = node;
         metaNodes->releaseNode(&relNode);
         dataSets.push_back(data);
         node = (MetaNodeEx*) metaNodes->referenceNextNode(nodeID);
      }
   }
   else
   {
      TabType tabType = selectTableTypeAndTimeRange(timespanM, &valueTimeRange);

      UInt16List nodes;
      Database *db = Program::getApp()->getDB();
      db->getMetaNodes(&nodes);

      TimeAbs t;
      t.setToNow();
      long timeNow = (t.getTimeval()->tv_sec);
      uint64_t startTime = t.getTimeMS() - (timespanM * 60 * 1000);

      for (UInt16ListIter iter = nodes.begin(); iter != nodes.end(); iter++)
      {
         MetaNodeDataContentList data;
         uint16_t nodeID = *iter;
         db->getMetaNodeSets(nodeID, tabType, (startTime / 1000), timeNow, &data);
         HighResStatsList highResData;
         for (MetaNodeDataContentListIter contIter = data.begin(); contIter != data.end();
            contIter++)
         {
            MetaNodeDataContent content = *contIter;
            HighResolutionStats stats;
            stats.rawVals.statsTimeMS = content.time * 1000;
            stats.rawVals.queuedRequests = content.queuedRequests;
            stats.incVals.workRequests = content.workRequests;
            highResData.push_back(stats);
         }
         dataSets.push_back(highResData);
      }
   }

   // do as long until all lists are empty
   while (!dataSets.empty())
   {
      int elementCount = dataSets.size();

      // as long as the 'master list' contains elements
      while (!(dataSets[0].empty()))
      {
         unsigned queuedRequestsSum = 0;
         unsigned workRequestsSum = 0;
         uint64_t masterTime = dataSets[0].front().rawVals.statsTimeMS;

         for (int i = 0; i < elementCount; i++)
         {
            while ((!dataSets[i].empty()) &&
               (dataSets[i].front().rawVals.statsTimeMS < masterTime + valueTimeRange))
            {
               // add it to sum and pop the element
               if (dataSets[i].front().rawVals.statsTimeMS >= masterTime - valueTimeRange)
               {
                  queuedRequestsSum += dataSets[i].front().rawVals.queuedRequests;
                  workRequestsSum += dataSets[i].front().incVals.workRequests;
               }
               dataSets[i].pop_front();
            }
         }
         // add the sum to the out lists (with timestamp of the master list)
         outListTime->push_back(StringTk::uint64ToStr(masterTime));
         outListQueuedRequests->push_back( StringTk::uintToStr(queuedRequestsSum));
         outListWorkRequests->push_back(StringTk::uintToStr(workRequestsSum));
      }
      // the master list was empty => pop it => next loop step will take the
      // next list as master
      dataSets.erase(dataSets.begin());
   }
}

void Display::storageTargetsInfo(uint16_t nodeID,
   StorageTargetInfoList *outStorageTargets)
{
   StorageNodeEx *node = (StorageNodeEx*) storageNodes->referenceNode(nodeID);
   if (node != NULL)
   {
      *outStorageTargets = node->getContent().storageTargets;

      Node *relNode = node;
      storageNodes->releaseNode(&relNode);
   }
}
