#include <common/toolkit/StringTk.h>
#include "Config.h"

#define IGNORE_CONFIG_CLIENT_VALUE(keyStr) /* to be used in applyConfigMap() */ \
   if(testConfigMapKeyMatch(iter, keyStr, addDashes) ) \
      ; \
   else

// Note: Keep in sync with enum RunMode
RunModesElem const __RunModes[] =
{
   { "--checkfs", RunMode_CHECKFS },
   { "--enablequota", RunMode_ENABLEQUOTA },
   { NULL, RunMode_INVALID }
};

Config::Config(int argc, char** argv) throw (InvalidConfigException) :
   AbstractConfig(argc, argv)
{
   initConfig(argc, argv, false, true);
}

/**
 * Determine RunMode from config.
 * If a valid RunMode exists in the config, the corresponding config element will be erased.
 */
enum RunMode Config::determineRunMode()
{
   StringMap* configMap = getConfigMap();
   /* test for given help argument, e.g. in case the user wants to see mode-specific help with
      arguments "--help --<mode>". */

   StringMapIter iter = configMap->find(RUNMODE_HELP_KEY_STRING);
   if(iter != configMap->end() )
   { // user did specify "--help"
      /* note: it's important to remove the help arg here, because mode help will call this again
         to find out whether user wants to see mode-specific help. */
      eraseFromConfigMap(iter);
      return RunMode_HELP;
   }

   // walk all defined modes to check whether we find any of them in the config

   for(int i=0; __RunModes[i].modeString != NULL; i++)
   {
      iter = configMap->find(__RunModes[i].modeString);
      if(iter != configMap->end() )
      { // we found a valid mode in the config
         eraseFromConfigMap(iter);
         return __RunModes[i].runMode;
      }
   }

   // no valid mode found

   return RunMode_INVALID;
}

/**
 * Sets the default values for each configurable in the configMap.
 *
 * @param addDashes true to prepend "--" to all config keys.
 */
void Config::loadDefaults(bool addDashes)
{
   AbstractConfig::loadDefaults(addDashes);

   // re-definitions
   configMapRedefine("cfgFile", createDefaultCfgFilename(), addDashes);

   // own definitions
   configMapRedefine("connInterfacesFile", "", addDashes);
   configMapRedefine("connFsckPortUDP", "0", addDashes);

   configMapRedefine("debugRunComponentThreads", "true", addDashes);
   configMapRedefine("debugRunStartupTests", "false", addDashes);
   configMapRedefine("debugRunComponentTests", "false", addDashes);
   configMapRedefine("debugRunIntegrationTests", "false", addDashes);
   configMapRedefine("debugRunThroughputTests", "false", addDashes);

   configMapRedefine("tuneNumWorkers", "8", addDashes);
   configMapRedefine("tunePreferredNodesFile", "", addDashes);

   configMapRedefine("sysForcedRoot", "0", addDashes);

   configMapRedefine("runDaemonized", "false", addDashes);

   configMapRedefine("databaseFile", CONFIG_DEFAULT_DBFILE, addDashes);

   configMapRedefine("overwriteDbFile", "false", addDashes);

   configMapRedefine("testDatabaseFile", CONFIG_DEFAULT_TESTDBFILE, addDashes);

   configMapRedefine("DatabaseNumMaxConns", "4", addDashes);

   configMapRedefine("overrideRootMDS", "", addDashes);

   configMapRedefine("logStdFile", CONFIG_DEFAULT_LOGFILE, addDashes);
   configMapRedefine("logOutFile", CONFIG_DEFAULT_OUTFILE, addDashes);
   configMapRedefine("logNoDate", "false", addDashes);

   configMapRedefine("readOnly", "false", addDashes);

   configMapRedefine("noFetch", "false", addDashes);

   configMapRedefine("automatic", "false", addDashes);

   configMapRedefine("runOnline", "false", addDashes);
   
   configMapRedefine("quotaEnabled", "false", addDashes);

   configMapRedefine("keepTestDatabaseFile", "false", addDashes);

   configMapRedefine("ignoreDBDiskSpace", "false", addDashes);
}

/**
 * @param addDashes true to prepend "--" to tested config keys for matching.
 */
