#include <app/log/Logger.h>
#include <common/toolkit/HashTk.h>
#include <common/toolkit/StringTk.h>
#include "Config.h"

#include <linux/fs.h>
#include <asm/uaccess.h>


#define CONFIG_DEFAULT_CFGFILENAME     "/etc/beegfs/beegfs-client.conf"
#define CONFIG_FILE_COMMENT_CHAR       '#'
#define CONFIG_FILE_MAX_LINE_LENGTH    1024
#define CONFIG_ERR_BUF_LENGTH          1024
#define CONFIG_AUTHFILE_READSIZE       1024 // max amount of data that we read from auth file
#define CONFIG_AUTHFILE_MINSIZE        4 // at least 2, because we compute two 32bit hashes

#define FILECACHETYPE_NONE_STR      "none"
#define FILECACHETYPE_BUFFERED_STR  "buffered"
#define FILECACHETYPE_PAGED_STR     "paged"
#define FILECACHETYPE_NATIVE_STR    "native"

#define LOGGERTYPE_HELPERD_STR      "helperd"
#define LOGGERTYPE_SYSLOG_STR       "syslog"


#define IGNORE_CONFIG_VALUE(compareStr) /* to be used in applyConfigMap() */ \
   if(!os_strcmp(keyStr, compareStr) ) \
      ; \
   else



static fhgfs_bool __Config_readLineFromFile(struct file* cfgFile,
   char* buf, size_t bufLen, fhgfs_bool* outEndOfFile);



/**
 * @param mountConfig will be copied (not owned by this object)
 */
void Config_init(Config* this, MountConfig* mountConfig)
{
   this->configMap = StrCpyMap_construct();
   this->configValid = fhgfs_true;

   // init configurable strings
   this->cfgFile = NULL;
   this->logHelperdIP = NULL;
   this->logErrFile = NULL;
   this->logStdFile = NULL;
   this->logType = NULL;
   this->connInterfacesFile = NULL;
   this->connNetFilterFile = NULL;
   this->connAuthFile = NULL;
   this->connTcpOnlyFilterFile = NULL;
   this->tunePreferredMetaFile = NULL;
   this->tunePreferredStorageFile = NULL;
   this->tuneFileCacheType = NULL;
   this->sysMgmtdHost = NULL;
   this->sysInodeIDStyle = NULL;

   _Config_initConfig(this, mountConfig, fhgfs_true);
}

Config* Config_construct(MountConfig* mountConfig)
{
   Config* this = (Config*)os_kmalloc(sizeof(Config) );

   Config_init(this, mountConfig);

   return this;
}

void Config_uninit(Config* this)
{
   SAFE_KFREE(this->cfgFile);
   SAFE_KFREE(this->logHelperdIP);
   SAFE_KFREE(this->logErrFile);
   SAFE_KFREE(this->logStdFile);
   SAFE_KFREE(this->logType);
   SAFE_KFREE(this->connInterfacesFile);
   SAFE_KFREE(this->connNetFilterFile);
   SAFE_KFREE(this->connAuthFile);
   SAFE_KFREE(this->connTcpOnlyFilterFile);
   SAFE_KFREE(this->tunePreferredMetaFile);
   SAFE_KFREE(this->tunePreferredStorageFile);
   SAFE_KFREE(this->tuneFileCacheType);
   SAFE_KFREE(this->sysMgmtdHost);
   SAFE_KFREE(this->sysInodeIDStyle);

   StrCpyMap_destruct(this->configMap);
}

void Config_destruct(Config* this)
{
   Config_uninit(this);

   os_kfree(this);
}

fhgfs_bool _Config_initConfig(Config* this, MountConfig* mountConfig, fhgfs_bool enableException)
{
   // load and apply args to see whether we have a cfgFile
   _Config_loadDefaults(this);
   __Config_loadFromMountConfig(this, mountConfig);

   if(!_Config_applyConfigMap(this, enableException) )
   {
      this->configValid = fhgfs_false;
      return fhgfs_false;
   }

   if(this->cfgFile && os_strlen(this->cfgFile) )
   { // there is a config file specified
      // start over again and include the config file this time
      StrCpyMap_clear(this->configMap);

      _Config_loadDefaults(this);
      if(!__Config_loadFromFile(this, this->cfgFile) )
      {
         this->configValid = fhgfs_false;
         return fhgfs_false;
      }

      __Config_loadFromMountConfig(this, mountConfig);

      if(!_Config_applyConfigMap(this, enableException) )
      {
         this->configValid = fhgfs_false;
         return fhgfs_false;
      }
   }

   __Config_initImplicitVals(this);

   return this->configValid;
}

StrCpyMapIter _Config_eraseFromConfigMap(Config* this, StrCpyMapIter* iter)
{
   char* nextKey;
   StrCpyMapIter nextIter;

   nextIter = *iter;
   StrCpyMapIter_next(&nextIter);

   if(StrCpyMapIter_end(&nextIter) )
   { // no next element in the map
      StrCpyMap_erase(this->configMap, StrCpyMapIter_key(iter) );
      return nextIter;
   }

   nextKey = StrCpyMapIter_key(&nextIter);
   StrCpyMap_erase(this->configMap, StrCpyMapIter_key(iter) );

   return StrCpyMap_find(this->configMap, nextKey);
}

