#include <common/app/log/LogContext.h>
#include <common/toolkit/serialization/Serialization.h>
#include <common/toolkit/StorageTk.h>
#include "QuotaData.h"



unsigned QuotaData::serialLen()
{

   return Serialization::serialLenUInt64() +    // size
          Serialization::serialLenUInt64() +    // inodes
          Serialization::serialLenUInt()   +    // id
          Serialization::serialLenInt()    +    // type
          Serialization::serialLenBool();       // valid
}

size_t QuotaData::serialize(char* buf)
{
   size_t bufPos = 0;

   // size, quotaData->dqb_bhardlimit
   bufPos += Serialization::serializeUInt64(&buf[bufPos], this->size);

   // inodes, quotaData->dqb_bsoftlimit
   bufPos += Serialization::serializeUInt64(&buf[bufPos], this->inodes);

   // id
   bufPos += Serialization::serializeUInt(&buf[bufPos], this->id);

   // type
   bufPos += Serialization::serializeInt(&buf[bufPos], this->type);

   // valid
   bufPos += Serialization::serializeBool(&buf[bufPos], this->valid);

   return bufPos;
}

bool QuotaData::deserialize(const char* buf, size_t bufLen, unsigned* outLen)
{
   size_t bufPos = 0;

   // size

   unsigned usedBlocksBufLen;

   if(!Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos,
      &this->size, &usedBlocksBufLen) )
      return false;

   bufPos += usedBlocksBufLen;

   // inodes

   unsigned usedInodesBufLen;

   if(!Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos,
      &this->inodes, &usedInodesBufLen) )
      return false;

   bufPos += usedInodesBufLen;

   // id

   unsigned idBufLen;

   if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
      &this->id, &idBufLen) )
      return false;

   bufPos += idBufLen;

   // type

   unsigned typeBufLen;

   if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos,
      &this->type, &typeBufLen) )
      return false;

   bufPos += typeBufLen;

   // valid

   unsigned validBufLen;

   if(!Serialization::deserializeBool(&buf[bufPos], bufLen-bufPos,
      &this->valid, &validBufLen) )
      return false;

   bufPos += validBufLen;


   *outLen = bufPos;

   return true;
}

/**
 * Loads the quota data from a file. The map is write locked during the load. Marks the store as
 * clean also when an error occurs.
 *
 * @param map the map with the QuotaData to load
 * @param path the path to the file
 * @return true if quota data are successful loaded
 */
bool QuotaData::loadQuotaDataMapForTargetFromFile(QuotaDataMapForTarget *map, std::string path)
{
   LogContext log("LoadQuotaDataMap");

   bool retVal = false;
   char* buf = NULL;
   size_t readRes;

   struct stat statBuf;
   int retValStat;

   if(!path.length() )
      return false;

   int fd = open(path.c_str(), O_RDONLY, 0);
   if(fd == -1)
   { // open failed
      log.log(Log_DEBUG, "Unable to open quota data file: " + path + ". " +
         "SysErr: " + System::getErrString() );

      return false;
   }

   retValStat = fstat(fd, &statBuf);
   if(retValStat)
   { // stat failed
      log.log(Log_WARNING, "Unable to stat quota data file: " + path + ". " +
         "SysErr: " + System::getErrString() );

      goto err_stat;
   }

   buf = (char*)malloc(statBuf.st_size);
   readRes = read(fd, buf, statBuf.st_size);
   if(readRes <= 0)
   { // reading failed
      log.log(Log_WARNING, "Unable to read quota data file: " + path + ". " +
         "SysErr: " + System::getErrString() );
   }
   else
   { // parse contents
      unsigned outLen;
      unsigned mapElemeCount;
      const char* mapStart;

      retVal = Serialization::deserializeQuotaDataMapForTargetPreprocess(buf, readRes,
         &mapElemeCount, &mapStart, &outLen);

      if (retVal)
         retVal = Serialization::deserializeQuotaDataMapForTarget(readRes, mapElemeCount, mapStart,
            map);
   }

   if (retVal == false)
      log.logErr("Could not deserialize quota data from file: " + path);
#ifdef BEEGFS_DEBUG
   else
   {
      int targetCounter = 0;
      int dataCounter = 0;

      for(QuotaDataMapForTargetIter targetIter = map->begin(); targetIter != map->end();
         targetIter++)
      {
         targetCounter++;

         for(QuotaDataMapIter iter = targetIter->second.begin(); iter != targetIter->second.end();
            iter++)
            dataCounter++;
      }

      log.log(Log_DEBUG, StringTk::intToStr(dataCounter) + " quota data of " +
         StringTk::intToStr(targetCounter) + " targets are loaded from file " + path);
   }
#endif

   free(buf);

err_stat:
   close(fd);

   return retVal;
}

