#include <common/components/worker/GetQuotaInfoWork.h>
#include <common/net/message/storage/quota/GetQuotaInfoRespMsg.h>
#include <common/toolkit/SynchronizedCounter.h>
#include "GetQuotaInfo.h"


typedef std::map<uint16_t, Mutex> MutexMap;
typedef MutexMap::iterator MutexMapIter;
typedef MutexMap::const_iterator MutexMapConstIter;
typedef MutexMap::value_type MutexMapVal;


/*
 * send the quota data requests to the storage nodes and collects the responses
 *
 * @param mgmtNode the management node, need to be referenced
 * @param storageNodes a NodeStore with all storage servers
 * @param workQ the MultiWorkQueue of the app
 * @param outQuotaResults returns the quota informations
 * @param mapper the target mapper (only required for GETQUOTACONFIG_SINGLE_TARGET and
 *        GETQUOTACONFIG_ALL_TARGETS_ONE_REQUEST_PER_TARGET)
 * @param requestLimits if true, the quota limits will be downloaded from the management server and
 *        not the used quota will be downloaded from the storage servers
 * @return error information, true on success, false if a error occurred
 *
 */
bool GetQuotaInfo::requestQuotaDataAndCollectResponses(Node* mgmtNode,
   NodeStoreServers* storageNodes, MultiWorkQueue* workQ, QuotaDataMapForTarget* outQuotaResults,
   TargetMapper* mapper, bool requestLimits)
{
   bool retVal = true;

   int numWorks = 0;
   SynchronizedCounter counter;

   int maxMessageCount = getMaxMessageCount();

   UInt16Vector nodeResults;
   NodeList allNodes;

   MutexMap mutexMap;

   if(requestLimits)
   { /* download the limits from the management daemon, multiple messages will be used if the
        payload of the message is to small */

      nodeResults = UInt16Vector(maxMessageCount);

      QuotaDataMap map;
      outQuotaResults->insert(QuotaDataMapForTargetMapVal(QUOTADATAMAPFORTARGET_ALL_TARGETS_ID,
         map) );

      Mutex mutex;
      mutexMap.insert(MutexMapVal(QUOTADATAMAPFORTARGET_ALL_TARGETS_ID, mutex) );

      //send command to the storage servers and print the response
      if(mgmtNode)
      {
         // request all subranges (=> msg size limitation) from current server
         for(int messageNumber = 0; messageNumber < maxMessageCount; messageNumber++)
         {
            Work* work = new GetQuotaInfoWork(this->cfg, mgmtNode, messageNumber,
               &outQuotaResults->find(QUOTADATAMAPFORTARGET_ALL_TARGETS_ID)->second,
               &mutexMap.find(QUOTADATAMAPFORTARGET_ALL_TARGETS_ID)->second, &counter,
               &nodeResults[numWorks]);
            workQ->addDirectWork(work);

            numWorks++;
         }
      }
   }
   else
   if(this->cfg.cfgTargetSelection == GETQUOTACONFIG_ALL_TARGETS_ONE_REQUEST)
   { /* download the used quota from the storage daemon, one request for all targets, multiple
        messages will be used if the payload of the message is to small */

      storageNodes->referenceAllNodes(&allNodes);

      nodeResults = UInt16Vector(maxMessageCount * storageNodes->getSize() );

      // request all subranges (=> msg size limitation) from current server
      for(int messageNumber = 0; messageNumber < maxMessageCount; messageNumber++)
      {
         for (NodeListIter nodeIt = allNodes.begin(); nodeIt != allNodes.end(); ++nodeIt)
         {
            //send command to the storage servers and print the response
            Node* storageNode = *nodeIt;

            if(messageNumber == 0)
            {
               QuotaDataMap map;
               outQuotaResults->insert(QuotaDataMapForTargetMapVal(
                  storageNode->getNumID(), map) );

               Mutex mutex;
               mutexMap.insert(MutexMapVal(storageNode->getNumID(), mutex) );
            }

            Work* work = new GetQuotaInfoWork(this->cfg, storageNode, messageNumber,
               &outQuotaResults->find(storageNode->getNumID())->second,
               &mutexMap.find(storageNode->getNumID())->second, &counter,
               &nodeResults[numWorks]);
            workQ->addDirectWork(work);

            numWorks++;
         }
      }
   }
   else
   if(this->cfg.cfgTargetSelection == GETQUOTACONFIG_ALL_TARGETS_ONE_REQUEST_PER_TARGET)
   { /* download the used quota from the storage daemon, a single request for each target, multiple
        messages will be used if the payload of the message is to small */

      storageNodes->referenceAllNodes(&allNodes);

      nodeResults = UInt16Vector(maxMessageCount * mapper->getSize() );

      // request all subranges (=> msg size limitation) from current server
      for(int messageNumber = 0; messageNumber < maxMessageCount; messageNumber++)
      {
         for (NodeListIter nodeIt = allNodes.begin(); nodeIt != allNodes.end(); ++nodeIt)
         {
            Node* storageNode = *nodeIt;

            //send command to the storage servers
            UInt16List targetIDs;
            mapper->getTargetsByNode(storageNode->getNumID(), targetIDs);

            // request the quota data for the targets in a separate message
            for(UInt16ListIter iter = targetIDs.begin(); iter != targetIDs.end(); iter++)
            {
               GetQuotaInfoConfig requestCfg = this->cfg;
               requestCfg.cfgTargetNumID = *iter;

               if(messageNumber == 0)
               {
                  QuotaDataMap map;
                  outQuotaResults->insert(QuotaDataMapForTargetMapVal(*iter, map) );

                  Mutex mutex;
                  mutexMap.insert(MutexMapVal(*iter, mutex) );
               }

               Work* work = new GetQuotaInfoWork(requestCfg, storageNode, messageNumber,
                  &outQuotaResults->at(*iter), &mutexMap.at(*iter), &counter,
                  &nodeResults[numWorks]);
               workQ->addDirectWork(work);

               numWorks++;
            }
         }
      }
   }
   else
   if(this->cfg.cfgTargetSelection == GETQUOTACONFIG_SINGLE_TARGET)
   { /* download the used quota from the storage daemon, request the data only for a single target,
        multiple messages will be used if the payload of the message is to small */
      uint16_t storageNumID = mapper->getNodeID(this->cfg.cfgTargetNumID);

      allNodes.push_back(storageNodes->referenceNode(storageNumID));
      Node* storageNode = allNodes.front();

      // use IntVector because it doesn't work with BoolVector. std::vector<bool> is not a container
      nodeResults = UInt16Vector(maxMessageCount);

      QuotaDataMap map;
      outQuotaResults->insert(QuotaDataMapForTargetMapVal(this->cfg.cfgTargetNumID, map) );

      Mutex mutex;
      mutexMap.insert(MutexMapVal(this->cfg.cfgTargetNumID, mutex) );

      //send command to the storage servers
      if(storageNode != NULL)
      {
         // request all subranges (=> msg size limitation) from current server
         for(int messageNumber = 0; messageNumber < maxMessageCount; messageNumber++)
         {
            Work* work = new GetQuotaInfoWork(this->cfg, storageNode, messageNumber,
               &outQuotaResults->find(this->cfg.cfgTargetNumID)->second,
               &mutexMap.find(this->cfg.cfgTargetNumID)->second, &counter, &nodeResults[numWorks]);
            workQ->addDirectWork(work);

            numWorks++;
         }
      }
      else
      {
         LogContext("GetQuotaInfo").logErr("Unknown target selection.");
         nodeResults[numWorks] = 0;
      }
   }

   // wait for all work to be done
   counter.waitForCount(numWorks);

   // merge the quota data if required
   if(!requestLimits && (this->cfg.cfgTargetSelection == GETQUOTACONFIG_ALL_TARGETS_ONE_REQUEST) )
   {
      QuotaDataMap map;

      for(QuotaDataMapForTargetIter storageIter = outQuotaResults->begin();
         storageIter != outQuotaResults->end(); storageIter++ )
      {
         //merge the QuotaData from the responses with the existing quotaData
         for(QuotaDataMapIter resultIter = storageIter->second.begin();
            resultIter != storageIter->second.end(); resultIter++)
         {
            QuotaDataMapIter outIter = map.find(resultIter->first);
            if(outIter != map.end())
               outIter->second.mergeQuotaDataCounter(&(resultIter->second) );
            else
               map.insert(QuotaDataMapVal(resultIter->first, resultIter->second) );
         }
      }

      outQuotaResults->clear();
      outQuotaResults->insert(QuotaDataMapForTargetMapVal(QUOTADATAMAPFORTARGET_ALL_TARGETS_ID,
         map) );
   }

   for (UInt16VectorIter iter = nodeResults.begin(); iter != nodeResults.end(); iter++)
   {
      if (*iter != 0)
      {
         // delete QuotaDataMap with invalid QuotaData
         if(this->cfg.cfgTargetSelection != GETQUOTACONFIG_ALL_TARGETS_ONE_REQUEST)
            outQuotaResults->erase(*iter);

         retVal = false;
      }
   }

   storageNodes->releaseAllNodes(&allNodes);

   return retVal;
}

/*
 * calculate the number of massages which are required to download all quota data
 */
int GetQuotaInfo::getMaxMessageCount()
{
   int retVal = 1;

   if(this->cfg.cfgUseList || this->cfg.cfgUseAll)
   {
      retVal = this->cfg.cfgIDList.size() / GETQUOTAINFORESPMSG_MAX_ID_COUNT;

      if( (this->cfg.cfgIDList.size() % GETQUOTAINFORESPMSG_MAX_ID_COUNT) != 0)
         retVal++;
   }
   else
   if(this->cfg.cfgUseRange)
   {
      int value = this->cfg.cfgIDRangeEnd - this->cfg.cfgIDRangeStart;
      retVal = value / GETQUOTAINFORESPMSG_MAX_ID_COUNT;

      if( (value % GETQUOTAINFORESPMSG_MAX_ID_COUNT) != 0)
         retVal++;
   }

   return retVal;
}

