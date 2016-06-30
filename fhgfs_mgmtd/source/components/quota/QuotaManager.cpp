#include <app/App.h>
#include <common/threading/PThread.h>
#include <common/threading/SafeRWLock.h>
#include <common/toolkit/SynchronizedCounter.h>
#include <common/storage/quota/QuotaData.h>
#include <components/worker/SetExceededQuotaWork.h>
#include <program/Program.h>
#include "QuotaManager.h"


#include <fstream>
#include <iostream>
#include <sys/file.h>


const unsigned QUOTAMANAGER_READ_ID_FILE_RETRY = 3;



/**
 * Constructor.
 */
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
   loadQuotaData();
}

/**
 * Destructor.
 */
QuotaManager::~QuotaManager()
{
   SAFE_DELETE(this->usedQuotaUser);
   SAFE_DELETE(this->usedQuotaGroup);

   SAFE_DELETE(this->quotaUserLimits);
   SAFE_DELETE(this->quotaGroupLimits);

   SAFE_DELETE(this->exceededQuotaStore);
}

/**
 * Run method of the thread.
 */
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

/**
 * The implementation of the thread loop. It checks the used quota, calculate the exceeded quota IDs
 * and push the IDs to the servers.
 */
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
         bool doUpdate = true;
         UIntList uidList;
         UIntList gidList;

         if(cfg->getQuotaQueryTypeNum() == MgmtQuotaQueryType_FILE)
         {
            doUpdate = doUpdate && loadIdsFromFileWithReTry(cfg->getQuotaQueryUIDFile(),
               QUOTAMANAGER_READ_ID_FILE_RETRY, uidList);
            doUpdate = doUpdate && loadIdsFromFileWithReTry(cfg->getQuotaQueryGIDFile(),
               QUOTAMANAGER_READ_ID_FILE_RETRY, gidList);
         }

         if(doUpdate)
         {
            updateUsedQuota(uidList, gidList);
            calculateExceededQuota(uidList, gidList);
            limitChanged.compareAndSet(0, 1); // reset flag for updated limits
            pushExceededQuotaIDs();
            lastQuotaUsageUpdateT.setToNow();
         }
      }
      else if(limitChanged.compareAndSet(0, 1) )
      { // check if a limit was updated, if yes only calculate the the exceeded quota IDs and push
        // the IDs to the servers, do not update the used quota data for performance reason
         bool doUpdate = true;
         UIntList uidList;
         UIntList gidList;

         if(cfg->getQuotaQueryTypeNum() == MgmtQuotaQueryType_FILE)
         {
            doUpdate = doUpdate && loadIdsFromFileWithReTry(cfg->getQuotaQueryUIDFile(),
               QUOTAMANAGER_READ_ID_FILE_RETRY, uidList);
            doUpdate = doUpdate && loadIdsFromFileWithReTry(cfg->getQuotaQueryGIDFile(),
               QUOTAMANAGER_READ_ID_FILE_RETRY, gidList);
         }

         if(doUpdate)
         {
            calculateExceededQuota(uidList, gidList);
            pushExceededQuotaIDs();
         }
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
 * Removes a target from the quota data store, required for target unmap and remove node.
 *
 * @param targetNumID The targetNumID  of the target to remove.
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
 * Adds or updates the quota limits with the given list.
 *
 * @param List the list with the new quota limits.
 */
bool QuotaManager::updateQuotaLimits(QuotaDataList* list)
{
   bool retVal = false;

   if(list->begin()->getType() == QuotaDataType_USER)
      retVal = this->quotaUserLimits->addOrUpdateLimits(list);
   else
      retVal = this->quotaGroupLimits->addOrUpdateLimits(list);

   // the calculation of the exceeded quota is done in the run loop
   // Note: don't do the calculation here because the beegfs-ctl command doesn't finish before
   // the calculation and the update of the server is done in this case
   if(retVal)
      limitChanged.set(1);

   return retVal;
}

/**
 * Requests the quota data from the storage servers and updates the quota data store.
 *
 * @param uidList A list with the UIDs to update, contains values if the IDs are provided by a file.
 * @param gidList A list with the GIDs to update, contains values if the IDs are provided by a file.
 */
bool QuotaManager::updateUsedQuota(UIntList& uidList, UIntList& gidList)
{
   bool retValUser = false;
   bool retValGroup = false;

   Config* cfg = Program::getApp()->getConfig();

   QuotaDataRequestor* requestorUser;
   QuotaDataRequestor* requestorGroup;

   if(cfg->getQuotaQueryTypeNum() == MgmtQuotaQueryType_SYSTEM)
   {
      requestorUser = new QuotaDataRequestor(QuotaDataType_USER,
         cfg->getQuotaQueryWithSystemUsersGroups() );
      requestorGroup = new QuotaDataRequestor(QuotaDataType_GROUP,
         cfg->getQuotaQueryWithSystemUsersGroups() );
   }
   else if(cfg->getQuotaQueryTypeNum() == MgmtQuotaQueryType_FILE)
   {
      requestorUser = new QuotaDataRequestor(QuotaDataType_USER, uidList);
      requestorGroup = new QuotaDataRequestor(QuotaDataType_GROUP, gidList);
   }
   else
   {
      requestorUser = new QuotaDataRequestor(QuotaDataType_USER,
         cfg->getQuotaQueryUIDRangeStart(), cfg->getQuotaQueryUIDRangeEnd(),
         cfg->getQuotaQueryWithSystemUsersGroups() );
      requestorGroup = new QuotaDataRequestor(QuotaDataType_GROUP,
         cfg->getQuotaQueryGIDRangeStart(), cfg->getQuotaQueryGIDRangeEnd(),
         cfg->getQuotaQueryWithSystemUsersGroups() );
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
 * Calculates the IDs with exceeded quota and stores the IDs in the exceeded quota store.
 *
 * @param uidList A list with the UIDs to check, contains values if the IDs are provided by a file.
 * @param gidList A list with the GIDs to check, contains values if the IDs are provided by a file.
 */
void QuotaManager::calculateExceededQuota(UIntList& uidList, UIntList& gidList)
{
   Config* cfg = Program::getApp()->getConfig();

   UIntList exceededUIDsSize;
   UIntList exceededUIDsInodes;

   QuotaDataMap userLimits;
   this->quotaUserLimits->getAllQuotaLimits(&userLimits);

   bool defaultLimitsConfigured = (defaultLimits.getDefaultUserQuotaInodes() ||
      defaultLimits.getDefaultUserQuotaSize() || defaultLimits.getDefaultGroupQuotaInodes() ||
      defaultLimits.getDefaultGroupQuotaSize() );

   if(defaultLimitsConfigured)
   { // default quota is configured, get all IDs to check
      if(cfg->getQuotaQueryTypeNum() == MgmtQuotaQueryType_SYSTEM)
      {
         System::getAllUserIDs(&uidList, !cfg->getQuotaQueryWithSystemUsersGroups() );
         System::getAllGroupIDs(&gidList, !cfg->getQuotaQueryWithSystemUsersGroups() );
      }
   }

   // calculate exceeded quota for users
   SafeRWLock lockUser(&this->usedQuotaUserRWLock, SafeRWLock_READ);        // R E A D L O C K

   if(defaultLimitsConfigured)
   { // default quota is configured, requires to check every UID
      if( (cfg->getQuotaQueryTypeNum() == MgmtQuotaQueryType_SYSTEM) ||
         (cfg->getQuotaQueryTypeNum() == MgmtQuotaQueryType_FILE) )
      { // use the users of the system
         for(UIntListIter iter = uidList.begin(); iter != uidList.end(); iter++)
         {
            if(isQuotaExceededForIDunlocked(*iter, QuotaLimitType_SIZE, usedQuotaUser, &userLimits,
               defaultLimits.getDefaultUserQuotaSize() ) )
               exceededUIDsSize.push_back(*iter);

            if(isQuotaExceededForIDunlocked(*iter, QuotaLimitType_INODE, usedQuotaUser, &userLimits,
               defaultLimits.getDefaultUserQuotaInodes() ) )
               exceededUIDsInodes.push_back(*iter);
         }
      }
      else
      { // use the users from a configured range
         for(unsigned index = cfg->getQuotaQueryUIDRangeStart();
            index <= cfg->getQuotaQueryUIDRangeEnd(); index++)
         {
            if(isQuotaExceededForIDunlocked(index, QuotaLimitType_SIZE, usedQuotaUser, &userLimits,
               defaultLimits.getDefaultUserQuotaSize() ) )
               exceededUIDsSize.push_back(index);

            if(isQuotaExceededForIDunlocked(index, QuotaLimitType_INODE, usedQuotaUser, &userLimits,
               defaultLimits.getDefaultUserQuotaInodes() ) )
               exceededUIDsInodes.push_back(index);
         }
      }
   }
   else
   { // no default quota configured, check only UIDs which has a configured limit
      for(QuotaDataMapIter iter = userLimits.begin(); iter != userLimits.end(); iter++)
      {
         if(isQuotaExceededForIDunlocked(iter->first, QuotaLimitType_SIZE, usedQuotaUser,
            &userLimits, defaultLimits.getDefaultUserQuotaSize() ) )
            exceededUIDsSize.push_back(iter->first);

         if(isQuotaExceededForIDunlocked(iter->first, QuotaLimitType_INODE, usedQuotaUser,
            &userLimits, defaultLimits.getDefaultUserQuotaInodes() ) )
            exceededUIDsInodes.push_back(iter->first);
      }
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

   // calculate exceeded quota for groups
   SafeRWLock lockGroup(&this->usedQuotaGroupRWLock, SafeRWLock_READ);       // R E A D L O C K

   if(defaultLimitsConfigured)
   { // default quota is configured, requires to check every GID
      if( (cfg->getQuotaQueryTypeNum() == MgmtQuotaQueryType_SYSTEM) ||
         (cfg->getQuotaQueryTypeNum() == MgmtQuotaQueryType_FILE) )
      { // use the groups of the system
         for(UIntListIter iter = gidList.begin(); iter != gidList.end(); iter++)
         {
            if(isQuotaExceededForIDunlocked(*iter, QuotaLimitType_SIZE, usedQuotaGroup,
               &groupLimits, defaultLimits.getDefaultGroupQuotaSize() ) )
               exceededGIDsSize.push_back(*iter);

            if(isQuotaExceededForIDunlocked(*iter, QuotaLimitType_INODE, usedQuotaGroup,
               &groupLimits, defaultLimits.getDefaultGroupQuotaInodes() ) )
               exceededGIDsInodes.push_back(*iter);
         }
      }
      else
      { // use the groups from a configured range
         for(unsigned index = cfg->getQuotaQueryGIDRangeStart();
            index <= cfg->getQuotaQueryGIDRangeEnd(); index++)
         {
            if(isQuotaExceededForIDunlocked(index, QuotaLimitType_SIZE, usedQuotaGroup,
               &groupLimits, defaultLimits.getDefaultGroupQuotaSize() ) )
               exceededGIDsSize.push_back(index);

            if(isQuotaExceededForIDunlocked(index, QuotaLimitType_INODE, usedQuotaGroup,
               &groupLimits, defaultLimits.getDefaultGroupQuotaInodes() ) )
               exceededGIDsInodes.push_back(index);
         }
      }
   }
   else
   { // no default quota configured, check only GIDs which has a configured limit
      for(QuotaDataMapIter iter = groupLimits.begin(); iter != groupLimits.end(); iter++)
      {
         if(isQuotaExceededForIDunlocked(iter->first, QuotaLimitType_SIZE, usedQuotaGroup,
            &groupLimits, defaultLimits.getDefaultGroupQuotaSize() ) )
            exceededGIDsSize.push_back(iter->first);

         if(isQuotaExceededForIDunlocked(iter->first, QuotaLimitType_INODE, usedQuotaGroup,
            &groupLimits, defaultLimits.getDefaultGroupQuotaInodes() ) )
            exceededGIDsInodes.push_back(iter->first);
      }
   }

   lockGroup.unlock();                                                       // U N L O C K

   this->exceededQuotaStore->updateExceededQuota(&exceededGIDsSize, QuotaDataType_GROUP,
      QuotaLimitType_SIZE);
   this->exceededQuotaStore->updateExceededQuota(&exceededGIDsInodes, QuotaDataType_GROUP,
      QuotaLimitType_INODE);
}

/**
 * Checks if the quota is exceeded for a single ID, not synchronized.
 *
 * @param id The ID to check.
 * @param limitType The limit type, inode or size.
 * @param usedQuota The quota data store.
 * @param limits The quota limits.
 * @return True if the quota is exceeded for the given ID.
 */
bool QuotaManager::isQuotaExceededForIDunlocked(unsigned id, QuotaLimitType limitType,
   QuotaDataMapForTarget* usedQuota, QuotaDataMap* limits, size_t defaultLimit)
{
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


   if(limitValue != 0)
      return limitValue <= usedValue;
   else
      return (defaultLimit != 0) && (defaultLimit <= usedValue);
}

/**
 * Transfers the exceeded quota IDs to the storage servers.
 */
bool QuotaManager::pushExceededQuotaIDs()
{
   bool retVal = true;

   App* app = Program::getApp();
   ExceededQuotaStore* exQuotaStore = app->getQuotaManager()->getExceededQuotaStore();
   MultiWorkQueue* workQ = app->getWorkQueue();

   NodeList storageNodeList;
   app->getStorageNodes()->referenceAllNodes(&storageNodeList);

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
      maxMsgCountGIDInodes) * storageNodeList.size() );

   //send the 4 ID list with exceeded quota to the storage servers

   // upload all subranges (=> msg size limitation) for user size limits
   for(int messageNumber = 0; messageNumber < maxMsgCountUIDSize; messageNumber++)
   {
      for (NodeListIter nodeIter = storageNodeList.begin();
            nodeIter != storageNodeList.end(); nodeIter++)
      {
         Work* work = new SetExceededQuotaWork(QuotaDataType_USER, QuotaLimitType_SIZE,
            *nodeIter, messageNumber, &exceededQuotaUIDSize, &counter, &nodeResults[numWorks]);
         workQ->addDirectWork(work);

         numWorks++;
      }
   }

   // upload all subranges (=> msg size limitation) for user inode limits
   for(int messageNumber = 0; messageNumber < maxMsgCountUIDInodes; messageNumber++)
   {
      for (NodeListIter nodeIter = storageNodeList.begin();
            nodeIter != storageNodeList.end(); nodeIter++)
      {
         Work* work = new SetExceededQuotaWork(QuotaDataType_USER, QuotaLimitType_INODE,
            *nodeIter, messageNumber, &exceededQuotaUIDInodes, &counter, &nodeResults[numWorks]);
         workQ->addDirectWork(work);

         numWorks++;
      }
   }

   // upload all subranges (=> msg size limitation) for group size limits
   for(int messageNumber = 0; messageNumber < maxMsgCountGIDSize; messageNumber++)
   {
      for (NodeListIter nodeIter = storageNodeList.begin();
            nodeIter != storageNodeList.end(); nodeIter++)
      {
         Work* work = new SetExceededQuotaWork(QuotaDataType_GROUP, QuotaLimitType_SIZE,
            *nodeIter, messageNumber, &exceededQuotaGIDSize, &counter, &nodeResults[numWorks]);
         workQ->addDirectWork(work);

         numWorks++;
      }
   }

   // upload all subranges (=> msg size limitation) for group inode limits
   for(int messageNumber = 0; messageNumber < maxMsgCountGIDInodes; messageNumber++)
   {
      for (NodeListIter nodeIter = storageNodeList.begin();
            nodeIter != storageNodeList.end(); nodeIter++)
      {
         Work* work = new SetExceededQuotaWork(QuotaDataType_GROUP, QuotaLimitType_INODE,
            *nodeIter, messageNumber, &exceededQuotaGIDInodes, &counter, &nodeResults[numWorks]);
         workQ->addDirectWork(work);

         numWorks++;
      }
   }

   // wait for all work to be done
   counter.waitForCount(numWorks);

   app->getStorageNodes()->releaseAllNodes(&storageNodeList);

   for (IntVectorIter iter = nodeResults.begin(); iter != nodeResults.end(); iter++)
   {
      if (*iter == false)
         retVal = false;
   }

   return retVal;
}

/**
 * Calculate the number of massages which are required to upload all exceeded quota IDs.
 *
 * @param listToSend The list with the IDs to send.
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
 * Saves the quota data on the hard-drives.
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

   if(!saveDefaultQuotaToFile() )
      retVal = false;


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
 * Loads the quota data from the hard-drives.
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


   if(!loadDefaultQuotaFromFile() )
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
 * Returns the quota limit of a given id.
 *
 * @param inOutQuotaData The input and output QuotaData for this method, the ID and the type must
 *        set before the method was called, the method updates the size and inode value of this
 *        QuotaData object with the limits for the given ID.
 * @return True a limits was found.
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
 * Creates a string which contains all used quota data of the users. The string could be printed to
 * the console.
 *
 * NOTE: It is just used by the GenericDebugMsg.
 *
 *@param mergeTargets True if the quota data of all targets should be merged then for each ID only
 *       one used quota value is added to the string. If false for each target the ID with the used
 *       quota value is added to the string.
 * @return A string which contains all used quota.
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
 * Creates a string which contains all used quota data of the groups. The string could be printed to
 * the console.
 *
 * NOTE: It is just used by the GenericDebugMsg.
 *
 * @param mergeTargets True if the quota data of all targets should be merged then for each ID only
 *        one used quota value is added to the string. If false for each target the ID with the used
 *        quota value is added to the string.
 * @return A string which contains all used quota.
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

   return returnStringStream.str();
}

/**
 * Stores the default quota to a file.
 *
 * @return True if the quota data could stored successful to the file.
 */
bool QuotaManager::saveDefaultQuotaToFile()
{
   LogContext log("SaveDefaultQuota");

   bool retVal = false;

   const std::string path = Program::getApp()->getConfig()->getStoreMgmtdDirectory() + "/" +
      CONFIG_QUOTA_DEFAULT_LIMITS;

   char* buf;
   unsigned bufLen;
   ssize_t writeRes;

   if(!path.length() )
      return false;

   Path quotaDataPath(path);
   if(!StorageTk::createPathOnDisk(quotaDataPath, true) )
   {
      log.logErr("Unable to create directory for quota data: " + quotaDataPath.getPathAsStr() );
      return false;
   }

   // create/trunc file
   int openFlags = O_CREAT|O_TRUNC|O_WRONLY;

   int fd = open(path.c_str(), openFlags, 0666);
   if(fd == -1)
   { // error
      log.logErr("Unable to create default quota limits file: " + path + ". " +
         "SysErr: " + System::getErrString() );

      return false;
   }

   // file created => store data
   buf = (char*)malloc(defaultLimits.serialLen() );
   if(!buf)
   {
      log.logErr("Unable to allocate memory to write the default limits file: " + path + ". " +
         "SysErr: " + System::getErrString() );

      goto err_closefile;
   }

   bufLen = defaultLimits.serialize(buf);
   writeRes = write(fd, buf, bufLen);
   free(buf);

   if(writeRes != (ssize_t)bufLen)
   {
      log.logErr("Unable to store default quota limits file: " + path + ". " +
         "SysErr: " + System::getErrString() );

      goto err_closefile;
   }

   retVal = true;

   // error compensation
err_closefile:
   close(fd);

   return retVal;
}

/**
 * Reads the default quota from a file.
 *
 * @return True if the quota data could load successful from the file.
 */
bool QuotaManager::loadDefaultQuotaFromFile()
{
   LogContext log("LoadDefaultQuota");

   bool retVal = false;

   const std::string path = Program::getApp()->getConfig()->getStoreMgmtdDirectory() + "/" +
      CONFIG_QUOTA_DEFAULT_LIMITS;

   char* buf = NULL;
   size_t readRes;

   struct stat statBuf;
   int retValStat;

   if(!path.length() )
      return false;

   int fd = open(path.c_str(), O_RDONLY, 0);
   if(fd == -1)
   { // open failed
      log.log(Log_DEBUG, "Unable to open default limits file: " + path + ". " +
         "SysErr: " + System::getErrString() );

      return false;
   }

   retValStat = fstat(fd, &statBuf);
   if(retValStat)
   { // stat failed
      log.log(Log_WARNING, "Unable to stat default limits file: " + path + ". " +
         "SysErr: " + System::getErrString() );

      goto err_closefile;
   }

   buf = (char*)malloc(statBuf.st_size);
   if(!buf)
   { // malloc failed
      log.log(Log_WARNING, "Unable to allocate memory to read the default limits file: " + path +
         ". " + "SysErr: " + System::getErrString() );

      goto err_closefile;
   }

   readRes = read(fd, buf, statBuf.st_size);
   if(readRes <= 0)
   { // reading failed
      log.log(Log_WARNING, "Unable to read default limits file: " + path + ". " +
         "SysErr: " + System::getErrString() );
   }
   else
   { // parse contents
      unsigned outLen;

      retVal = defaultLimits.deserialize(buf, readRes, &outLen);
   }

   if (retVal == false)
      log.logErr("Could not deserialize limits from file: " + path);

   free(buf);

   retVal = true;

   // error compensation
err_closefile:
   close(fd);

   return retVal;
}

/**
 * Loads UIDs or GIDs from a file. In case of error it does some retries.
 *
 * NOTE: Normally use this function instead of loadIdsFromFile() because it is allowed to change the
 * ID file during a running system. So error can happen, so we should do a re-try to check if it is
 * a temporary problem.
 *
 * @param path The file to read from.
 * @param numRetries The number of retries to do, if a error happens.
 * @param outIdList The list which contains all IDs from the file afterwards.
 * @return True if the IDs could load successful from the file.
 */
bool QuotaManager::loadIdsFromFileWithReTry(const std::string& path, unsigned numRetries,
   UIntList& outIdList)
{
   bool retVal = false;

   retVal = loadIdsFromFile(path, outIdList);
   while(!retVal && (numRetries > 0) )
   {
      retVal = loadIdsFromFile(path, outIdList);
      --numRetries;
   }

   return retVal;
}

/**
 * Loads UIDs or GIDs from a file.
 *
 * NOTE: Normally use the function loadIdsFromFileWithReTry() instead of this because it is allowed
 * to change the ID file during a running system. So error can happen, so we should do a re-try to
 * check if it is a temporary problem.
 *
 * @param path The file to read from.
 * @param outIdList The list which contains all IDs from the file afterwards.
 * @return True if the IDs could load successful from the file.
 */
bool QuotaManager::loadIdsFromFile(const std::string& path, UIntList& outIdList)
{
   LogContext log("LoadIDsFromFile");

   bool retVal = false;

   if(!path.length() )
      return false;

   std::ifstream reader(path.c_str(), std::ifstream::in);

   if(reader.is_open() )
   {
      unsigned newId;
      reader >> newId;

      while(reader.good() )
      {
         outIdList.push_back(newId);
         reader >> newId;
      }

      if(reader.eof() )
      {  // add the last ID from the file
         outIdList.push_back(newId);
         retVal = true;
      }
      else
      {
         log.log(Log_ERR, "Error during read file with UIDs or GIDs: " + path + ". " +
            "SysErr: " + System::getErrString() );
         retVal = false;
      }

      reader.close();
   }
   else
   {
      log.log(Log_ERR, "Unable to open file with UIDs or GIDs: " + path + ". " +
         "SysErr: " + System::getErrString() );
      retVal = false;
   }

   if(retVal)
   { // remove duplicated IDs
      outIdList.sort();
      outIdList.unique();
   }
   else
   { // in case of error clear the output list
      outIdList.clear();
   }

   return retVal;
}
