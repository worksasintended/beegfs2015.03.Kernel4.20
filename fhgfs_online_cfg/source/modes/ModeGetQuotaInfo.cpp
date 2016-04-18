#include <app/App.h>
#include <common/Common.h>

#include <common/toolkit/NodesTk.h>
#include <common/toolkit/UnitTk.h>
#include <common/system/System.h>
#include <program/Program.h>

#include "ModeGetQuotaInfo.h"


//the arguments of the mode getQuota
#define MODEGETQUOTAINFO_ARG_UID                "--uid"
#define MODEGETQUOTAINFO_ARG_GID                "--gid"
#define MODEGETQUOTAINFO_ARG_ALL                "--all"
#define MODEGETQUOTAINFO_ARG_LIST               "--list"
#define MODEGETQUOTAINFO_ARG_RANGE              "--range"
#define MODEGETQUOTAINFO_ARG_WITHZERO           "--withzero"
#define MODEGETQUOTAINFO_ARG_WITHSYSTEM         "--withsystem"

int ModeGetQuotaInfo::execute()
{
   const int mgmtTimeoutMS = 2500;
   int retVal = APPCODE_RUNTIME_ERROR;

   NodeList storageNodesList;

   App* app = Program::getApp();
   DatagramListener* dgramLis = app->getDatagramListener();
   unsigned short mgmtPortUDP = app->getConfig()->getConnMgmtdPortUDP();
   NodeStoreServers* mgmtNodes = app->getMgmtNodes();
   std::string mgmtHost = app->getConfig()->getSysMgmtdHost();
   NodeStoreServers* storageNodes = app->getStorageNodes();
   StringMap* cfg = app->getConfig()->getUnknownConfigArgs();

   // check mgmt node
   if(!NodesTk::waitForMgmtHeartbeat(
      NULL, dgramLis, mgmtNodes, mgmtHost, mgmtPortUDP, mgmtTimeoutMS) )
   {
      std::cerr << "Management node communication failed: " << mgmtHost << std::endl;
      return APPCODE_RUNTIME_ERROR;
   }

   Node* mgmtNode = mgmtNodes->referenceFirstNode();

   //download the list of storage servers from the management node
   if(!NodesTk::downloadNodes(mgmtNode, NODETYPE_Storage, &storageNodesList, NULL) )
   {
      std::cerr << "Node download failed." << std::endl;
      mgmtNodes->releaseNode(&mgmtNode);
      return APPCODE_RUNTIME_ERROR;
   }

   NodesTk::moveNodesFromListToStore(&storageNodesList, storageNodes);

   retVal = checkConfig(cfg);
   if (retVal != APPCODE_NO_ERROR)
   {
      mgmtNodes->releaseNode(&mgmtNode);
      return retVal;
   }

   if(this->cfg.cfgUseAll)
   {
      if(this->cfg.cfgType == QuotaDataType_USER)
         System::getAllUserIDs(&this->cfg.cfgIDList, !this->cfg.cfgPrintSystem);
      else
         System::getAllGroupIDs(&this->cfg.cfgIDList, !this->cfg.cfgPrintSystem);
   }

   //remove all duplicated IDs, the list::unique() needs a sorted list
   if(this->cfg.cfgIDList.size() > 0)
   {
      this->cfg.cfgIDList.sort();
      this->cfg.cfgIDList.unique();
   }

   QuotaDataMapForTarget usedQuotaDataResults;
   QuotaDataMapForTarget quotaDataLimitsResults;

   // query the used quota data
   if (!this->requestQuotaDataAndCollectResponses(mgmtNode, storageNodes, app->getWorkQueue(),
      &usedQuotaDataResults, NULL, false) )
   {
      mgmtNodes->releaseNode(&mgmtNode);
      return APPCODE_RUNTIME_ERROR;
   }

   // query the quota limits
   if (!this->requestQuotaDataAndCollectResponses(mgmtNode, storageNodes, app->getWorkQueue(),
         &quotaDataLimitsResults, NULL, true) )
   {
      mgmtNodes->releaseNode(&mgmtNode);
      return APPCODE_RUNTIME_ERROR;
   }

   QuotaDataMapForTargetIter iterUsedQuota = usedQuotaDataResults.find(
      QUOTADATAMAPFORTARGET_ALL_TARGETS_ID);
   QuotaDataMapForTargetIter iterQuotaLimits = quotaDataLimitsResults.find(
      QUOTADATAMAPFORTARGET_ALL_TARGETS_ID);

   if(iterUsedQuota != usedQuotaDataResults.end() &&
      iterQuotaLimits != quotaDataLimitsResults.end() )
   {
      printQuota(&iterUsedQuota->second, &iterQuotaLimits->second);
   }
   else
   {
      std::cerr << "Couldn't receive valid quota data." << std::endl;
      retVal = APPCODE_RUNTIME_ERROR;
   }

   mgmtNodes->releaseNode(&mgmtNode);

   return retVal;
}