void _Config_loadDefaults(Config* this)
{
   /**
    * **IMPORTANT**: Don't forget to add new values also to the BEEGFS_ONLINE_CFG and BEEGFS_FSCK
    * ignored config values!
    */

   _Config_configMapRedefine(this, "cfgFile",                          "");

   _Config_configMapRedefine(this, "logLevel",                         "3");
   _Config_configMapRedefine(this, "logErrsToStdlog",                  "true");
   _Config_configMapRedefine(this, "logNoDate",                        "true");
   _Config_configMapRedefine(this, "logClientID",                      "false");
   _Config_configMapRedefine(this, "logHelperdIP",                     "127.0.0.1");
   _Config_configMapRedefine(this, "logStdFile",                       "");
   _Config_configMapRedefine(this, "logErrFile",                       "");
   _Config_configMapRedefine(this, "logNumLines",                      "100000");
   _Config_configMapRedefine(this, "logNumRotatedFiles",               "2");
   _Config_configMapRedefine(this, "logType",                          LOGGERTYPE_HELPERD_STR);

   _Config_configMapRedefine(this, "connPortShift",                    "0");
   _Config_configMapRedefine(this, "connClientPortUDP",                "8004");
   _Config_configMapRedefine(this, "connMetaPortUDP",                  "8005");
   _Config_configMapRedefine(this, "connStoragePortUDP",               "8003");
   _Config_configMapRedefine(this, "connMgmtdPortUDP",                 "8008");
   _Config_configMapRedefine(this, "connMetaPortTCP",                  "8005");
   _Config_configMapRedefine(this, "connStoragePortTCP",               "8003");
   _Config_configMapRedefine(this, "connHelperdPortTCP",               "8006");
   _Config_configMapRedefine(this, "connMgmtdPortTCP",                 "8008");
   _Config_configMapRedefine(this, "connUseSDP",                       "false");
   _Config_configMapRedefine(this, "connUseRDMA",                      "true");
   _Config_configMapRedefine(this, "connMaxInternodeNum",              "8");
   _Config_configMapRedefine(this, "connInterfacesFile",               "");
   _Config_configMapRedefine(this, "connFallbackExpirationSecs",       "900");
   _Config_configMapRedefine(this, "connCommRetrySecs",                "600");
   _Config_configMapRedefine(this, "connUnmountRetries",               "true");
   _Config_configMapRedefine(this, "connRDMABufSize",                  "8192");
   _Config_configMapRedefine(this, "connRDMABufNum",                   "70");
   _Config_configMapRedefine(this, "connRDMATypeOfService",            "0");
   _Config_configMapRedefine(this, "connNetFilterFile",                "");
   _Config_configMapRedefine(this, "connAuthFile",                     "");
   _Config_configMapRedefine(this, "connRecvNonIntrTimeoutMS",         "5000");
   _Config_configMapRedefine(this, "connTcpOnlyFilterFile",            "");

   _Config_configMapRedefine(this, "debugRunComponentThreads",         "true");
   _Config_configMapRedefine(this, "debugRunStartupTests",             "false");
   _Config_configMapRedefine(this, "debugRunComponentTests",           "false");
   _Config_configMapRedefine(this, "debugRunIntegrationTests",         "false");
   _Config_configMapRedefine(this, "debugRunThroughputTests",          "false");
   _Config_configMapRedefine(this, "debugFindOtherNodes",              "true");

   _Config_configMapRedefine(this, "tuneNumWorkers",                   "0");
   _Config_configMapRedefine(this, "tuneNumRetryWorkers",              "1");
   _Config_configMapRedefine(this, "tunePreferredMetaFile",            "");
   _Config_configMapRedefine(this, "tunePreferredStorageFile",         "");
   _Config_configMapRedefine(this, "tuneFileCacheType",                FILECACHETYPE_BUFFERED_STR);
   _Config_configMapRedefine(this, "tunePagedIOBufSize",               "2097152");
   _Config_configMapRedefine(this, "tunePagedIOBufNum",                "8");
   _Config_configMapRedefine(this, "tuneFileCacheBufSize",             "524288");
   _Config_configMapRedefine(this, "tuneFileCacheBufNum",              "0");
   _Config_configMapRedefine(this, "tunePageCacheValidityMS",          "2000000000");
   _Config_configMapRedefine(this, "tuneAttribCacheValidityMS",        "1000");
   _Config_configMapRedefine(this, "tuneDirSubentryCacheValidityMS",   "1000");
   _Config_configMapRedefine(this, "tuneFileSubentryCacheValidityMS",  "0");
   _Config_configMapRedefine(this, "tuneMaxWriteWorks",                "8");
   _Config_configMapRedefine(this, "tuneMaxReadWorks",                 "8");
   _Config_configMapRedefine(this, "tuneAllowMultiSetWrite",           "false");
   _Config_configMapRedefine(this, "tuneAllowMultiSetRead",            "false");
   _Config_configMapRedefine(this, "tunePathBufSize",                  "4096");
   _Config_configMapRedefine(this, "tunePathBufNum",                   "8");
   _Config_configMapRedefine(this, "tuneMaxReadWriteNum",              "0");
   _Config_configMapRedefine(this, "tuneMaxReadWriteNodesNum",         "8");
   _Config_configMapRedefine(this, "tuneMsgBufSize",                   "65536");
   _Config_configMapRedefine(this, "tuneMsgBufNum",                    "0");
   _Config_configMapRedefine(this, "tuneRemoteFSync",                  "true");
   _Config_configMapRedefine(this, "tuneUseGlobalFileLocks",           "false");
   _Config_configMapRedefine(this, "tuneRefreshOnGetAttr",             "false");
   _Config_configMapRedefine(this, "tuneInodeBlockBits",               "19");
   _Config_configMapRedefine(this, "tuneEarlyCloseResponse",           "false");
   _Config_configMapRedefine(this, "tuneUseGlobalAppendLocks",         "false");
   _Config_configMapRedefine(this, "tuneUseBufferedAppend",            "true");
   _Config_configMapRedefine(this, "tuneStatFsCacheSecs",              "10");

   _Config_configMapRedefine(this, "sysMgmtdHost",                     "");
   _Config_configMapRedefine(this, "sysInodeIDStyle",                  INODEIDSTYLE_DEFAULT);
   _Config_configMapRedefine(this, "sysCreateHardlinksAsSymlinks",     "false");
   _Config_configMapRedefine(this, "sysMountSanityCheckMS",            "11000");
   _Config_configMapRedefine(this, "sysSyncOnClose",                   "false");
   _Config_configMapRedefine(this, "sysSessionCheckOnClose",           "false");

   // Note: The default here is intentionally set to 60, while the default for the servers is 30.
   // This ensures that the servers push their state twice during one Mgmtd InternodeSyncer run,
   // but the client only needs to fetch the states once during that period.
   _Config_configMapRedefine(this, "sysUpdateTargetStatesSecs",        "60");
   _Config_configMapRedefine(this, "sysTargetOfflineTimeoutSecs",      "900");

   _Config_configMapRedefine(this, "sysXAttrsEnabled",                 "false");
   _Config_configMapRedefine(this, "sysACLsEnabled",                   "false");

   _Config_configMapRedefine(this, "quotaEnabled",                     "false");
}

