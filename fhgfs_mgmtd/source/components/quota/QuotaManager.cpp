#include <app/App.h>
#include <common/threading/PThread.h>
#include <common/threading/SafeRWLock.h>
#include <common/toolkit/SynchronizedCounter.h>
#include <common/storage/quota/QuotaData.h>
#include <components/worker/SetExceededQuotaWork.h>
#include <program/Program.h>


#include "QuotaManager.h"

QuotaManager::QuotaManager() : PThread("QuotaMgr")
{
   log.setContext("QuotaManager");

   this->usedQuotaUser = new QuotaDataMapForTarget();
   this->usedQuotaGroup = new QuotaDataMapForTarget();

   std::string mgmtStorePath = Program::getApp()->getConfig()->getStoreMgmtdDirectory();
   this->quotaUserLimits = new QuotaStoreLimits(QuotaDataType_USER, "QuotaUserLimits",
      mgmtStorePath + "/" + CONFIG_QUOTA_USER_LIMITS_FILENAME);
   this->quotaGroupLimits = new QuotaStoreLimits(QuotaDataType_GROUP, "QuotaGroupLimits",
      mgmtStorePath + "/" + CONFIG_QUOTA_GROUP_LIMITS_FILENAME);

   this->exceededQuotaStore = new ExceededQuotaStore();

   // load the quota data and the quota limits if possible
   this->loadQuotaData();
}

QuotaManager::~QuotaManager()
{
   SAFE_DELETE(this->usedQuotaUser);
   SAFE_DELETE(this->usedQuotaGroup);

   SAFE_DELETE(this->quotaUserLimits);
   SAFE_DELETE(this->quotaGroupLimits);

   SAFE_DELETE(this->exceededQuotaStore);
}

void QuotaManager::run()
{
   try
   {
      registerSignalHandler();

      requestLoop();

      log.log(Log_DEBUG, "Component stopped.");
   }
   catch(std::exception& e)
   {
      Program::getApp()->handleComponentException(e);
   }
}

void QuotaManager::requestLoop()
{
   Config* cfg = Program::getApp()->getConfig();

   const int sleepIntervalMS = 5*1000; // 5sec

   const unsigned updateQuotaUsageMS = cfg->getQuotaUpdateIntervalMin()*60*1000; // min to millisec
   const unsigned saveQuotaDataMS = cfg->getQuotaStoreIntervalMin()*60*1000; // min to millisec

   bool firstUpdate = true;

   Time lastQuotaUsageUpdateT;
   Time lastQuotaDataSaveT;

   while(!waitForSelfTerminateOrder(sleepIntervalMS) )
   {
      if(firstUpdate || (lastQuotaUsageUpdateT.elapsedMS() > updateQuotaUsageMS) )
      {
         updateUsedQuota();
         calculateExceededQuota();
         pushExceededQuotaIDs();
         lastQuotaUsageUpdateT.setToNow();
      }

      if(firstUpdate || (lastQuotaDataSaveT.elapsedMS() > saveQuotaDataMS) )
      {
         saveQuotaData();
         lastQuotaDataSaveT.setToNow();
      }

      firstUpdate = false;
   }

   saveQuotaData();
}

/**
 * removes a target from the quota data store, required for target unmap and remove node
 *
 * @param targetNumID the targetNumID  of the target to remove
 */
void QuotaManager::removeTargetFromQuotaDataStores(uint16_t targetNumID)
{
   SafeRWLock lockUser(&this->usedQuotaUserRWLock, SafeRWLock_WRITE);            //W R I T E L O C K

   QuotaDataMapForTargetIter iterUser = this->usedQuotaUser->find(targetNumID);
   if(iterUser != this->usedQuotaUser->end() )
   {
      this->usedQuotaUser->erase(iterUser);
      this->usedQuotaUserStoreDirty = true;
   }

   lockUser.unlock();                                                            // U N L O C K


   SafeRWLock lockGroup(&this->usedQuotaGroupRWLock, SafeRWLock_WRITE);          //W R I T E L O C K

   QuotaDataMapForTargetIter iterGroup = this->usedQuotaGroup->find(targetNumID);
   if(iterGroup != this->usedQuotaGroup->end() )
   {
      this->usedQuotaGroup->erase(iterGroup);
      this->usedQuotaGroupStoreDirty = true;
   }

   lockGroup.unlock();                                                           // U N L O C K
}