/*
 * checks the arguments from the commandline
 *
 * @param cfg a mstring map with all arguments from the command line
 * @return the error code (APPCODE_...)
 *
 */
int ModeGetQuotaInfo::checkConfig(StringMap* cfg)
{
   StringMapIter iter;

   // check arguments

   // parse include unused UIDs/GIDs
   iter = cfg->find(MODEGETQUOTAINFO_ARG_WITHZERO);
   if(iter != cfg->end() )
   {
      this->cfg.cfgPrintUnused = true;
      cfg->erase(iter);
   }

   // parse include system user/groups
   iter = cfg->find(MODEGETQUOTAINFO_ARG_WITHSYSTEM);
   if(iter != cfg->end() )
   {
      this->cfg.cfgPrintSystem = true;
      cfg->erase(iter);
   }

   // parse quota type argument for user
   iter = cfg->find(MODEGETQUOTAINFO_ARG_UID);
   if (iter != cfg->end())
   {
      if (this->cfg.cfgType != QuotaDataType_NONE)
      {
         std::cerr << "Invalid configuration. Only one of --uid or --gid is allowed." << std::endl;
         return APPCODE_INVALID_CONFIG;
      }

      this->cfg.cfgType = QuotaDataType_USER;
      cfg->erase(iter);
   }


   // parse quota type argument for group
   iter = cfg->find(MODEGETQUOTAINFO_ARG_GID);
   if (iter != cfg->end())
   {
      if (this->cfg.cfgType != QuotaDataType_NONE)
      {
         std::cerr << "Invalid configuration. "
            "Only one of --uid or --gid is allowed." << std::endl;
         return APPCODE_INVALID_CONFIG;
      }

      this->cfg.cfgType = QuotaDataType_GROUP;
      cfg->erase(iter);
   }


   // check if one quota type argument was set
   if (this->cfg.cfgType == QuotaDataType_NONE)
   {
      std::cerr << "Invalid configuration. "
         "One of --uid or --gid is required." << std::endl;
      return APPCODE_INVALID_CONFIG;
   }


   // parse quota argument for query all GIDs or UIDs
   iter = cfg->find(MODEGETQUOTAINFO_ARG_ALL);
   if (iter != cfg->end())
   {
      if(this->cfg.cfgUseList || this->cfg.cfgUseRange)
      {
         std::cerr << "Invalid configuration. "
            "Only one of --all, --list, or --range is allowed." << std::endl;
         return APPCODE_INVALID_CONFIG;
      }

      this->cfg.cfgUseAll = true;
      cfg->erase(iter);
   }

   // parse quota argument for a list of GIDs or UIDs
   iter = cfg->find(MODEGETQUOTAINFO_ARG_LIST);
   if (iter != cfg->end())
   {
      if(this->cfg.cfgUseAll || this->cfg.cfgUseRange)
      {
         std::cerr << "Invalid configuration. "
            "Only one of --all, --list, or --range is allowed." << std::endl;
         return APPCODE_INVALID_CONFIG;
      }

      this->cfg.cfgUseList = true;
      cfg->erase(iter);
   }

   // parse quota argument for a range of GIDs or UIDs
   iter = cfg->find(MODEGETQUOTAINFO_ARG_RANGE);
   if (iter != cfg->end())
   {
      if(this->cfg.cfgUseAll || this->cfg.cfgUseList)
      {
         std::cerr << "Invalid configuration. "
            "Only one of --all, --list, or --range is allowed." << std::endl;
         return APPCODE_INVALID_CONFIG;
      }

      this->cfg.cfgUseRange = true;
      cfg->erase(iter);
   }

   // check if ID, ID list or ID range is given if it is required
   if(cfg->empty() && !this->cfg.cfgUseAll)
   {
      if(this->cfg.cfgUseRange)
         std::cerr << "No ID range specified." << std::endl;
      else
      if(this->cfg.cfgUseList)
         std::cerr << "No ID list specified." << std::endl;
      else
         std::cerr << "No ID specified." << std::endl;

      return APPCODE_INVALID_CONFIG;
   }


   // parse the ID, ID list or ID range if needed
   if(this->cfg.cfgUseRange)
   {
      if(cfg->size() < 2) // 2 for start and end value of the range
      {
         std::cerr << "No ID range specified. "
            "Use a space to separate the first ID and the last ID." << std::endl;
         return APPCODE_INVALID_CONFIG;
      }

      this->cfg.cfgIDRangeStart = StringTk::strToUInt(cfg->begin()->first);
      cfg->erase(cfg->begin() );

      this->cfg.cfgIDRangeEnd = StringTk::strToUInt(cfg->begin()->first);
      cfg->erase(cfg->begin() );

      if(this->cfg.cfgIDRangeStart > this->cfg.cfgIDRangeEnd)
      {
         unsigned tmpValue = this->cfg.cfgIDRangeStart;
         this->cfg.cfgIDRangeStart = this->cfg.cfgIDRangeEnd;
         this->cfg.cfgIDRangeEnd = tmpValue;
      }
   }
   else
   if(this->cfg.cfgUseList)
   {
      StringList listOfIDs;
      std::string listString = cfg->begin()->first;
      StringTk::explodeEx(listString, ',', true , &listOfIDs);

      for(StringListIter iter = listOfIDs.begin(); iter != listOfIDs.end(); iter++)
      {
         if(StringTk::isNumeric(*iter) )
            this->cfg.cfgIDList.push_back(StringTk::strToUInt(*iter));
         else
         {
            uint newID;
            bool lookupRes = (this->cfg.cfgType == QuotaDataType_USER) ?
               System::getUIDFromUserName(*iter, &newID) :
               System::getGIDFromGroupName(*iter, &newID);

            if(!lookupRes)
            { // user/group name not found
               std::cerr << "Unable to resolve given name: " << *iter << std::endl;
               return APPCODE_INVALID_CONFIG;
            }
            this->cfg.cfgIDList.push_back(newID);
         }
      }

      cfg->erase(cfg->begin() );
   }
   else
   if(!this->cfg.cfgUseAll)
   { // single ID is given
      std::string queryIDStr = cfg->begin()->first;

      if(StringTk::isNumeric(queryIDStr) )
         this->cfg.cfgID = StringTk::strToUInt(queryIDStr);
      else
      {
         bool lookupRes = (this->cfg.cfgType == QuotaDataType_USER) ?
            System::getUIDFromUserName(queryIDStr, &this->cfg.cfgID) :
            System::getGIDFromGroupName(queryIDStr, &this->cfg.cfgID);

         if(!lookupRes)
         { // user/group name not found
            std::cerr << "Unable to resolve given name: " << queryIDStr << std::endl;
            return APPCODE_INVALID_CONFIG;
         }
      }

      cfg->erase(cfg->begin() );
   }


   // print UID/GID with zero values when a list or a single UID/GID is given
   if(!this->cfg.cfgUseRange && !this->cfg.cfgUseAll)
   {
      this->cfg.cfgPrintUnused = true;
   }


   //check any wrong/unknown arguments are given
   if( ModeHelper::checkInvalidArgs(cfg) )
      return APPCODE_INVALID_CONFIG;


   return APPCODE_NO_ERROR;
}