void Config::applyConfigMap(bool enableException, bool addDashes) throw (InvalidConfigException)
{
   AbstractConfig::applyConfigMap(false, addDashes);

   for (StringMapIter iter = getConfigMap()->begin(); iter != getConfigMap()->end();)
   {
      bool unknownElement = false;

      IGNORE_CONFIG_CLIENT_VALUE("logClientID")
      IGNORE_CONFIG_CLIENT_VALUE("logHelperdIP")
      IGNORE_CONFIG_CLIENT_VALUE("logType")
      if(testConfigMapKeyMatch(iter, "connInterfacesFile", addDashes) )
         connInterfacesFile = iter->second;
      else
      if(testConfigMapKeyMatch(iter, "connFsckPortUDP", addDashes) )
         connFsckPortUDP = StringTk::strToInt(iter->second);
      else
      IGNORE_CONFIG_CLIENT_VALUE("connNumCommRetries")
      IGNORE_CONFIG_CLIENT_VALUE("connUnmountRetries")
      IGNORE_CONFIG_CLIENT_VALUE("connCommRetrySecs")
      IGNORE_CONFIG_CLIENT_VALUE("connRecvNonIntrTimeoutMS")
      if(testConfigMapKeyMatch(iter, "debugRunComponentThreads", addDashes) )
         debugRunComponentThreads = StringTk::strToBool(iter->second);
      else
      if(testConfigMapKeyMatch(iter, "debugRunStartupTests", addDashes) )
         debugRunStartupTests = StringTk::strToBool(iter->second);
      else
      if(testConfigMapKeyMatch(iter, "debugRunComponentTests", addDashes) )
         debugRunComponentTests = StringTk::strToBool(iter->second);
      else
      if(testConfigMapKeyMatch(iter, "debugRunIntegrationTests", addDashes) )
         debugRunIntegrationTests = StringTk::strToBool(iter->second);
      else
      if(testConfigMapKeyMatch(iter, "debugRunThroughputTests", addDashes) )
         debugRunThroughputTests = StringTk::strToBool(iter->second);
      else
      if(testConfigMapKeyMatch(iter, "tuneNumWorkers", addDashes) )
         tuneNumWorkers = StringTk::strToUInt(iter->second);
      else
      IGNORE_CONFIG_CLIENT_VALUE("tuneNumRetryWorkers")
      if(testConfigMapKeyMatch(iter, "tunePreferredNodesFile", addDashes) )
         tunePreferredNodesFile = iter->second;
      else
      IGNORE_CONFIG_CLIENT_VALUE("tuneFileCacheType")
      IGNORE_CONFIG_CLIENT_VALUE("tunePagedIOBufSize")
      IGNORE_CONFIG_CLIENT_VALUE("tunePagedIOBufNum")
      IGNORE_CONFIG_CLIENT_VALUE("tuneFileCacheBufSize")
      IGNORE_CONFIG_CLIENT_VALUE("tuneFileCacheBufNum")
      IGNORE_CONFIG_CLIENT_VALUE("tunePageCacheValidityMS")
      IGNORE_CONFIG_CLIENT_VALUE("tuneAttribCacheValidityMS")
      IGNORE_CONFIG_CLIENT_VALUE("tuneMaxWriteWorks")
      IGNORE_CONFIG_CLIENT_VALUE("tuneMaxReadWorks")
      IGNORE_CONFIG_CLIENT_VALUE("tuneAllowMultiSetWrite")
      IGNORE_CONFIG_CLIENT_VALUE("tuneAllowMultiSetRead")
      IGNORE_CONFIG_CLIENT_VALUE("tunePathBufSize")
      IGNORE_CONFIG_CLIENT_VALUE("tunePathBufNum")
      IGNORE_CONFIG_CLIENT_VALUE("tuneMaxReadWriteNum")
      IGNORE_CONFIG_CLIENT_VALUE("tuneMaxReadWriteNodesNum")
      IGNORE_CONFIG_CLIENT_VALUE("tuneMsgBufSize")
      IGNORE_CONFIG_CLIENT_VALUE("tuneMsgBufNum")
      IGNORE_CONFIG_CLIENT_VALUE("tuneRemoteFSync")
      IGNORE_CONFIG_CLIENT_VALUE("tunePreferredMetaFile")
      IGNORE_CONFIG_CLIENT_VALUE("tunePreferredStorageFile")
      IGNORE_CONFIG_CLIENT_VALUE("tuneUseGlobalFileLocks")
      IGNORE_CONFIG_CLIENT_VALUE("tuneRefreshOnGetAttr")
      IGNORE_CONFIG_CLIENT_VALUE("tuneInodeBlockBits")
      IGNORE_CONFIG_CLIENT_VALUE("tuneInodeBlockSize")
      IGNORE_CONFIG_CLIENT_VALUE("tuneMaxClientMirrorSize")
      IGNORE_CONFIG_CLIENT_VALUE("tuneEarlyCloseResponse")
      IGNORE_CONFIG_CLIENT_VALUE("tuneUseGlobalAppendLocks")
      IGNORE_CONFIG_CLIENT_VALUE("tuneUseBufferedAppend")
      IGNORE_CONFIG_CLIENT_VALUE("tuneStatFsCacheSecs")
      IGNORE_CONFIG_CLIENT_VALUE("sysCreateHardlinksAsSymlinks")
      IGNORE_CONFIG_CLIENT_VALUE("sysMountSanityCheckMS")
      IGNORE_CONFIG_CLIENT_VALUE("sysSyncOnClose")
      IGNORE_CONFIG_CLIENT_VALUE("sysSessionCheckOnClose")
      IGNORE_CONFIG_CLIENT_VALUE("sysTargetOfflineTimeoutSecs")
      IGNORE_CONFIG_CLIENT_VALUE("sysInodeIDStyle")
      IGNORE_CONFIG_CLIENT_VALUE("sysACLsEnabled")
      IGNORE_CONFIG_CLIENT_VALUE("sysXAttrsEnabled")
      IGNORE_CONFIG_CLIENT_VALUE("tuneDirSubentryCacheValidityMS")
      IGNORE_CONFIG_CLIENT_VALUE("tuneFileSubentryCacheValidityMS")
      if(testConfigMapKeyMatch(iter, "sysForcedRoot", addDashes) )
         sysForcedRoot = (int16_t)StringTk::strToInt(iter->second);
      else
      if(testConfigMapKeyMatch(iter, "runDaemonized", addDashes) )
         runDaemonized = StringTk::strToBool(iter->second);
      else
      if(testConfigMapKeyMatch(iter, "databaseFile", addDashes) )
         databaseFile = iter->second;
      else
      if(testConfigMapKeyMatch(iter, "overwriteDbFile", addDashes) )
         overwriteDbFile = StringTk::strToBool(iter->second);
      else
      if(testConfigMapKeyMatch(iter, "testDatabaseFile", addDashes) )
         testDatabaseFile = iter->second;
      else
      if(testConfigMapKeyMatch(iter, "databaseNumMaxConns", addDashes) )
         databaseNumMaxConns = StringTk::strHexToUInt(iter->second);
      else
      if(testConfigMapKeyMatch(iter, "overrideRootMDS", addDashes) )
         overrideRootMDS = iter->second;
      else
      if(testConfigMapKeyMatch(iter, "logStdFile", addDashes) )
         logStdFile = iter->second;
      else
      if(testConfigMapKeyMatch(iter, "logOutFile", addDashes) )
         logOutFile = iter->second;
      else
      if(testConfigMapKeyMatch(iter, "readOnly", addDashes) )
         readOnly = StringTk::strToBool(iter->second);
      else
      if(testConfigMapKeyMatch(iter, "noFetch", addDashes) )
         noFetch = StringTk::strToBool(iter->second);
      else
      if(testConfigMapKeyMatch(iter, "automatic", addDashes) )
         automatic = StringTk::strToBool(iter->second);
      else
      if(testConfigMapKeyMatch(iter, "runOnline", addDashes) )
         runOnline = StringTk::strToBool(iter->second);
      else
      if(testConfigMapKeyMatch(iter, "quotaEnabled", addDashes) )
         quotaEnabled = StringTk::strToBool(iter->second);
      else
      if(testConfigMapKeyMatch(iter, "keepTestDatabaseFile", addDashes) )
         keepTestDatabaseFile = StringTk::strToBool(iter->second);
      else
      if(testConfigMapKeyMatch(iter, "ignoreDBDiskSpace", addDashes) )
         ignoreDBDiskSpace = StringTk::strToBool(iter->second);
      else
      { // unknown element occurred
         unknownElement = true;

         if (enableException)
         {
            throw InvalidConfigException(
               std::string("The config argument '") + iter->first + std::string("' is invalid") );
         }
      }

      // advance iterator (and remove handled element)

      if(unknownElement)
      { // just skip the unknown element
         iter++;
      }
      else
      { // remove this element from the map
         iter = eraseFromConfigMap(iter);
      }
   }
}

void Config::initImplicitVals() throw (InvalidConfigException)
{
   // tuneNumWorkers
   if (!tuneNumWorkers)
      tuneNumWorkers = BEEGFS_MAX(System::getNumOnlineCPUs() * 2, 4);

   // connAuthHash
   AbstractConfig::initConnAuthHash(connAuthFile, &connAuthHash);
}

std::string Config::createDefaultCfgFilename()
{
   struct stat statBuf;

   int statRes = stat(CONFIG_DEFAULT_CFGFILENAME, &statBuf);

   if (!statRes && S_ISREG(statBuf.st_mode))
      return CONFIG_DEFAULT_CFGFILENAME; // there appears to be a config file

   return ""; // no default file otherwise
}