fhgfs_bool _Config_applyConfigMap(Config* this, fhgfs_bool enableException)
{
   /**
    * **IMPORTANT**: Don't forget to add new values also to the BEEGFS_ONLINE_CFG and BEEGFS_FSCK
    * ignored config values!
    */


   StrCpyMapIter iter = StrCpyMap_begin(this->configMap);

   while(!StrCpyMapIter_end(&iter) )
   {
      fhgfs_bool unknownElement = fhgfs_false;

      char* keyStr = StrCpyMapIter_key(&iter);
      char* valueStr = StrCpyMapIter_value(&iter);

      if(!os_strcmp(keyStr, "cfgFile") )
      {
         SAFE_KFREE(this->cfgFile);
         this->cfgFile = StringTk_strDup(valueStr );
      }
      else
      if(!os_strcmp(keyStr, "logLevel") )
         this->logLevel = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "logErrsToStdlog") )
         this->logErrsToStdlog = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "logNoDate") )
         this->logNoDate = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "logClientID") )
         this->logClientID = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "logHelperdIP") )
      {
         SAFE_KFREE(this->logHelperdIP);
         this->logHelperdIP = StringTk_strDup(valueStr);
      }
      else
      if(!os_strcmp(keyStr, "logStdFile") )
      {
         SAFE_KFREE(this->logStdFile);
         this->logStdFile = StringTk_strDup(valueStr);
      }
      else
      if(!os_strcmp(keyStr, "logErrFile") )
      {
         SAFE_KFREE(this->logErrFile);
         this->logErrFile = StringTk_strDup(valueStr);
      }
      else
      if(!os_strcmp(keyStr, "logNumLines") )
         this->logNumLines = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "logNumRotatedFiles") )
         this->logNumRotatedFiles = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "logType") )
      {
         SAFE_KFREE(this->logType);
         this->logType = StringTk_strDup(valueStr);
      }
      else
      if(!os_strcmp(keyStr, "connPortShift") )
         this->connPortShift = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "connClientPortUDP") )
         this->connClientPortUDP = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "connMetaPortUDP") )
         this->connMetaPortUDP = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "connStoragePortUDP") )
         this->connStoragePortUDP = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "connMgmtdPortUDP") )
         this->connMgmtdPortUDP = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "connMetaPortTCP") )
         this->connMetaPortTCP = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "connStoragePortTCP") )
         this->connStoragePortTCP = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "connHelperdPortTCP") )
         this->connHelperdPortTCP = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "connMgmtdPortTCP") )
         this->connMgmtdPortTCP = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "connUseSDP") )
         this->connUseSDP = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "connUseRDMA") )
         this->connUseRDMA = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "connRDMATypeOfService") )
         this->connRDMATypeOfService = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "connMaxInternodeNum") )
         this->connMaxInternodeNum = StringTk_strToUInt(valueStr);
      else
      if(!os_strcmp(keyStr, "connInterfacesFile") )
      {
         SAFE_KFREE(this->connInterfacesFile);
         this->connInterfacesFile = StringTk_strDup(valueStr);
      }
      else
      if(!os_strcmp(keyStr, "connNonPrimaryExpiration") )
      {
         // superseded by connFallbackExpirationSecs, ignored here for config file compatibility
      }
      else
      if(!os_strcmp(keyStr, "connFallbackExpirationSecs") )
         this->connFallbackExpirationSecs = StringTk_strToUInt(valueStr);
      else
      if(!os_strcmp(keyStr, "connCommRetrySecs") )
         this->connCommRetrySecs = StringTk_strToUInt(valueStr);
      else
      IGNORE_CONFIG_VALUE("connNumCommRetries") // auto-generated based on connCommRetrySecs
      if(!os_strcmp(keyStr, "connUnmountRetries") )
         this->connUnmountRetries = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "connRDMABufSize") )
         this->connRDMABufSize = StringTk_strToUInt(valueStr);
      else
      if(!os_strcmp(keyStr, "connRDMABufNum") )
         this->connRDMABufNum = StringTk_strToUInt(valueStr);
      else
      if(!os_strcmp(keyStr, "connNetFilterFile") )
      {
         SAFE_KFREE(this->connNetFilterFile);
         this->connNetFilterFile = StringTk_strDup(valueStr);
      }
      else
      if(!os_strcmp(keyStr, "connAuthFile") )
      {
         SAFE_KFREE(this->connAuthFile);
         this->connAuthFile = StringTk_strDup(valueStr);
      }
      else
      if(!os_strcmp(keyStr, "connRecvNonIntrTimeoutMS") )
         this->connRecvNonIntrTimeoutMS = StringTk_strToUInt(valueStr);
      else
      if(!os_strcmp(keyStr, "connTcpOnlyFilterFile") )
      {
         SAFE_KFREE(this->connTcpOnlyFilterFile);
         this->connTcpOnlyFilterFile = StringTk_strDup(valueStr);
      }
      else
      if(!os_strcmp(keyStr, "debugRunComponentThreads") )
         this->debugRunComponentThreads = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "debugRunStartupTests") )
         this->debugRunStartupTests = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "debugRunComponentTests") )
         this->debugRunComponentTests = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "debugRunIntegrationTests") )
         this->debugRunIntegrationTests = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "debugRunThroughputTests") )
         this->debugRunThroughputTests = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "debugFindOtherNodes") )
         this->debugFindOtherNodes = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "tuneNumWorkers") )
         this->tuneNumWorkers = StringTk_strToUInt(valueStr);
      else
      if(!os_strcmp(keyStr, "tuneNumRetryWorkers") )
         this->tuneNumRetryWorkers = StringTk_strToUInt(valueStr);
      else
      if(!os_strcmp(keyStr, "tunePreferredMetaFile") )
      {
         SAFE_KFREE(this->tunePreferredMetaFile);
         this->tunePreferredMetaFile = StringTk_strDup(valueStr);
      }
      else
      if(!os_strcmp(keyStr, "tunePreferredStorageFile") )
      {
         SAFE_KFREE(this->tunePreferredStorageFile);
         this->tunePreferredStorageFile = StringTk_strDup(valueStr);
      }
      else
      if(!os_strcmp(keyStr, "tuneFileCacheType") )
      {
         SAFE_KFREE(this->tuneFileCacheType);
         this->tuneFileCacheType = StringTk_strDup(valueStr);
      }
      else
      if(!os_strcmp(keyStr, "tunePagedIOBufSize") )
         this->tunePagedIOBufSize = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "tunePagedIOBufNum") )
         this->tunePagedIOBufNum = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "tuneFileCacheBufSize") )
         this->tuneFileCacheBufSize = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "tuneFileCacheBufNum") )
         this->tuneFileCacheBufNum = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "tunePageCacheValidityMS") )
         this->tunePageCacheValidityMS= StringTk_strToUInt(valueStr);
      else
      if(!os_strcmp(keyStr, "tuneAttribCacheValidityMS") )
         this->tuneAttribCacheValidityMS = StringTk_strToUInt(valueStr);
      else
      if(!os_strcmp(keyStr, "tuneDirSubentryCacheValidityMS") )
         this->tuneDirSubentryCacheValidityMS = StringTk_strToUInt(valueStr);
      else
      if(!os_strcmp(keyStr, "tuneFileSubentryCacheValidityMS") )
         this->tuneFileSubentryCacheValidityMS = StringTk_strToUInt(valueStr);
      else
      if(!os_strcmp(keyStr, "tuneMaxWriteWorks") )
         this->tuneMaxWriteWorks = StringTk_strToUInt(valueStr);
      else
      if(!os_strcmp(keyStr, "tuneMaxReadWorks") )
         this->tuneMaxReadWorks = StringTk_strToUInt(valueStr);
      else
      if(!os_strcmp(keyStr, "tuneAllowMultiSetWrite") )
         this->tuneAllowMultiSetWrite = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "tuneAllowMultiSetRead") )
         this->tuneAllowMultiSetRead = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "tunePathBufSize") )
         this->tunePathBufSize = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "tunePathBufNum") )
         this->tunePathBufNum = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "tuneMaxReadWriteNum") )
         this->tuneMaxReadWriteNum = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "tuneMaxReadWriteNodesNum") )
         this->tuneMaxReadWriteNodesNum = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "tuneMsgBufSize") )
         this->tuneMsgBufSize = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "tuneMsgBufNum") )
         this->tuneMsgBufNum = StringTk_strToInt(valueStr);
      else
      if(!os_strcmp(keyStr, "tuneRemoteFSync") )
         this->tuneRemoteFSync = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "tuneUseGlobalFileLocks") )
         this->tuneUseGlobalFileLocks = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "tuneRefreshOnGetAttr") )
         this->tuneRefreshOnGetAttr = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "tuneInodeBlockBits") )
         this->tuneInodeBlockBits = StringTk_strToUInt(valueStr);
      else
      IGNORE_CONFIG_VALUE("tuneInodeBlockSize") // auto-generated based on tuneInodeBlockBits
      IGNORE_CONFIG_VALUE("tuneMaxClientMirrorSize") // was removed, kept here for compat
      if(!os_strcmp(keyStr, "tuneEarlyCloseResponse") )
         this->tuneEarlyCloseResponse = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "tuneUseGlobalAppendLocks") )
         this->tuneUseGlobalAppendLocks = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "tuneUseBufferedAppend") )
         this->tuneUseBufferedAppend = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "tuneStatFsCacheSecs") )
         this->tuneStatFsCacheSecs = StringTk_strToUInt(valueStr);
      else
      if(!os_strcmp(keyStr, "sysMgmtdHost") )
      {
         SAFE_KFREE(this->sysMgmtdHost);
         this->sysMgmtdHost = StringTk_strDup(valueStr);
      }
      else
      if(!os_strcmp(keyStr, "sysInodeIDStyle") )
      {
         SAFE_KFREE(this->sysInodeIDStyle);
         this->sysInodeIDStyle = StringTk_strDup(valueStr);
      }
      else
      if(!os_strcmp(keyStr, "sysCreateHardlinksAsSymlinks") )
         this->sysCreateHardlinksAsSymlinks = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "sysMountSanityCheckMS") )
         this->sysMountSanityCheckMS = StringTk_strToUInt(valueStr);
      else
      if(!os_strcmp(keyStr, "sysSyncOnClose") )
         this->sysSyncOnClose = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "sysSessionCheckOnClose") )
         this->sysSessionCheckOnClose = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "sysUpdateTargetStatesSecs") )
         this->sysUpdateTargetStatesSecs = StringTk_strToUInt(valueStr);
      else
      if(!os_strcmp(keyStr, "sysTargetOfflineTimeoutSecs") )
         this->sysTargetOfflineTimeoutSecs = StringTk_strToUInt(valueStr);
      else
      if(!os_strcmp(keyStr, "sysXAttrsEnabled") )
         this->sysXAttrsEnabled = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "sysACLsEnabled") )
         this->sysACLsEnabled = StringTk_strToBool(valueStr);
      else
      if(!os_strcmp(keyStr, "quotaEnabled") )
         this->quotaEnabled = StringTk_strToBool(valueStr);
      else
      { // unknown element occurred
         unknownElement = fhgfs_true;

         if(enableException)
         {
            printk_fhgfs(KERN_WARNING, "The config argument '%s' is invalid\n", keyStr );

            return fhgfs_false;
         }
      }

      // remove known elements from the map (we don't need them any longer)
      if(unknownElement)
      { // just skip the unknown element
         StrCpyMapIter_next(&iter);
      }
      else
      { // remove this element from the map
         iter = _Config_eraseFromConfigMap(this, &iter);
      }
   } // end of while loop

   return fhgfs_true;
}

