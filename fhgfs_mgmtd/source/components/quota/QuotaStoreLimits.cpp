/*
 * QuotaStoreLimits.cpp
 */


#include <common/app/log/LogContext.h>
#include <common/toolkit/StorageTk.h>
#include <program/Program.h>
#include "QuotaStoreLimits.h"


/**
 * Getter for the limits of a given ID. The limits are returned in the given QuotaData. The map is
 * read locked during the execution.
 *
 * @param quotaDataInOut the input and output QuotaData for this method, the ID and the type must
 *        set before the method was called, the method updates the size and inode value of this
 *        QuotaData object with the limits for the given ID
 * @return true a limits was found
 */
bool QuotaStoreLimits::getQuotaLimit(QuotaData& quotaDataInOut)
{
#ifdef BEEGFS_DEBUG
   if (quotaDataInOut.getType() != this->quotaType)
      LogContext(this->logContext).log(Log_ERR, "Given QuotaDataType doesn't match to the "
         "QuotaDataType of the store.");
#endif

   bool retVal = false;

   SafeRWLock rwLock(&this->limitsRWLock, SafeRWLock_READ);          // R E A D L O C K

   QuotaDataMapIter iter = this->limits.find(quotaDataInOut.getID() );
   if(iter != this->limits.end() )
   {
      uint64_t size = iter->second.getSize();
      uint64_t inodes = iter->second.getInodes();
      quotaDataInOut.setQuotaData(size, inodes);

      retVal = true;
   }

   rwLock.unlock();                                                  // U N L O C K

   return retVal;
}

/**
 * returns a map with all quota limits in the given map
 *
 * @param outMap contains all quota Limits
 */
void QuotaStoreLimits::getAllQuotaLimits(QuotaDataMap* outMap)
{
   SafeRWLock rwLock(&this->limitsRWLock, SafeRWLock_READ);          // R E A D L O C K

   for(QuotaDataMapIter iter = this->limits.begin(); iter !=this->limits.end(); iter++)
      outMap->insert(*iter);

   rwLock.unlock();                                                  // U N L O C K
}

/**
 * Updates or inserts QuotaData into the map. The map is write locked during the update. The store
 * is marked as dirty when the update or insert was successful.
 *
 * @param quotaData the QuotaData to insert or to update
 * @return true if limits successful updated or inserted
 */
bool QuotaStoreLimits::addOrUpdateLimit(QuotaData quotaData)
{
#ifdef BEEGFS_DEBUG
   if (quotaData.getType() != this->quotaType)
      LogContext(this->logContext).log(Log_ERR, "Given QuotaDataType doesn't match to the "
         "QuotaDataType of the store.");
#endif

   bool retVal;

   SafeRWLock safeRWLock(&this->limitsRWLock, SafeRWLock_WRITE); // W R I T E L O C K

   retVal = addOrUpdateLimitUnlocked(quotaData);
   this->storeDirty = retVal;

   safeRWLock.unlock(); // U N L O C K

   return retVal;
}

/**
 * Updates or inserts QuotaData into the map. The map is NOT locked during the update. The store is
 * marked as dirty when the update or insert was successful.
 *
 * @param quotaData the QuotaData to insert or to update
 * @return true if limits successful updated or inserted
 */
bool QuotaStoreLimits::addOrUpdateLimitUnlocked(QuotaData quotaData)
{
#ifdef BEEGFS_DEBUG
   if (quotaData.getType() != this->quotaType)
      LogContext(this->logContext).log(Log_ERR, "Given QuotaDataType doesn't match to the "
         "QuotaDataType of the store.");
#endif

   bool retVal;

   // remove quota limit if size and inode value is zero
   if((quotaData.getSize() == 0) && (quotaData.getInodes() == 0) )
   {
      this->limits.erase(quotaData.getID() );
      return true;
   }

   // update quota limit
   QuotaDataMapVal newQuotaMapValue(quotaData.getID(), quotaData);
   std::pair<QuotaDataMapIter, bool> retMapVal = this->limits.insert(newQuotaMapValue);
   if(!retMapVal.second)
   {
      this->limits.erase(retMapVal.first);
      retVal = (this->limits.insert(newQuotaMapValue)).second;
   }
   else
      retVal = true;

   return retVal;
}

/**
 * Updates or inserts a List of QuotaData into the map. The map is write locked during the update.
 * The store is marked as dirty when one or more updates or inserts were successful.
 *
 * @param quotaDataList the List of QuotaData to insert or to update
 * @return true if all limits successful updated or inserted, if one or more updates/inserts fails
 *         the return value is false
 */