/*
 * prints the help
 */
void ModeGetQuotaInfo::printHelp()
{
   std::cout << "MODE ARGUMENTS:" << std::endl;
   std::cout << " Mandatory:" << std::endl;
   std::cout << "  One of these arguments is mandatory:" << std::endl;
   std::cout << "    --uid       Show information for users." << std::endl;
   std::cout << "    --gid       Show information for groups." << std::endl;
   std::cout << std::endl;
   std::cout << "  One of these arguments is mandatory:" << std::endl;
   std::cout << "    <ID>                  Show quota for this single user or group ID." << std::endl;
   std::cout << "    --all                 Show quota for all user or group IDs on localhost." << std::endl;
   std::cout << "                          (System users/groups with UID/GID less than 100 or" << std::endl;
   std::cout << "                          shell set to \"nologin\" or \"false\" are filtered.)" << std::endl;
   std::cout << "    --list <list>         Use a comma separated list of user/group IDs." << std::endl;
   std::cout << "    --range <start> <end> Use a range of user/group IDs." << std::endl;
   std::cout << std::endl;
   std::cout << " Optional:" << std::endl;
   std::cout << "    --withzero     Print also users/groups that use no disk space. It is default" << std::endl;
   std::cout << "                   for a single UID/GID and a list of UIDs/GIDs." << std::endl;
   std::cout << "    --withsystem   Print also system users/groups. It is default for a single" << std::endl;
   std::cout << "                   UID/GID and a list of UIDs/GIDs." << std::endl;
   std::cout << std::endl;
   std::cout << "USAGE:" << std::endl;
   std::cout << " This mode collects the user/group quota information for the given UIDs/GIDs" << std::endl;
   std::cout << " from the storage servers and prints it to the console." << std::endl;
   std::cout << std::endl;
   std::cout << " Example: Show quota information for user ID 1000." << std::endl;
   std::cout << "  $ beegfs-ctl --getquota --uid 1000" << std::endl;
   std::cout << std::endl;
   std::cout << " Example: Show quota information for all normal users." << std::endl;
   std::cout << "  $ beegfs-ctl --getquota --uid --all" << std::endl;
   std::cout << std::endl;
   std::cout << " Example: Show quota information for user IDs 100 and 110." << std::endl;
   std::cout << "  $ beegfs-ctl --getquota --uid --list 100,110" << std::endl;
   std::cout << std::endl;
   std::cout << " Example: Show quota information for group IDs range 1000 to 1500." << std::endl;
   std::cout << "  $ beegfs-ctl --getquota --gid --range 1000 1500" << std::endl;
}