void _Config_configMapRedefine(Config* this, char* keyStr, const char* valueStr)
{
   StrCpyMapIter iter;

   iter = StrCpyMap_find(this->configMap, keyStr);

   if(!StrCpyMapIter_end(&iter) )
      StrCpyMap_erase(this->configMap, keyStr);


   StrCpyMap_insert(this->configMap, keyStr, valueStr);
}

void __Config_addLineToConfigMap(Config* this, char* line)
{
   int divPos;
   char* divPosStr;
   char* param;
   char* value;
   char* trimParam;
   char* trimValue;

   divPosStr = os_strchr(line, '=');

   if(!divPosStr)
   {
      char* trimCopy = StringTk_trimCopy(line);
      _Config_configMapRedefine(this, trimCopy, "");
      os_kfree(trimCopy);

      return;
   }

   divPos = divPosStr - line;

   param = StringTk_subStr(line, divPos);
   value = StringTk_subStr(&divPosStr[1], os_strlen(&divPosStr[1]) );

   trimParam = StringTk_trimCopy(param);
   trimValue = StringTk_trimCopy(value);

   _Config_configMapRedefine(this, trimParam, trimValue);

   os_kfree(param);
   os_kfree(value);

   os_kfree(trimParam);
   os_kfree(trimValue);
}

