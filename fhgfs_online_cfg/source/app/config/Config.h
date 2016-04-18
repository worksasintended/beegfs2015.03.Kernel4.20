#ifndef CONFIG_H_
#define CONFIG_H_

#include <common/app/config/AbstractConfig.h>


#define CONFIG_DEFAULT_CFGFILENAME "/etc/beegfs/beegfs-client.conf"

#define RUNMODE_HELP_KEY_STRING     "--help" /* key for usage help */
#define __RUNMODES_SIZE \
   ( (sizeof(__RunModes) ) / (sizeof(RunModesElem) ) - 1) /* -1 because last elem is NULL */


// Note: Keep in sync with __RunModes array
enum RunMode
{
   RunMode_GETENTRYINFO          =     0,
   RunMode_SETPATTERN,
   RunMode_LISTNODES,
   RunMode_DISPOSEUNUSED,
   RunMode_CREATEFILE,
   RunMode_CREATEDIR,
   RunMode_REMOVENODE,
   RunMode_IOSTAT,
   RunMode_SERVERSTATS, // just an alias for RunMode_IOSTAT
   RunMode_IOTEST,
   RunMode_REFRESHENTRYINFO,
   RunMode_REMOVEDIRENTRY,
   RunMode_MAPTARGET,
   RunMode_REMOVETARGET,
   RunMode_LISTTARGETS,
   RunMode_IOCTL,
   RunMode_GENERICDEBUG,
   RunMode_CLIENTSTATS,
   RunMode_USERSTATS,
   RunMode_REFRESHALLENTRIES,
   RunMode_FIND,
   RunMode_MIGRATE,
   RunMode_REVERSELOOKUP,
   RunMode_STORAGEBENCH,
   RunMode_SETMETADATAMIRRORING,
   RunMode_GETQUOTAINFO,
   RunMode_HASHDIR,
   RunMode_SETQUOTALIMITS,
   RunMode_ADDMIRRORBUDDYGROUP,
   RunMode_LISTMIRRORBUDDYGROUPS,
   RunMode_STARTSTORAGERESYNC,
   RunMode_STORAGERESYNCSTATS,
   RunMode_INVALID   /* not valid as index in RunModes array */
};


struct RunModesElem
{
   const char* modeString;
   enum RunMode runMode;
};

extern RunModesElem const __RunModes[];


class Config : public AbstractConfig
{
   public:
      Config(int argc, char** argv) throw(InvalidConfigException);

      enum RunMode determineRunMode();

      static enum RunMode determineRunMode(std::string modeNameStr);


   private:

      // configurables
      std::string mount; //special argument to detect configuration file by mount point

      bool        logEnabled;

      std::string connInterfacesFile;

      bool        debugRunComponentThreads;
      bool        debugRunStartupTests;
      bool        debugRunComponentTests;
      bool        debugRunIntegrationTests;
      bool        debugRunThroughputTests;

      unsigned    tuneNumWorkers;
      std::string tunePreferredMetaFile;
      std::string tunePreferredStorageFile;

      bool        runDaemonized;

      // internals

      virtual void loadDefaults(bool addDashes);
      virtual void applyConfigMap(bool enableException, bool addDashes)
         throw(InvalidConfigException);
      virtual void initImplicitVals() throw(InvalidConfigException);
      std::string createDefaultCfgFilename();


   public:
      // getters & setters
      StringMap* getUnknownConfigArgs()
      {
         return getConfigMap();
      }

      std::string getMount() const
      {
         return mount;
      }

      bool getLogEnabled() const
      {
         return logEnabled;
      }

      std::string getConnInterfacesFile() const
      {
         return connInterfacesFile;
      }

      bool getDebugRunComponentThreads() const
      {
         return debugRunComponentThreads;
      }

      bool getDebugRunStartupTests() const
      {
         return debugRunStartupTests;
      }

      bool getDebugRunComponentTests() const
      {
         return debugRunComponentTests;
      }

      bool getDebugRunIntegrationTests() const
      {
         return debugRunIntegrationTests;
      }

      bool getDebugRunThroughputTests() const
      {
         return debugRunThroughputTests;
      }

      unsigned getTuneNumWorkers() const
      {
         return tuneNumWorkers;
      }

      std::string getTunePreferredMetaFile() const
      {
         return tunePreferredMetaFile;
      }

      std::string getTunePreferredStorageFile() const
      {
         return tunePreferredStorageFile;
      }

      bool getRunDaemonized() const
      {
         return runDaemonized;
      }
};

#endif /*CONFIG_H_*/