/*
 * prints the quota results
 *
 * @param usedQuota a list with the content of all used quota
 * @param quotaLimits a list with the content of all quota limits
 *
 */
void ModeGetQuotaInfo::printQuota(QuotaDataMap* usedQuota, QuotaDataMap* quotaLimits)
{
   bool allValid = false;

   printf("       user/group    ||           size          ||    chunk files    \n");
   printf("     name     |  id  ||    used    |    hard    ||  used   |  hard   \n");
   printf("--------------|------||------------|------------||---------|---------\n");

   if(this->cfg.cfgUseAll || this->cfg.cfgUseList)
      allValid = printQuotaForList(usedQuota, quotaLimits);
   else
   if(this->cfg.cfgUseRange)
      allValid = printQuotaForRange(usedQuota, quotaLimits);
   else
      allValid = printQuotaForID(usedQuota, quotaLimits, this->cfg.cfgID);

   if(!allValid)
      std::cerr << std::endl << "Some IDs were skipped because of invalid values. "
         "Check server logs." << std::endl;
}

bool ModeGetQuotaInfo::printQuotaForID(QuotaDataMap* usedQuota, QuotaDataMap* quotaLimits,
   unsigned id)
{
   // prepare used quota values
   std::string usedSize;
   std::string usedSizeUnit;
   unsigned long long usedInodes;

   QuotaDataMapIter usedQuotaIter = usedQuota->find(id);
   if(usedQuotaIter != usedQuota->end() )
   {
      // check if used quota data are valid
      if(!usedQuotaIter->second.isValid() )
         return false;

      // skip user/group with no space usage
      if(!this->cfg.cfgPrintUnused &&
         !usedQuotaIter->second.getSize() && !usedQuotaIter->second.getInodes() )
         return true;

      double usedSizeValue = UnitTk::byteToXbyte(usedQuotaIter->second.getSize(), &usedSizeUnit);
      int precision = ( (usedSizeUnit == "Byte") || (usedSizeUnit == "B") ) ? 0 : 2;
      usedSize = StringTk::doubleToStr(usedSizeValue, precision);

      usedInodes = usedQuotaIter->second.getInodes();
   }
   else
   if(this->cfg.cfgPrintUnused)
   {
      usedSize = "0";
      usedSizeUnit = "Byte";

      usedInodes = 0;
   }
   else // skip user/group with no space usage
      return true;



   // prepare hard limit values
   std::string hardLimitSize;
   std::string hardLimitSizeUnit;
   unsigned long long hardLimitInodes;

   QuotaDataMapIter limitsIter = quotaLimits->find(id);
   if(limitsIter != quotaLimits->end() )
   {
      double hardLimitSizeValue = UnitTk::byteToXbyte(limitsIter->second.getSize(),
         &hardLimitSizeUnit);
      int precision = ( (hardLimitSizeUnit == "Byte") || (hardLimitSizeUnit == "B") ) ? 0 : 2;
      hardLimitSize = StringTk::doubleToStr(hardLimitSizeValue, precision);

      hardLimitInodes = limitsIter->second.getInodes();
   }
   else
   {
      hardLimitSize = "0";
      hardLimitSizeUnit = "Byte";

      hardLimitInodes = 0llu;
   }



   // prepare the user or group name
   std::string name;

   if(this->cfg.cfgType == QuotaDataType_USER)
      name = System::getUserNameFromUID(id);
   else
      name = System::getGroupNameFromGID(id);

   if(name.size() == 0)
      name = StringTk::uintToStr(id);



   /*
    * The uses size value looks like not very exact but the calculation form KByte to KiByte and
    * the quota size in used blocks and not file size, makes the difference
    */
   printf("%14s|%6u||%7s %-4s|%7s %-4s||%9qu|%9qu\n",
      name.c_str(),                                         // name
      id,                                                   // id
      usedSize.c_str(),                                     // used (blocks)
      usedSizeUnit.c_str(),                                 // unit (used blocks)
      hardLimitSize.c_str(),                                // hard limit (blocks)
      hardLimitSizeUnit.c_str(),                            // unit (hard limit)
      usedInodes,                                           // used (inodes)
      hardLimitInodes);                                     // hard limit (inodes)

   return true;
}

bool ModeGetQuotaInfo::printQuotaForRange(QuotaDataMap* usedQuota, QuotaDataMap* quotaLimits)
{
   bool allValid = false;

   for(unsigned id = this->cfg.cfgIDRangeStart; id <= this->cfg.cfgIDRangeEnd; id++)
      allValid = printQuotaForID(usedQuota, quotaLimits, id);

   return allValid;
}

bool ModeGetQuotaInfo::printQuotaForList(QuotaDataMap* usedQuota, QuotaDataMap* quotaLimits)
{
   bool allValid = false;

   for(UIntListIter id = this->cfg.cfgIDList.begin(); id != this->cfg.cfgIDList.end(); id++)
      allValid = printQuotaForID(usedQuota, quotaLimits, *id);

   return allValid;
}