void __Config_loadFromMountConfig(Config* this, MountConfig* mountConfig)
{
   // string args

   if(mountConfig->cfgFile)
      _Config_configMapRedefine(this, "cfgFile", mountConfig->cfgFile);

   if(mountConfig->logStdFile)
      _Config_configMapRedefine(this, "logStdFile", mountConfig->logStdFile);

   if(mountConfig->logErrFile)
      _Config_configMapRedefine(this, "logErrFile", mountConfig->logErrFile);

   if(mountConfig->sysMgmtdHost)
      _Config_configMapRedefine(this, "sysMgmtdHost", mountConfig->sysMgmtdHost);

   if(mountConfig->tunePreferredMetaFile)
      _Config_configMapRedefine(this, "tunePreferredMetaFile", mountConfig->tunePreferredMetaFile);

   if(mountConfig->tunePreferredStorageFile)
      _Config_configMapRedefine(this, "tunePreferredStorageFile",
         mountConfig->tunePreferredStorageFile);

   // integer args

   if(mountConfig->logLevelDefined)
   {
      char* valueStr = StringTk_intToStr(mountConfig->logLevel);
      _Config_configMapRedefine(this, "logLevel", valueStr);
      os_kfree(valueStr);
   }

   if(mountConfig->connPortShiftDefined)
   {
      char* valueStr = StringTk_uintToStr(mountConfig->connPortShift);
      _Config_configMapRedefine(this, "connPortShift", valueStr);
      os_kfree(valueStr);
   }

   if(mountConfig->connMgmtdPortUDPDefined)
   {
      char* valueStr = StringTk_uintToStr(mountConfig->connMgmtdPortUDP);
      _Config_configMapRedefine(this, "connMgmtdPortUDP", valueStr);
      os_kfree(valueStr);
   }

   if(mountConfig->connMgmtdPortTCPDefined)
   {
      char* valueStr = StringTk_uintToStr(mountConfig->connMgmtdPortTCP);
      _Config_configMapRedefine(this, "connMgmtdPortTCP", valueStr);
      os_kfree(valueStr);
   }

   if(mountConfig->sysMountSanityCheckMSDefined)
   {
      char* valueStr = StringTk_uintToStr(mountConfig->sysMountSanityCheckMS);
      _Config_configMapRedefine(this, "sysMountSanityCheckMS", valueStr);
      os_kfree(valueStr);
   }
}

