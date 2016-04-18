/*
 *  SetupFunctions is a collection of static functions to perform all kinds of
 *  things related to admon-based graphical BeeGFS setup. Many functions here
 *  call external scripts of the beegfs_setup package
 */

#ifndef SETUPFUNCTIONS_H_
#define SETUPFUNCTIONS_H_

#include <common/toolkit/StringTk.h>
#include <common/nodes/Node.h>
#include <components/ExternalJobRunner.h>
#include <common/Common.h>


enum RoleType
{
   ROLETYPE_Meta = 1,
   ROLETYPE_Storage = 2,
   ROLETYPE_Client = 3,
   ROLETYPE_Mgmtd = 4
};

enum Arch
{
   ARCH_32 = 1, ARCH_64 = 2, ARCH_ppc32 = 3, ARCH_ppc64 = 4
};

const std::string SETUP_DIR                  = "/opt/beegfs/setup";
const std::string CONFIG_FILE_PATH           = SETUP_DIR +"/confs/config";
const std::string META_ROLE_FILE_PATH        = SETUP_DIR + "/info/meta_server";
const std::string STORAGE_ROLE_FILE_PATH     = SETUP_DIR + "/info/storage_server";
const std::string CLIENT_ROLE_FILE_PATH      = SETUP_DIR + "/info/clients";
const std::string MGMTD_ROLE_FILE_PATH       = SETUP_DIR + "/info/management";
const std::string IB_CONFIG_FILE_PATH        = SETUP_DIR + "/info/ib_nodes";

const std::string TMP_DIR                    = "/tmp/beegfs";
const std::string SETUP_CLIENT_WARNING       = TMP_DIR + "/beegfs-setup-client-warnings.tmp";
const std::string FAILED_NODES_PATH          = TMP_DIR + "/failedNodes";
const std::string FAILED_SSH_CONNECTIONS     = TMP_DIR + "/beegfs-setup-check-ssh.tmp";

const std::string SETUP_LOG_PATH             = "/var/log/beegfs-setup.log";

const std::string ADMON_GUI_PATH             = "/opt/beegfs/beegfs-admon-gui/beegfs-admon-gui.jar";


struct InstallNodeInfo
{
      std::string name;
      Arch arch;
      std::string distributionName;
      std::string distributionTag;
};


typedef std::list<InstallNodeInfo> InstallNodeInfoList;
typedef InstallNodeInfoList::iterator InstallNodeInfoListIter;


class SetupFunctions
{
   public:
      static int writeConfigFile(std::map<std::string,
         std::string> *paramsMap);
      static int writeIBFile(std::map<std::string, std::string> *paramsMap);
      static int readIBFile(StringList *hosts, StringMap *outMap);
      static int readConfigFile(std::string *paramsStr);
      static int readConfigFile(std::map<std::string,
         std::string> *paramsMap);
      static int changeConfig(std::map<std::string, std::string> *paramsMap,
         StringList *failedNodes);
      static int writeRolesFile(RoleType roleType, StringList *nodeNames);
      static int readRolesFile(RoleType roleType, StringList *nodeNames);
      static int readRolesFile(RoleType roleType, StringVector *nodeNames);
      static int detectIBPaths(std::string host, StringList *incPaths,
         StringList *libPaths,
         StringList *kernelIncPaths);
      static int detectIBPaths(std::string host, std::string *incPath,
         std::string *libPath,
         std::string *kernelIncPath);
      static int getInstallNodeInfo(InstallNodeInfoList *metaInfo,
         InstallNodeInfoList *storageInfo, InstallNodeInfoList *clientInfo);
      static int getInstallNodeInfo(InstallNodeInfoList *metaInfo,
         InstallNodeInfoList *storageInfo, InstallNodeInfoList *clientInfo,
         InstallNodeInfoList *mgmtdInfo);
      static int getInstallNodeInfo(RoleType roleType,
         InstallNodeInfoList *info);
      static int file2HTML(std::string filename, std::string *outStr);
      static int getFileContents(std::string filename, std::string *outStr);
      static int createRepos(StringList *failedNodes);
      static int createNewSetupLogfile(std::string type);
      static int installPackage(std::string package,
         StringList *failedNodes);
      static int installSinglePackage(std::string package,
         std::string host);
      static int uninstallPackage(std::string package,
         StringList *failedNodes);
      static int uninstallSinglePackage(std::string package,
         std::string host);
      static int startService(std::string service,
         StringList *failedNodes);
      static int startService(std::string service, std::string host);
      static int stopService(std::string service, StringList *failedNodes);
      static int stopService(std::string service, std::string host);
      static int checkStatus(std::string service, std::string host,
         std::string *status);
      static int checkStatus(std::string service, StringList *runningHosts,
         StringList *downHosts);
      static int checkDistriArch();
      static void restartAdmon();
      static int checkSSH(StringList *hosts, StringList *failedNodes);
      static int getCheckClientDependenciesLog(std::string *outStr);
      static int getLogFile(NodeType nodeType, uint16_t nodeNumID, uint lines,
         std::string *outStr, std::string nodeID);
      static int updateAdmonConfig();

   private:
      SetupFunctions() {}

      static int checkFailedNodesFile(StringList *failedNodes);

      // inliners
      /**
       * @return 0 on success (or if file didn't exist), !=0 otherwise
       */
      static int saveRemoveFile(std::string filename)
      {
         int removeRes = remove(filename.c_str() );

         if(removeRes && (errno != ENOENT) )
            return removeRes;

         return 0;
      }
};

#endif /* SETUPFUNCTIONS_H_ */