/**
 * adds or updates the quota limits with the given list
 *
 * @param list the list with the new quota limits
 *
 * @return true if the update or insert was successful
 */
bool QuotaManager::updateQuotaLimits(QuotaDataList* list)
{
   bool retVal = false;

   if(list->begin()->getType() == QuotaDataType_USER)
      retVal = this->quotaUserLimits->addOrUpdateLimits(list);
   else
      retVal = this->quotaGroupLimits->addOrUpdateLimits(list);

   if (!retVal)
      log.log(Log_ERR, "Could not update quota limits.");

   this->calculateExceededQuota();

   return retVal;
}

/**
 * requests the quota data from the storage servers and updates the quota data store
 */
bool QuotaManager::updateUsedQuota()
{
   bool retValUser = false;
   bool retValGroup = false;

   Config* cfg = Program::getApp()->getConfig();

   QuotaDataRequestor* requestorUser;
   QuotaDataRequestor* requestorGroup;

   if(cfg->getQuotaQueryTypeNum() == MgmtQuotaQueryType_SYSTEM)
   {
      requestorUser = new QuotaDataRequestor(QuotaDataType_USER);
      requestorGroup = new QuotaDataRequestor(QuotaDataType_GROUP);
   }
   else
   {
      requestorUser = new QuotaDataRequestor(QuotaDataType_USER,
         cfg->getQuotaQueryUIDRangeStart(), cfg->getQuotaQueryUIDRangeEnd());
      requestorGroup = new QuotaDataRequestor(QuotaDataType_GROUP,
         cfg->getQuotaQueryGIDRangeStart(), cfg->getQuotaQueryGIDRangeEnd());
   }


   retValUser = requestorUser->requestQuota(this->usedQuotaUser, &this->usedQuotaUserRWLock,
      &this->usedQuotaUserStoreDirty);

   if (!retValUser)
      log.log(Log_ERR, "Could not update user quota data.");


   retValGroup = requestorGroup->requestQuota(this->usedQuotaGroup, &this->usedQuotaGroupRWLock,
      &this->usedQuotaGroupStoreDirty);

   if (!retValGroup)
      log.log(Log_ERR, "Could not update group quota data.");


   SAFE_DELETE(requestorUser);
   SAFE_DELETE(requestorGroup);

   return (retValUser && retValGroup);
}

/**
 * calculates the IDs with exceeded quota and stores the IDs in the exceeded quota store
 */
void QuotaManager::calculateExceededQuota()
{
   UIntList exceededUIDsSize;
   UIntList exceededUIDsInodes;

   QuotaDataMap userLimits;
   this->quotaUserLimits->getAllQuotaLimits(&userLimits);

   // calculate exceeded quota for users
   SafeRWLock lockUser(&this->usedQuotaUserRWLock, SafeRWLock_READ);        // R E A D L O C K

   for(QuotaDataMapIter iter = userLimits.begin(); iter != userLimits.end(); iter++)
   {
      if(isQuotaExceededForIDunlocked(iter->first, QuotaLimitType_SIZE, this->usedQuotaUser,
         &userLimits) )
         exceededUIDsSize.push_back(iter->first);

      if(isQuotaExceededForIDunlocked(iter->first, QuotaLimitType_INODE,  this->usedQuotaUser,
         &userLimits) )
         exceededUIDsInodes.push_back(iter->first);
   }

   lockUser.unlock();                                                       // U N L O C K

   this->exceededQuotaStore->updateExceededQuota(&exceededUIDsSize, QuotaDataType_USER,
      QuotaLimitType_SIZE);
   this->exceededQuotaStore->updateExceededQuota(&exceededUIDsInodes, QuotaDataType_USER,
      QuotaLimitType_INODE);


   UIntList listGIDs;
   UIntList exceededGIDsSize;
   UIntList exceededGIDsInodes;

   QuotaDataMap groupLimits;
   this->quotaGroupLimits->getAllQuotaLimits(&groupLimits);

   SafeRWLock lockGroup(&this->usedQuotaGroupRWLock, SafeRWLock_READ);       // R E A D L O C K

   for(QuotaDataMapIter iter = groupLimits.begin(); iter != groupLimits.end(); iter++)
   {
      if(isQuotaExceededForIDunlocked(iter->first, QuotaLimitType_SIZE, this->usedQuotaGroup,
         &groupLimits) )
         exceededGIDsSize.push_back(iter->first);

      if(isQuotaExceededForIDunlocked(iter->first, QuotaLimitType_INODE, this->usedQuotaGroup,
         &groupLimits) )
         exceededGIDsInodes.push_back(iter->first);
   }

   lockGroup.unlock();                                                       // U N L O C K

   this->exceededQuotaStore->updateExceededQuota(&exceededGIDsSize, QuotaDataType_GROUP,
      QuotaLimitType_SIZE);
   this->exceededQuotaStore->updateExceededQuota(&exceededGIDsInodes, QuotaDataType_GROUP,
      QuotaLimitType_INODE);
}