fhgfs_bool __Config_loadFromFile(struct Config* this, const char* filename)
{
   fhgfs_bool retVal = fhgfs_true;

   struct file* cfgFile;
   char* line;
   char* trimLine;
   int currentLineNum; // 1-based (just for error logging)
   mm_segment_t oldfs;

   //printk_fhgfs_debug(KERN_INFO, "Attempting to read config file: '%s'\n", filename);
   cfgFile = filp_open(filename, (O_RDONLY), 0);
   if(IS_ERR(cfgFile) )
   {
      printk_fhgfs(KERN_WARNING, "Failed to open config file: '%s'\n", filename);
      return fhgfs_false;
   }

   line = os_kmalloc(CONFIG_FILE_MAX_LINE_LENGTH);


   for(currentLineNum=1; ; currentLineNum++)
   {
      fhgfs_bool endOfFile;
      fhgfs_bool readRes;

      readRes = __Config_readLineFromFile(cfgFile, line, CONFIG_FILE_MAX_LINE_LENGTH, &endOfFile);

      // stop on end of file
      if(endOfFile)
         break;

      if(!readRes)
      { // error occurred
         printk_fhgfs(KERN_WARNING, "Error occurred while reading the config file "
            "(line: %d, file: '%s')\n", currentLineNum, filename);

         retVal = fhgfs_false;

         break;
      }


      // trim the line and add it to the config, ignore comment lines
      trimLine = StringTk_trimCopy(line);
      if(strlen(trimLine) && (trimLine[0] != CONFIG_FILE_COMMENT_CHAR) )
         __Config_addLineToConfigMap(this, trimLine);

      kfree(trimLine);
   }

   // clean up
   kfree(line);

   ACQUIRE_PROCESS_CONTEXT(oldfs);
   filp_close(cfgFile, NULL);
   RELEASE_PROCESS_CONTEXT(oldfs);

   return retVal;
}

/**
 * Reads each line of a file into a separate string and appends it to outList.
 *
 * All strings are trimmed and empty lines are not added to the list.
 */
fhgfs_bool Config_loadStringListFile(const char* filename, StrCpyList* outList)
{
   fhgfs_bool retVal = fhgfs_true;

   struct file* listFile;
   char* line;
   char* trimLine;
   mm_segment_t oldfs;

   //printk_fhgfs(KERN_INFO, "Attempting to read configured list file: '%s'\n", filename);
   listFile = filp_open(filename, (O_RDONLY), 0);
   if(IS_ERR(listFile) )
   {
      printk_fhgfs(KERN_WARNING, "Failed to open (config) list file: '%s'\n", filename);
      return fhgfs_false;
   }

   line = os_kmalloc(CONFIG_FILE_MAX_LINE_LENGTH);


   for( ; ; )
   {
      fhgfs_bool endOfFile;
      fhgfs_bool readRes;

      readRes = __Config_readLineFromFile(listFile, line, CONFIG_FILE_MAX_LINE_LENGTH,
         &endOfFile);

      // stop on end of file
      if(endOfFile)
         break;

      if(!readRes)
      { // error occurred
         printk_fhgfs(KERN_WARNING, "Error occurred while reading a (config) list file: "
            "'%s'\n", filename);

         retVal = fhgfs_false;

         break;
      }

      // trim the line and add it to the nodes list, ignore comment lines
      trimLine = StringTk_trimCopy(line);
      if(strlen(trimLine) && (trimLine[0] != CONFIG_FILE_COMMENT_CHAR) )
         StrCpyList_append(outList, trimLine);

      kfree(trimLine);

   }

   // clean up
   kfree(line);

   ACQUIRE_PROCESS_CONTEXT(oldfs);
   filp_close(listFile, NULL);
   RELEASE_PROCESS_CONTEXT(oldfs);

   return retVal;
}

/**
 * Wrapper for loadStringListFile() to read file into a UInt16List.
 */
fhgfs_bool Config_loadUInt16ListFile(struct Config* this, const char* filename, UInt16List* outList)
{
   StrCpyList* strList = StrCpyList_construct();
   StrCpyListIter iter;
   fhgfs_bool loadRes;

   loadRes = Config_loadStringListFile(filename, strList);
   if(!loadRes)
      goto cleanup_and_exit;

   StrCpyListIter_init(&iter, strList);

   for( ; !StrCpyListIter_end(&iter); StrCpyListIter_next(&iter) )
   {
      char* currentLine = StrCpyListIter_value(&iter);

      UInt16List_append(outList, StringTk_strToUInt(currentLine) );
   }

cleanup_and_exit:
   StrCpyList_destruct(strList);

   return loadRes;
}

/**
 * @return the string is not alloc'ed and does not need to be freed
 */
const char* __Config_createDefaultCfgFilename(void)
{
   const char* retVal = "";

   struct file* cfgFile;

   mm_segment_t oldfs;

   // try to open the default config file
   cfgFile = filp_open(CONFIG_DEFAULT_CFGFILENAME, (O_RDONLY), 0);
   if(!IS_ERR(cfgFile) )
   { // this is something that we can open, so it's probably the config file ;)
      retVal = CONFIG_DEFAULT_CFGFILENAME;

      ACQUIRE_PROCESS_CONTEXT(oldfs);
      filp_close(cfgFile, NULL);
      RELEASE_PROCESS_CONTEXT(oldfs);
   }

   return retVal;
}

/**
 * Note: Cuts off lines that are longer than the buffer
 *
 * @param outEndOfFile fhgfs_true if end of file reached
 * @return fhgfs_false on file io error
 */