bool QuotaStoreLimits::addOrUpdateLimits(QuotaDataList* quotaDataList)
{
   bool retVal = true;
   bool storeIsDirty = false;

   SafeRWLock safeRWLock(&this->limitsRWLock, SafeRWLock_WRITE); // W R I T E L O C K

   for (QuotaDataListIter iter = quotaDataList->begin(); iter != quotaDataList->end(); iter++)
   {
#ifdef BEEGFS_DEBUG
      if (iter->getType() != this->quotaType)
         LogContext(this->logContext).log(Log_ERR, "Given QuotaDataType doesn't match to the "
            "QuotaDataType of the store.");
#endif

      if(!addOrUpdateLimitUnlocked(*iter) )
         retVal = false;
      else
         storeIsDirty = true;
   }
   this->storeDirty = storeIsDirty;

   safeRWLock.unlock(); // U N L O C K

   return retVal;
}

/**
 * Loads the quota limits form a file. The map is write locked during the load. Marks the store as
 * clean also when an error occurs.
 *
 * @return true if limits are successful loaded
 */
bool QuotaStoreLimits::loadFromFile()
{
   LogContext log(this->logContext);

   bool retVal = false;
   char* buf = NULL;
   size_t readRes;

   struct stat statBuf;
   int retValStat;

   if(!this->storePath.length() )
      return false;

   SafeRWLock safeRWLock(&this->limitsRWLock, SafeRWLock_WRITE); // W R I T E L O C K

   int fd = open(this->storePath.c_str(), O_RDONLY, 0);
   if(fd == -1)
   { // open failed
      log.log(Log_DEBUG, "Unable to open limits file: " + this->storePath + ". " +
         "SysErr: " + System::getErrString() );

      goto err_unlock;
   }

   retValStat = fstat(fd, &statBuf);
   if(retValStat)
   { // stat failed
      log.log(Log_WARNING, "Unable to stat limits file: " + this->storePath + ". " +
         "SysErr: " + System::getErrString() );

      goto err_stat;
   }

   buf = (char*)malloc(statBuf.st_size);
   readRes = read(fd, buf, statBuf.st_size);
   if(readRes <= 0)
   { // reading failed
      log.log(Log_WARNING, "Unable to read limits file: " + this->storePath + ". " +
         "SysErr: " + System::getErrString() );
   }
   else
   { // parse contents
      unsigned outLen;
      unsigned mapElemeCount;
      const char* mapStart;

      retVal = Serialization::deserializeQuotaDataMapPreprocess(buf, readRes, &mapElemeCount,
         &mapStart, &outLen);

      if (retVal)
         retVal = Serialization::deserializeQuotaDataMap(readRes, mapElemeCount, mapStart,
            &this->limits);
   }

   if (retVal == false)
      log.logErr("Could not deserialize limits from file: " + this->storePath);

   free(buf);

err_stat:
   close(fd);

err_unlock:
   this->storeDirty = false;

   safeRWLock.unlock(); // U N L O C K

   return retVal;
}

/**
 * Stores the quota limits into a file. The map is write locked during the save. Marks the store as
 * clean when the limits are successful stored. If an error occurs the store is marked as dirty.
 *
 * @return true if the limits are successful stored
 */
bool QuotaStoreLimits::saveToFile()
{
   LogContext log(this->logContext);

   bool retVal = false;

   char* buf;
   unsigned bufLen;
   ssize_t writeRes;

   if(!this->storePath.length() )
      return false;

   SafeRWLock safeRWLock(&this->limitsRWLock, SafeRWLock_READ); // W R I T E L O C K

   Path quotaDataPath(this->storePath);
   if(!StorageTk::createPathOnDisk(quotaDataPath, true))
   {
      log.logErr("Unable to create directory for quota data: " + quotaDataPath.getPathAsStr() );
      return false;
   }

   // create/trunc file
   int openFlags = O_CREAT|O_TRUNC|O_WRONLY;

   int fd = open(this->storePath.c_str(), openFlags, 0666);
   if(fd == -1)
   { // error
      log.logErr("Unable to create limits file: " + this->storePath + ". " +
         "SysErr: " + System::getErrString() );

      goto err_unlock;
   }

   // file created => store data
   buf = (char*)malloc(Serialization::serialLenQuotaDataMap(&this->limits));
   bufLen = Serialization::serializeQuotaDataMap(buf, &this->limits);
   writeRes = write(fd, buf, bufLen);
   free(buf);

   if(writeRes != (ssize_t)bufLen)
   {
      log.logErr("Unable to store limits file: " + this->storePath + ". " +
         "SysErr: " + System::getErrString() );

      goto err_closefile;
   }

   retVal = true;

   // error compensation
err_closefile:
   close(fd);

err_unlock:

   this->storeDirty = !retVal;
   safeRWLock.unlock(); // U N L O C K

   return retVal;
}