/**
 * checks if the quota is exceeded for a single ID, not synchronized
 *
 * @param id the ID to check
 * @param limitType the limit type, inode or size
 * @param usedQuota the quota data store
 * @param limits the quota limits
 *
 * @return true if the quota is exceeded for the given ID
 */
bool QuotaManager::isQuotaExceededForIDunlocked(unsigned id, QuotaLimitType limitType,
   QuotaDataMapForTarget* usedQuota, QuotaDataMap* limits)
{
   bool retVal = false;
   uint64_t usedValue = 0;
   uint64_t limitValue = 0;

   for(QuotaDataMapForTargetIter iter = usedQuota->begin(); iter != usedQuota->end(); iter++)
   {
      QuotaDataMapIter quotaDataIter = iter->second.find(id);
      if(quotaDataIter != iter->second.end() )
      {
         QuotaData quotaDataUsed = quotaDataIter->second;

         if(limitType == QuotaLimitType_SIZE)
            usedValue += quotaDataUsed.getSize();
         else if(limitType == QuotaLimitType_INODE)
            usedValue += quotaDataUsed.getInodes();
      }
   }

   QuotaDataMapIter quotaLimitsIter = limits->find(id);
   if(quotaLimitsIter != limits->end() )
   {
      if(limitType == QuotaLimitType_SIZE)
         limitValue = quotaLimitsIter->second.getSize();
      else if(limitType == QuotaLimitType_INODE)
         limitValue = quotaLimitsIter->second.getInodes();
   }

   if(limitValue == 0)
      retVal = false;
   else if(limitValue <= usedValue)
      retVal = true;

   return retVal;
}

/**
 * transfers the exceeded quota IDs to the storage servers
 */