fhgfs_bool __Config_readLineFromFile(struct file* cfgFile,
   char* buf, size_t bufLen, fhgfs_bool* outEndOfFile)
{
   mm_segment_t oldfs;
   size_t numRead;
   fhgfs_bool endOfLine;
   fhgfs_bool erroroccurred;

   *outEndOfFile = fhgfs_false;
   endOfLine = fhgfs_false;
   erroroccurred = fhgfs_false;

   ACQUIRE_PROCESS_CONTEXT(oldfs);

   for(numRead = 0; numRead < (bufLen-1); numRead++)
   {
      char charBuf;

      ssize_t readRes = vfs_read(cfgFile, &charBuf, 1, &cfgFile->f_pos);

      if( (readRes > 0) && (charBuf == '\n') )
      { // end of line
         endOfLine = fhgfs_true;
         break;
      }
      else
      if(readRes > 0)
      { // any normal character
         buf[numRead] = charBuf;
      }
      else
      if(readRes == 0)
      { // end of file
         *outEndOfFile = fhgfs_true;
         break;
      }
      else
      { // error occurred
         printk_fhgfs(KERN_WARNING, "Failed to read from file at line position: %lld\n",
            (long long)numRead);

         erroroccurred = fhgfs_true;
         break;
      }

   }

   buf[numRead] = 0; // add terminating zero

   // read the rest of the line if it is longer than the buffer
   while(!endOfLine && !(*outEndOfFile) && !erroroccurred)
   {
      char charBuf;

      ssize_t readRes = vfs_read(cfgFile, &charBuf, 1, &cfgFile->f_pos);
      if( (readRes > 0) && (charBuf == '\n') )
         endOfLine = fhgfs_true;
      if(readRes == 0)
         *outEndOfFile = fhgfs_true;
      else
      if(readRes < 0)
         erroroccurred = fhgfs_true;
   }


   RELEASE_PROCESS_CONTEXT(oldfs);

   return !erroroccurred;

}

/**
 * Init values that are not directly set by the user, but computed from values that the user
 * has set.
 */
void __Config_initImplicitVals(Config* this)
{
   fhgfs_bool authHashRes;

   __Config_initConnNumCommRetries(this);
   __Config_initTuneFileCacheTypeNum(this);
   __Config_initSysInodeIDStyleNum(this);
   __Config_initLogTypeNum(this);

   // tuneNumWorkers
   if(!this->tuneNumWorkers)
      this->tuneNumWorkers = MAX(os_getNumOnlineCPUs(), 3);

   // tuneMsgBufNum
   if(!this->tuneMsgBufNum)
      this->tuneMsgBufNum = (os_getNumOnlineCPUs() * 4) + this->tuneNumWorkers + 1;

   // tuneMaxReadWriteNum
   if(!this->tuneMaxReadWriteNum)
      this->tuneMaxReadWriteNum = MAX(os_getNumOnlineCPUs() * 4, 4);

   // tuneFileCacheBufNum
   if(!this->tuneFileCacheBufNum)
      this->tuneFileCacheBufNum = MAX(os_getNumOnlineCPUs() * 4, 4);

   // tuneInodeBlockSize
   this->tuneInodeBlockSize = (1 << this->tuneInodeBlockBits);

   /* tuneUseBufferedAppend: allow only in combination with global locks
      (locally locked code paths are currently not prepared to work with buffering) */
   if(!this->tuneUseGlobalAppendLocks)
      this->tuneUseBufferedAppend = fhgfs_false;

   // connAuthHash
   authHashRes = __Config_initConnAuthHash(this, this->connAuthFile, &this->connAuthHash);
   if(!authHashRes)
   {
      this->configValid = fhgfs_false;
      return;
   }
}

void __Config_initConnNumCommRetries(Config* this)
{
   // note: keep in sync with MessagingTk_getRetryWaitMS()

   unsigned retrySecs = this->connCommRetrySecs;
   unsigned retryMS = retrySecs * 1000;

   unsigned numRetries;

   if(retrySecs <= 60)
   { // 12 x 5sec retries during 1st min
      numRetries = (retryMS + (5000-1) ) / 5000;
   }
   else
   if(retrySecs <= 300)
   { // 12 x 20sec retries during 2nd to 5th min
      numRetries = (retryMS + (20000-1) - 60000) / 20000; // without 1st min
      numRetries += 12; // add 1st min
   }
   else
   { // 60 sec retries after 5th min
      numRetries = (retryMS + (60000-1) - (60000*5) ) / 60000; // without first 5 mins
      numRetries += 12; // add 1st min
      numRetries += 12; // add 2nd to 5th min
   }

   this->connNumCommRetries = numRetries;
}

void __Config_initTuneFileCacheTypeNum(Config* this)
{
   const char* valueStr = this->tuneFileCacheType;

   if(!os_strnicmp(valueStr, FILECACHETYPE_NATIVE_STR, strlen(FILECACHETYPE_NATIVE_STR) ) )
      this->tuneFileCacheTypeNum = FILECACHETYPE_Native;
   else
   if(!os_strnicmp(valueStr, FILECACHETYPE_BUFFERED_STR, strlen(FILECACHETYPE_BUFFERED_STR) ) )
      this->tuneFileCacheTypeNum = FILECACHETYPE_Buffered;
   else
   if(!os_strnicmp(valueStr, FILECACHETYPE_PAGED_STR, os_strlen(FILECACHETYPE_PAGED_STR) ) )
      this->tuneFileCacheTypeNum = FILECACHETYPE_Paged;
   else
      this->tuneFileCacheTypeNum = FILECACHETYPE_None;
}

const char* Config_fileCacheTypeNumToStr(FileCacheType cacheType)
{
   switch(cacheType)
   {
      case FILECACHETYPE_Native:
         return FILECACHETYPE_NATIVE_STR;
      case FILECACHETYPE_Buffered:
         return FILECACHETYPE_BUFFERED_STR;
      case FILECACHETYPE_Paged:
         return FILECACHETYPE_PAGED_STR;

      default:
         return FILECACHETYPE_NONE_STR;
   }
}