/**
 * Stores the quota data into a file. The map is write locked during the save. Marks the store as
 * clean when the limits are successful stored. If an error occurs the store is marked as dirty.
 *
 * @param map the map with the QuotaData to store
 * @param path the path to the file
 * @return true if the limits are successful stored
 */
bool QuotaData::saveQuotaDataMapForTargetToFile(QuotaDataMapForTarget *map, std::string path)
{
   LogContext log("SaveQuotaDataMap");

   bool retVal = false;

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
      log.logErr("Unable to create quota data file: " + path + ". " +
         "SysErr: " + System::getErrString() );

      return false;
   }

   // file created => store data
   buf = (char*)malloc(Serialization::serialLenQuotaDataMapForTarget(map));
   if(!buf)
   {
      log.logErr("Unable to store quota data file: " + path + ". " +
         "SysErr: " + System::getErrString() );

      goto err_closefile;
   }

   bufLen = Serialization::serializeQuotaDataMapForTarget(buf, map);
   writeRes = write(fd, buf, bufLen);
   free(buf);

   if(writeRes != (ssize_t)bufLen)
   {
      log.logErr("Unable to store quota data file: " + path + ". " +
         "SysErr: " + System::getErrString() );

      goto err_closefile;
   }
#ifdef BEEGFS_DEBUG
   else
   {
      int targetCounter = 0;
      int dataCounter = 0;

      for(QuotaDataMapForTargetIter targetIter = map->begin(); targetIter != map->end();
         targetIter++)
      {
         targetCounter++;

         for(QuotaDataMapIter iter = targetIter->second.begin(); iter != targetIter->second.end();
            iter++)
            dataCounter++;
      }

      log.log(Log_DEBUG, StringTk::intToStr(dataCounter) + " quota data of " +
         StringTk::intToStr(targetCounter) + " targets are stored to file " + path);
   }
#endif

   retVal = true;

   // error compensation
err_closefile:
   close(fd);

   return retVal;
}

void QuotaData::quotaDataMapToString(QuotaDataMap* map, QuotaDataType quotaDataType,
   std::ostringstream* outStream)
{
   std::string quotaDataTypeStr;

   if(quotaDataType == QuotaDataType_USER)
      quotaDataTypeStr = "user";
   else
   if(quotaDataType == QuotaDataType_GROUP)
      quotaDataTypeStr = "group";


   *outStream << map->size() << " used quota for " << quotaDataTypeStr << " IDs: " << std::endl;

   for(QuotaDataMapIter idIter = map->begin(); idIter != map->end(); idIter++)
   {
      *outStream << "ID: " << idIter->second.getID()
                << " size: " << idIter->second.getSize()
                << " inodes: " << idIter->second.getInodes()
                << std::endl;
   }
}

void QuotaData::quotaDataMapForTargetToString(QuotaDataMapForTarget* map,
   QuotaDataType quotaDataType, std::ostringstream* outStream)
{
   std::string quotaDataTypeStr;

   if(quotaDataType == QuotaDataType_USER)
      quotaDataTypeStr = "user";
   else
   if(quotaDataType == QuotaDataType_GROUP)
      quotaDataTypeStr = "group";

   *outStream << "Quota data of " << map->size() << " targets." << std::endl;
   for(QuotaDataMapForTargetIter iter = map->begin(); iter != map->end(); iter++)
   {
      uint16_t targetNumID = iter->first;
      *outStream << "Used quota for " << quotaDataTypeStr << " IDs on target: " << targetNumID
         << std::endl;

      QuotaData::quotaDataMapToString(&iter->second, quotaDataType, outStream);
   }
}

void QuotaData::quotaDataListToString(QuotaDataList* list, std::ostringstream* outStream)
{
   for(QuotaDataListIter idIter = list->begin(); idIter != list->end(); idIter++)
      *outStream << "ID: " << idIter->getID()
                << " size: " << idIter->getSize()
                << " inodes: " << idIter->getInodes()
                << std::endl;
}