bool QuotaManager::pushExceededQuotaIDs()
{
   bool retVal = true;

   App* app = Program::getApp();
   ExceededQuotaStore* exQuotaStore = app->getQuotaManager()->getExceededQuotaStore();
   NodeStoreServers* storageNodes = app->getStorageNodes();
   MultiWorkQueue* workQ = app->getWorkQueue();

   int numWorks = 0;
   SynchronizedCounter counter;

   IntVector nodeResults;

   UIntList exceededQuotaUIDSize;
   exQuotaStore->getExceededQuota(&exceededQuotaUIDSize, QuotaDataType_USER,
      QuotaLimitType_SIZE);
   int maxMsgCountUIDSize = getMaxMessageCountForPushExceededQuota(&exceededQuotaUIDSize);

   UIntList exceededQuotaUIDInodes;
   exQuotaStore->getExceededQuota(&exceededQuotaUIDInodes, QuotaDataType_USER,
      QuotaLimitType_INODE);
   int maxMsgCountUIDInodes = getMaxMessageCountForPushExceededQuota(&exceededQuotaUIDInodes);

   UIntList exceededQuotaGIDSize;
   exQuotaStore->getExceededQuota(&exceededQuotaGIDSize, QuotaDataType_GROUP,
      QuotaLimitType_SIZE);
   int maxMsgCountGIDSize = getMaxMessageCountForPushExceededQuota(&exceededQuotaGIDSize);

   UIntList exceededQuotaGIDInodes;
   exQuotaStore->getExceededQuota(&exceededQuotaGIDInodes, QuotaDataType_GROUP,
      QuotaLimitType_INODE);
   int maxMsgCountGIDInodes = getMaxMessageCountForPushExceededQuota(&exceededQuotaGIDInodes);

   // use IntVector because it doesn't work with BoolVector. std::vector<bool> is not a container
   nodeResults = IntVector( (maxMsgCountUIDSize + maxMsgCountUIDInodes + maxMsgCountGIDSize +
      maxMsgCountGIDInodes) * storageNodes->getSize() );


   //send the 4 ID list with exceeded quota to the storage servers

   // upload all subranges (=> msg size limitation) for user size limits
   for(int messageNumber = 0; messageNumber < maxMsgCountUIDSize; messageNumber++)
   {
      Node* storageNode = storageNodes->referenceFirstNode();

      while(storageNode)
      {
         Work* work = new SetExceededQuotaWork(QuotaDataType_USER, QuotaLimitType_SIZE,
            storageNode, messageNumber, &exceededQuotaUIDSize, &counter, &nodeResults[numWorks]);
         workQ->addDirectWork(work);

         numWorks++;

         storageNode = storageNodes->referenceNextNodeAndReleaseOld(storageNode);
      }
   }

   // upload all subranges (=> msg size limitation) for user inode limits
   for(int messageNumber = 0; messageNumber < maxMsgCountUIDInodes; messageNumber++)
   {
      Node* storageNode = storageNodes->referenceFirstNode();

      while(storageNode)
      {
         Work* work = new SetExceededQuotaWork(QuotaDataType_USER, QuotaLimitType_INODE,
            storageNode, messageNumber, &exceededQuotaUIDInodes, &counter, &nodeResults[numWorks]);
         workQ->addDirectWork(work);

         numWorks++;

         storageNode = storageNodes->referenceNextNodeAndReleaseOld(storageNode);
      }
   }

   // upload all subranges (=> msg size limitation) for group size limits
   for(int messageNumber = 0; messageNumber < maxMsgCountGIDSize; messageNumber++)
   {
      Node* storageNode = storageNodes->referenceFirstNode();

      while(storageNode)
      {
         Work* work = new SetExceededQuotaWork(QuotaDataType_GROUP, QuotaLimitType_SIZE,
            storageNode, messageNumber, &exceededQuotaGIDSize, &counter, &nodeResults[numWorks]);
         workQ->addDirectWork(work);

         numWorks++;

         storageNode = storageNodes->referenceNextNodeAndReleaseOld(storageNode);
      }
   }

   // upload all subranges (=> msg size limitation) for group inode limits
   for(int messageNumber = 0; messageNumber < maxMsgCountGIDInodes; messageNumber++)
   {
      Node* storageNode = storageNodes->referenceFirstNode();

      while(storageNode)
      {
         Work* work = new SetExceededQuotaWork(QuotaDataType_GROUP, QuotaLimitType_INODE,
            storageNode, messageNumber, &exceededQuotaGIDInodes, &counter, &nodeResults[numWorks]);
         workQ->addDirectWork(work);

         numWorks++;

         storageNode = storageNodes->referenceNextNodeAndReleaseOld(storageNode);
      }
   }

   // wait for all work to be done
   counter.waitForCount(numWorks);

   for (IntVectorIter iter = nodeResults.begin(); iter != nodeResults.end(); iter++)
   {
      if (*iter == false)
         retVal = false;
   }

   return retVal;
}

/**
 * calculate the number of massages which are required to upload all exceeded quota IDs
 *
 * @param listToSend the list with the IDs to send
 */
int QuotaManager::getMaxMessageCountForPushExceededQuota(UIntList* listToSend)
{
   int retVal = 1;

   retVal = listToSend->size() / SETEXCEEDEDQUOTAMSG_MAX_ID_COUNT;

   if( (listToSend->size() % SETEXCEEDEDQUOTAMSG_MAX_ID_COUNT) != 0)
      retVal++;

   // at least on on message is required to reset the exceeded quota IDs
   if(retVal == 0)
      retVal++;

   return retVal;
}