void __Config_initSysInodeIDStyleNum(Config* this)
{
   const char* valueStr = this->sysInodeIDStyle;

   if(!os_strnicmp(valueStr, INODEIDSTYLE_HASH64HSIEH_STR,
      os_strlen(INODEIDSTYLE_HASH64HSIEH_STR) ) )
      this->sysInodeIDStyleNum = INODEIDSTYLE_Hash64HSieh;
   else
   if(!os_strnicmp(valueStr, INODEIDSTYLE_HASH32HSIEH_STR,
      os_strlen(INODEIDSTYLE_HASH32HSIEH_STR) ) )
      this->sysInodeIDStyleNum = INODEIDSTYLE_Hash32Hsieh;
   else
   if(!os_strnicmp(valueStr, INODEIDSTYLE_HASH64MD4_STR, os_strlen(INODEIDSTYLE_HASH64MD4_STR) ) )
      this->sysInodeIDStyleNum = INODEIDSTYLE_Hash64MD4;
   else
   if(!os_strnicmp(valueStr, INODEIDSTYLE_HASH32MD4_STR,  os_strlen(INODEIDSTYLE_HASH32MD4_STR) ) )
      this->sysInodeIDStyleNum = INODEIDSTYLE_Hash32MD4;
   else
   { // default
      printk_fhgfs(KERN_INFO, "Unknown Inode-Hash: %s. Defaulting to hash64md4.\n", valueStr);
      this->sysInodeIDStyleNum = INODEIDSTYLE_Default;
   }
}

const char* Config_inodeIDStyleNumToStr(InodeIDStyle inodeIDStyle)
{
   switch(inodeIDStyle)
   {
      case INODEIDSTYLE_Hash64HSieh:
         return INODEIDSTYLE_HASH64HSIEH_STR;

      case INODEIDSTYLE_Hash32MD4:
         return INODEIDSTYLE_HASH32MD4_STR;

      case INODEIDSTYLE_Hash64MD4:
         return INODEIDSTYLE_HASH64MD4_STR;

      default:
         return INODEIDSTYLE_HASH32HSIEH_STR;
   }
}

void __Config_initLogTypeNum(Config* this)
{
   const char* valueStr = this->logType;

   if(!os_strnicmp(valueStr, LOGGERTYPE_SYSLOG_STR, os_strlen(LOGGERTYPE_SYSLOG_STR) ) )
      this->logTypeNum = LOGTYPE_Syslog;
   else
      this->logTypeNum = LOGTYPE_Helperd;
}

const char* Config_logTypeNumToStr(LogType logType)
{
   switch(logType)
   {
      case LOGTYPE_Syslog:
         return LOGGERTYPE_SYSLOG_STR;

      default:
         return LOGGERTYPE_HELPERD_STR;
   }
}

/**
 * Generate connection authentication hash based on contents of given authentication file.
 *
 * @param outConnAuthHash will be set to 0 if file is not defined
 * @return fhgfs_true on success or unset file, fhgfs_false on error
 */
fhgfs_bool __Config_initConnAuthHash(Config* this, char* connAuthFile, uint64_t* outConnAuthHash)
{
   struct file* fileHandle;
   char* buf;
   mm_segment_t oldfs;
   ssize_t readRes;


   if(!connAuthFile || !StringTk_hasLength(connAuthFile) )
   {
      *outConnAuthHash = 0;
      return fhgfs_true; // no file given => no hash to be generated
   }


   // open file...

   fileHandle = filp_open(connAuthFile, O_RDONLY, 0);
   if(IS_ERR(fileHandle) )
   {
      printk_fhgfs(KERN_WARNING, "Failed to open auth file: '%s'\n", connAuthFile);
      return fhgfs_false;
   }

   // read file...

   buf = os_kmalloc(CONFIG_AUTHFILE_READSIZE);
   if(unlikely(!buf) )
   {
      printk_fhgfs(KERN_WARNING, "Failed to alloc mem for auth file reading: '%s'\n", connAuthFile);

      ACQUIRE_PROCESS_CONTEXT(oldfs);
      filp_close(fileHandle, NULL);
      RELEASE_PROCESS_CONTEXT(oldfs);

      return fhgfs_false;
   }


   ACQUIRE_PROCESS_CONTEXT(oldfs);

   readRes = vfs_read(fileHandle, buf, CONFIG_AUTHFILE_READSIZE, &fileHandle->f_pos);

   filp_close(fileHandle, NULL);

   RELEASE_PROCESS_CONTEXT(oldfs);


   if(readRes < 0)
   {
      printk_fhgfs(KERN_WARNING, "Unable to read auth file: '%s'\n", connAuthFile);
      return fhgfs_false;
   }

   // empty authFile is probably unintended, so treat it as error
   if(!readRes || (readRes < CONFIG_AUTHFILE_MINSIZE) )
   {
      printk_fhgfs(KERN_WARNING, "Auth file is empty or too small: '%s'\n", connAuthFile);
      return fhgfs_false;
   }


   { // hash file contents

      // (hsieh hash only generates 32bit hashes, so we make two hashes for 64 bits)

      int len1stHalf = readRes / 2;
      int len2ndHalf = readRes - len1stHalf;

      uint64_t high = HashTk_hash32(HASHTK_HSIEHHASH32, buf, len1stHalf);
      uint64_t low  = HashTk_hash32(HASHTK_HSIEHHASH32, &buf[len1stHalf], len2ndHalf);

      *outConnAuthHash = (high << 32) | low;
   }

   // clean up
   kfree(buf);


   return fhgfs_true;
}