/**
 * saves the quota data on the hard-drives
 */
bool QuotaManager::saveQuotaData()
{
   bool retVal = true;

   if(this->quotaUserLimits->isStoreDirty() )
   {
      if(this->quotaUserLimits->saveToFile() )
         log.log(Log_DEBUG, "Saved quota user limits " +
            StringTk::intToStr(quotaUserLimits->getSize()) + " to file " +
            CONFIG_QUOTA_USER_LIMITS_FILENAME);
      else
         retVal = false;
   }


   if(this->quotaGroupLimits->isStoreDirty() )
   {
      if(this->quotaGroupLimits->saveToFile() )
         log.log(Log_DEBUG, "Saved quota group limits " +
            StringTk::intToStr(quotaGroupLimits->getSize()) + " to file " +
            CONFIG_QUOTA_GROUP_LIMITS_FILENAME);
      else
         retVal = false;
   }


   if(this->usedQuotaUserStoreDirty)
   {
      SafeRWLock lockUser(&this->usedQuotaUserRWLock, SafeRWLock_READ);           // R E A D L O C K

      if(QuotaData::saveQuotaDataMapForTargetToFile(this->usedQuotaUser,
         CONFIG_QUOTA_DATA_USER_FILENAME) )
      {
         log.log(Log_SPAM, "Saved quota data for users of " +
            StringTk::intToStr(this->usedQuotaUser->size()) + " targets to file " +
            CONFIG_QUOTA_DATA_USER_FILENAME);

         this->usedQuotaUserStoreDirty = false;
      }
      else
         retVal = false;

      lockUser.unlock();                                                          // U N L O C K
   }


   if(this->usedQuotaGroupStoreDirty)
   {
      SafeRWLock lockGroup(&this->usedQuotaGroupRWLock, SafeRWLock_READ);         // R E A D L O C K

      if(QuotaData::saveQuotaDataMapForTargetToFile(this->usedQuotaGroup,
         CONFIG_QUOTA_DATA_GROUP_FILENAME) )
      {
         log.log(Log_SPAM, "Saved quota data for groups of " +
            StringTk::intToStr(this->usedQuotaGroup->size()) + " targets to file " +
            CONFIG_QUOTA_DATA_GROUP_FILENAME);

         this->usedQuotaGroupStoreDirty = false;
      }
      else
         retVal = false;

      lockGroup.unlock();                                                         // U N L O C K
   }

   return retVal;
}

/**
 * loads the quota data from the hard-drives
 */
bool QuotaManager::loadQuotaData()
{
   bool retVal = true;

   std::string mgmtStorePath = Program::getApp()->getConfig()->getStoreMgmtdDirectory();

   if(quotaUserLimits->loadFromFile() )
      log.log(Log_NOTICE, "Loaded quota user limits: " +
         StringTk::intToStr(this->quotaUserLimits->getSize()) + " from file " +
         this->quotaUserLimits->getStorePath() );
   else
      retVal = false;


   if(quotaGroupLimits->loadFromFile() )
      log.log(Log_NOTICE, "Loaded quota group limits: " +
         StringTk::intToStr(this->quotaGroupLimits->getSize()) + " from file " +
         this->quotaGroupLimits->getStorePath() );
   else
      retVal = false;


   SafeRWLock lockUser(&this->usedQuotaUserRWLock, SafeRWLock_WRITE);           // W R I T E L O C K

   Path usedQuotaUserPath(mgmtStorePath + "/" + CONFIG_QUOTA_DATA_USER_FILENAME);
   if(QuotaData::loadQuotaDataMapForTargetFromFile(this->usedQuotaUser,
      usedQuotaUserPath.getPathAsStr() ) )
      log.log(Log_NOTICE, "Loaded quota data for users of " +
         StringTk::intToStr(this->usedQuotaUser->size()) + " targets from file " +
         usedQuotaUserPath.getPathAsStr() );
   else
      retVal = false;

   this->usedQuotaUserStoreDirty = false;

   lockUser.unlock();                                                           // U N L O C K


   SafeRWLock lockGroup(&this->usedQuotaGroupRWLock, SafeRWLock_WRITE);         // W R I T E L O C K

   Path usedQuotaGroupPath(mgmtStorePath + "/" + CONFIG_QUOTA_DATA_GROUP_FILENAME);
   if(QuotaData::loadQuotaDataMapForTargetFromFile(this->usedQuotaGroup,
      usedQuotaGroupPath.getPathAsStr() ) )
      log.log(Log_NOTICE, "Loaded quota data for groups of " +
         StringTk::intToStr(this->usedQuotaGroup->size() ) + " targets from file " +
         usedQuotaGroupPath.getPathAsStr() );
   else
      retVal = false;

   this->usedQuotaGroupStoreDirty = false;

   lockGroup.unlock();                                                          // U N L O C K


   return retVal;
}

/**
 * returns the quota limit of a given id
 *
 * @param inOutQuotaData the input and output QuotaData for this method, the ID and the type must
 *        set before the method was called, the method updates the size and inode value of this
 *        QuotaData object with the limits for the given ID
 * @return true a limits was found
 */
bool QuotaManager::getQuotaLimitsForID(QuotaData& inOutQuotaData)
{
   bool retVal = false;

   if(inOutQuotaData.getType() == QuotaDataType_USER)
      retVal = this->quotaUserLimits->getQuotaLimit(inOutQuotaData);
   else
   if(inOutQuotaData.getType() == QuotaDataType_GROUP)
      retVal = this->quotaGroupLimits->getQuotaLimit(inOutQuotaData);

   return retVal;
}

/**
 *@param  mergeTargets
 */
std::string QuotaManager::usedQuotaUserToString(bool mergeTargets)
{
   std::ostringstream returnStringStream;

   SafeRWLock lockUser(&this->usedQuotaUserRWLock, SafeRWLock_READ);           // R E A D L O C K

   if(mergeTargets)
   {
      //merge the quota data from the targets
      QuotaDataMap mergedQuota;

      for(QuotaDataMapForTargetIter iter = this->usedQuotaUser->begin();
         iter != this->usedQuotaUser->end(); iter++)
      {
         for(QuotaDataMapIter idIter = iter->second.begin(); idIter != iter->second.end(); idIter++)
         {
            QuotaDataMapIter foundIter = mergedQuota.find(idIter->first);
            if (foundIter != mergedQuota.end() )
               foundIter->second.mergeQuotaDataCounter(&idIter->second);
            else
               mergedQuota.insert(*idIter);
         }
      }
      QuotaData::quotaDataMapToString(&mergedQuota, QuotaDataType_USER, &returnStringStream);
   }
   else
      QuotaData::quotaDataMapForTargetToString(this->usedQuotaUser, QuotaDataType_USER,
         &returnStringStream);

   lockUser.unlock();                                                          // U N L O C K

   return returnStringStream.str();
}

/**
 *@param  mergeTargets
 */
std::string QuotaManager::usedQuotaGroupToString(bool mergeTargets)
{
   std::ostringstream returnStringStream;

   SafeRWLock lockGroup(&this->usedQuotaGroupRWLock, SafeRWLock_READ);           // R E A D L O C K

   if(mergeTargets)
   {
      //merge the quota data from the targets
      QuotaDataMap mergedQuota;

      for(QuotaDataMapForTargetIter iter = this->usedQuotaGroup->begin();
         iter != this->usedQuotaGroup->end(); iter++)
      {
         for(QuotaDataMapIter idIter = iter->second.begin(); idIter != iter->second.end(); idIter++)
         {
            QuotaDataMapIter foundIter = mergedQuota.find(idIter->first);
            if (foundIter != mergedQuota.end() )
               foundIter->second.mergeQuotaDataCounter(&idIter->second);
            else
               mergedQuota.insert(*idIter);
         }
      }
      QuotaData::quotaDataMapToString(&mergedQuota, QuotaDataType_GROUP, &returnStringStream);
   }
   else
      QuotaData::quotaDataMapForTargetToString(this->usedQuotaGroup, QuotaDataType_GROUP,
         &returnStringStream);

   lockGroup.unlock();                                                          // U N L O C K

   return returnStringStream.str();;
}
