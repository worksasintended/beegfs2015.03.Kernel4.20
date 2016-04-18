#include <program/Program.h>
#include "SetupFunctions.h"


#define CLIENT_LOGFILEPATH "/var/log/beegfs-client.log"


const std::string BASH                          = "/bin/bash";

const std::string SCRIPT_CHECK_SSH_PARALLEL     = SETUP_DIR + "/setup.check_ssh_parallel.sh";
const std::string SCRIPT_CHECK_STATUS           = SETUP_DIR + "/setup.check_status.sh";
const std::string SCRIPT_CHECK_STATUS_PARALLEL  = SETUP_DIR + "/setup.check_status_parallel.sh";
const std::string SCRIPT_GET_REMOTE_FILE        = SETUP_DIR + "/setup.get_remote_file.sh";
const std::string SCRIPT_INSTALL                = SETUP_DIR + "/setup.install.sh";
const std::string SCRIPT_INSTALL_PARALLEL       = SETUP_DIR + "/setup.install_parallel.sh";
const std::string SCRIPT_UNINSTALL              = SETUP_DIR + "/setup.uninstall.sh";
const std::string SCRIPT_UNINSTALL_PARALLEL     = SETUP_DIR + "/setup.uninstall_parallel.sh";
const std::string SCRIPT_REPO_PARALLEL          = SETUP_DIR + "/setup.add_repo_parallel.sh";
const std::string SCRIPT_START                  = SETUP_DIR + "/setup.start.sh";
const std::string SCRIPT_START_PARALLEL         = SETUP_DIR + "/setup.start_parallel.sh";
const std::string SCRIPT_STOP                   = SETUP_DIR + "/setup.stop.sh";
const std::string SCRIPT_STOP_PARALLEL          = SETUP_DIR + "/setup.stop_parallel.sh";
const std::string SCRIPT_SYSTEM_INFO_PARALLEL   = SETUP_DIR + "/setup.get_system_info_parallel.sh";
const std::string SCRIPT_WRITE_CONFIG           = SETUP_DIR + "/setup.write_config.sh";
const std::string SCRIPT_WRITE_CONFIG_PARALLEL  = SETUP_DIR + "/setup.write_config_parallel.sh";
const std::string SCRIPT_WRITE_MOUNTS           = SETUP_DIR + "/setup.write_mounts.sh";



int SetupFunctions::writeConfigFile(std::map<std::string, std::string> *paramsMap)
{
   if (saveRemoveFile(CONFIG_FILE_PATH) != 0)
      return -1;

   std::ofstream confFile(CONFIG_FILE_PATH.c_str(), std::ios::app);
   if (!(confFile.is_open()))
      return -1;

   for (std::map<std::string, std::string>::iterator iter = paramsMap->begin(); iter
      != paramsMap->end(); iter++)
   {
      confFile << (*iter).first + "=" + (*iter).second + "\n";
   }

   StringList mgmtdHosts;
   readRolesFile(ROLETYPE_Mgmtd, &mgmtdHosts);
   if (mgmtdHosts.empty())
   {
      confFile << "sysMgmtdHost=\n";
   }
   else
   {
      confFile << "sysMgmtdHost=" + mgmtdHosts.front() + "\n";
   }

   confFile.close();

   return 0;
}

int SetupFunctions::readConfigFile(std::string *paramsStr)
{
   *paramsStr = "";
   std::ifstream confFile(CONFIG_FILE_PATH.c_str() );

   if (!(confFile.is_open()))
      return -1;

   while (!confFile.eof())
   {
      std::string line;
      getline(confFile, line);
      *paramsStr += line + "&";
   }

   confFile.close();
   return 0;
}

/*
 * This method is unused in the moment but in the future with the fix it is helpful to update the
 * configuration with out a new installation of all components.
 */
int SetupFunctions::changeConfig(std::map<std::string, std::string> *paramsMap,
   StringList *failedNodes)
{

   saveRemoveFile(FAILED_NODES_PATH);
   if (saveRemoveFile(CONFIG_FILE_PATH) != 0)
      return -1;

   std::ofstream confFile(CONFIG_FILE_PATH.c_str(), std::ios::app);
   if (!(confFile.is_open()))
      return -1;

   for (std::map<std::string, std::string>::iterator iter = paramsMap->begin(); iter
      != paramsMap->end(); iter++)
   {
      confFile << (*iter).first + "=" + (*iter).second + "\n";
   }

   StringList mgmtdHosts;
   readRolesFile(ROLETYPE_Mgmtd, &mgmtdHosts);
   confFile << "sysMgmtdHost=" + mgmtdHosts.front() + "\n";

   confFile.close();

   //TODO Frank replace for update config, use SCRIPT_WRITE_CONFIG_PARALLEL
   /*
   ExternalJobRunner *jobRunner = Program::getApp()->getJobRunner();

   Mutex mutexB;
   SafeMutexLock mutexLockB(&mutexB);

   Job *jobB = jobRunner->addJob(BASH + " " + SCRIPT_WRITE_CONFIG_PARALLEL + " " +
      FAILED_NODES_PATH, &mutexB);

   while(!jobB->finished)
      jobB->jobFinishedCond.wait(&mutexB);

   mutexLockB.unlock();

   if (jobB->returnCode != 0)
   {
      jobRunner->delJob(jobB->id);
      return -1;
   }

   jobRunner->delJob(jobB->id);

   if(checkFailedNodesFile(failedNodes) )
      return -1;
   */

   return 0;
}

int SetupFunctions::readConfigFile(std::map<std::string, std::string> *paramsMap)
{
   std::ifstream confFile(CONFIG_FILE_PATH.c_str() );
   if (!(confFile.is_open()))
      return -1;

   while (!confFile.eof())
   {
      std::string line;
      getline(confFile, line);
      StringList lineSplit;
      StringTk::explode(line, '=', &lineSplit);

      if (lineSplit.size() > 1)
         (*paramsMap)[lineSplit.front()] = lineSplit.back();
      else
      if (lineSplit.size() == 1)
         (*paramsMap)[lineSplit.front()] = "";
   }

   confFile.close();
   return 0;
}

int SetupFunctions::writeRolesFile(RoleType roleType, StringList *nodeNames)
{
   std::string filename;

   switch (roleType)
   {
      case ROLETYPE_Meta:
         filename = META_ROLE_FILE_PATH;
         break;
      case ROLETYPE_Storage:
         filename = STORAGE_ROLE_FILE_PATH;
         break;
      case ROLETYPE_Client:
         filename = CLIENT_ROLE_FILE_PATH;
         break;
      case ROLETYPE_Mgmtd:
         filename = MGMTD_ROLE_FILE_PATH;
         break;
   }

   if (saveRemoveFile(filename) != 0)
      return -1;

   std::ofstream rolesFile(filename.c_str(), std::ios::app);
   if (!(rolesFile.is_open()))
      return -1;

   for (StringListIter iter = nodeNames->begin(); iter != nodeNames->end(); iter++)
   {
      rolesFile << (*iter) + "\n";
   }

   rolesFile.close();

   // rewrite the config if the mgmtd has changed
   if (roleType == ROLETYPE_Mgmtd)
   {
      std::string paramsStr;
      readConfigFile(&paramsStr);

      StringMap paramsMap;
      WebTk::paramsToMap(paramsStr, &paramsMap);
      writeConfigFile(&paramsMap);
   }


   return 0;
}

int SetupFunctions::readRolesFile(RoleType roleType, StringList *outNodeNames)
{
   std::string filename;

   switch (roleType)
   {
      case ROLETYPE_Meta:
         filename = META_ROLE_FILE_PATH;
         break;
      case ROLETYPE_Storage:
         filename = STORAGE_ROLE_FILE_PATH;
         break;
      case ROLETYPE_Client:
         filename = CLIENT_ROLE_FILE_PATH;
         break;
      case ROLETYPE_Mgmtd:
         filename = MGMTD_ROLE_FILE_PATH;
         break;
   }

   std::ifstream rolesFile(filename.c_str());
   if (!(rolesFile.is_open()))
   {
      return -1;
   }

   while (!rolesFile.eof())
   {
      std::string line;
      getline(rolesFile, line);
      StringList splitLine;
      StringTk::explode(line, ',', &splitLine);

      if (!splitLine.empty())
      {
         std::string node = StringTk::trim(splitLine.front());
         if (node != "")
         {
            outNodeNames->push_back(node);
         }
      }
   }

   rolesFile.close();
   return 0;
}

int SetupFunctions::readRolesFile(RoleType roleType, StringVector *outNodeNames)
{
   StringList tmpList;
   int ret = readRolesFile(roleType, &tmpList);

   for (StringListIter iter = tmpList.begin(); iter != tmpList.end(); iter++)
   {
      outNodeNames->push_back(*iter);
   }

   return ret;
}

int SetupFunctions::writeIBFile(std::map<std::string, std::string> *paramsMap)
{
   if (saveRemoveFile(IB_CONFIG_FILE_PATH) != 0)
      return -1;

   std::ofstream ibConfFile(IB_CONFIG_FILE_PATH.c_str(), std::ios::app);
   if (!(ibConfFile.is_open()))
      return -1;

   StringList hostList;

   for (std::map<std::string, std::string>::iterator iter = paramsMap->begin();
      iter != paramsMap->end();
      iter++)
   {
      std::string line = (*iter).first;
      StringList tmpList;
      StringTk::explode(line, '_', &tmpList);
      std::string host = tmpList.front();
      hostList.push_back(host);
   }

   hostList.sort();
   hostList.unique();

   for (StringListIter iter = hostList.begin(); iter != hostList.end(); iter++)
   {
      if (StringTk::strToBool((*paramsMap)[std::string((*iter) + "_useIB")]))
         ibConfFile << (*iter) << ",OFED_KERNEL_INCLUDE_PATH=" << (*paramsMap)[std::string((*iter)
            + "_kernelibincpath")] << "\n";
   }

   ibConfFile.close();
   return 0;
}

int SetupFunctions::readIBFile(StringList *hosts, StringMap *outMap)
{
   std::ifstream ibConfFile(IB_CONFIG_FILE_PATH.c_str() );

   if (!(ibConfFile.is_open()))
      return -1;

   while (!ibConfFile.eof())
   {
      std::string line;
      getline(ibConfFile, line);
      StringList splitLine;
      StringTk::explode(line, ',', &splitLine);
      if (!splitLine.empty())
      {
         std::string node = StringTk::trim(splitLine.front());
         hosts->push_back(node);
         StringList splitOptions;
         StringTk::explode(splitLine.back(), ' ', &splitOptions);

         StringList splitParam;
         StringTk::explode(splitOptions.front(), '=', &splitParam);
         splitOptions.pop_front();

         if (splitParam.size() > 1)
         {
            (*outMap)[std::string((node) + "_kernelibincpath")] = splitParam.back();
         }
         else
         {
            (*outMap)[std::string((node) + "_kernelibincpath")] = "";
         }
      }
   }

   ibConfFile.close();
   return 0;
}

int SetupFunctions::getInstallNodeInfo(InstallNodeInfoList *metaInfo,
   InstallNodeInfoList *storageInfo, InstallNodeInfoList *clientInfo)
{
   int ret = 0;

   ret += getInstallNodeInfo(ROLETYPE_Meta, metaInfo);

   ret += getInstallNodeInfo(ROLETYPE_Storage, storageInfo);

   ret += getInstallNodeInfo(ROLETYPE_Client, clientInfo);

   return ret;
}

int SetupFunctions::getInstallNodeInfo(InstallNodeInfoList *metaInfo,
   InstallNodeInfoList *storageInfo, InstallNodeInfoList *clientInfo,
   InstallNodeInfoList *mgmtdInfo)
{
   int ret = 0;

   ret += getInstallNodeInfo(ROLETYPE_Meta, metaInfo);

   ret += getInstallNodeInfo(ROLETYPE_Storage, storageInfo);

   ret += getInstallNodeInfo(ROLETYPE_Client, clientInfo);

   ret += getInstallNodeInfo(ROLETYPE_Mgmtd, mgmtdInfo);

   return ret;
}

int SetupFunctions::getInstallNodeInfo(RoleType roleType, InstallNodeInfoList *info)
{
   std::string filename;

   switch (roleType)
   {
      case ROLETYPE_Meta:
         filename = META_ROLE_FILE_PATH;
         break;
      case ROLETYPE_Storage:
         filename = STORAGE_ROLE_FILE_PATH;
         break;
      case ROLETYPE_Client:
         filename = CLIENT_ROLE_FILE_PATH;
         break;
      case ROLETYPE_Mgmtd:
         filename = MGMTD_ROLE_FILE_PATH;
         break;
   }

   std::ifstream rolesFile(filename.c_str());
   if (!(rolesFile.is_open()))
   {
      return -1;
   }

   while (!rolesFile.eof())
   {
      std::string line;
      getline(rolesFile, line);

      if (StringTk::trim(line) != "")
      {
         StringList splitLine;
         StringTk::explode(line, ',', &splitLine);
         InstallNodeInfo nodeInfo;
         StringListIter iter = splitLine.begin();

         if (iter == splitLine.end())
            return -1;

         nodeInfo.name = StringTk::trim(*iter);

         if (++iter == splitLine.end())
            return -1;

         std::string arch = *iter;

         if (arch.compare("i586") == 0)
            nodeInfo.arch = ARCH_32;
         else
         if (arch.compare("x86_64") == 0)
            nodeInfo.arch = ARCH_64;
         else
         if (arch.compare("ppc") == 0)
            nodeInfo.arch = ARCH_ppc32;
         else
         if (arch.compare("ppc64") == 0)
            nodeInfo.arch = ARCH_ppc64;

         if (++iter == splitLine.end())
            return -1;

         nodeInfo.distributionName = *iter;
         if (++iter == splitLine.end())
            return -1;

         nodeInfo.distributionTag = *iter;
         info->push_back(nodeInfo);
      }
   }

   rolesFile.close();

   return 0;
}

int SetupFunctions::file2HTML(std::string filename, std::string *outStr)
{
   std::ifstream file(filename.c_str());

   if (!(file.is_open()))
      return -1;

   while (!file.eof())
   {
      std::string line;
      getline(file, line);
      *outStr += line + "<br>";
   }

   file.close();
   return 0;
}

/**
 * Reads a complete file into outStr.
 *
 * @return 0 on success, !=0 on error
 */
int SetupFunctions::getFileContents(std::string filename, std::string *outStr)
{
   std::ifstream file(filename.c_str());
   if(!(file.is_open() ) )
      return -1;

   while(!file.eof() && file.good() )
   {
      std::string line;
      getline(file, line);

      // filter escape characters
      /* note: ("\33" is octal.) this escape character is e.g. used to start coloring in bash. there
         are actually more characters for color codes following, but parsing those would be too
         complex here, so we only remove the escape char, for the admon GUI presentation. */

      if (line.empty() && file.eof())
      {
         break;
      }

      size_t pos = line.find('\33');
      while(pos != std::string::npos)
      {
         line.erase(pos, 1);
         pos = line.find('\33');
      }
      // note: "\r" is added here in addition to "\n" for windows GUI compatibility.
      *outStr += line + "\r\n";
   }

   file.close();

   return 0;
}

int SetupFunctions::createRepos(StringList *failedNodes)
{
   ExternalJobRunner *jobRunner = Program::getApp()->getJobRunner();

   saveRemoveFile(FAILED_NODES_PATH);

   Mutex mutex;
   SafeMutexLock mutexLock(&mutex);

   Job *job = jobRunner->addJob(BASH + " " + SCRIPT_REPO_PARALLEL + " " + FAILED_NODES_PATH,
      &mutex);

   while(!job->finished)
      job->jobFinishedCond.wait(&mutex);

   mutexLock.unlock();

   if (job->returnCode != 0)
   {
      jobRunner->delJob(job->id);
      return -1;
   }

   jobRunner->delJob(job->id);

   if(checkFailedNodesFile(failedNodes) )
      return -1;

   return 0;
}

int SetupFunctions::createNewSetupLogfile(std::string type)
{
   std::string mgmtdString;
   std::string metaString;
   std::string storageString;
   std::string clientsString;
   std::string field;
   std::string line;

   std::ifstream finStorage(STORAGE_ROLE_FILE_PATH.c_str() );
   while (std::getline(finStorage, line))
   {
      std::istringstream linestream(line);
      std::getline(linestream, field, ',');
      storageString = storageString + field + ", ";
   }
   finStorage.close();

   std::ifstream finClient(CLIENT_ROLE_FILE_PATH.c_str() );
   while (std::getline(finClient, line))
   {
      std::istringstream linestream(line);
      std::getline(linestream, field, ',');
      clientsString = clientsString + field + ", ";
   }
   finClient.close();

   std::ifstream finMgmtd(MGMTD_ROLE_FILE_PATH.c_str() );
   while (std::getline(finMgmtd, line))
   {
      std::istringstream linestream(line);
      std::getline(linestream, field, ',');
      mgmtdString = mgmtdString + field + ", ";
   }
   finMgmtd.close();

   std::ifstream finMeta(META_ROLE_FILE_PATH.c_str() );
   while (std::getline(finMeta, line))
   {
      std::istringstream linestream(line);
      std::getline(linestream, field, ',');
      metaString = metaString + field + ", ";
   }
   finMeta.close();

   std::string destPath(SETUP_LOG_PATH);
   destPath.append(".old");
   int retValue = rename(SETUP_LOG_PATH.c_str(), destPath.c_str() );

   std::ofstream fout(SETUP_LOG_PATH.c_str() );
   fout << "--------------------------------------------" << std::endl;

   if (type == "install")
   {
      fout << "starting installation..."  << std::endl;
   } else
   {
      fout << "starting uninstallation..."  << std::endl;
   }

   fout << "--------------------------------------------" << std::endl;
   fout << "Managment server: " << mgmtdString << std::endl;
   fout << "Metadata server: " << metaString << std::endl;
   fout << "Storage server: " << storageString << std::endl;
   fout << "Clients: " << clientsString << std::endl;
   fout << "--------------------------------------------" << std::endl;
   fout << "--------------------------------------------" << std::endl;
   fout << std::endl;
   fout.close();

   return retValue;
}

int SetupFunctions::installSinglePackage(std::string package, std::string host)
{
   ExternalJobRunner *jobRunner = Program::getApp()->getJobRunner();

   Mutex mutex;
   SafeMutexLock mutexLock(&mutex);

   Job *job = jobRunner->addJob(BASH + " " + SCRIPT_INSTALL + " " + package + " " + host, &mutex);

   while(!job->finished)
      job->jobFinishedCond.wait(&mutex);

   mutexLock.unlock();

   if(job->returnCode != 0)
   {
      jobRunner->delJob(job->id);
      return -1;
   }

   jobRunner->delJob(job->id);
   return 0;
}

int SetupFunctions::installPackage(std::string package, StringList *failedNodes)
{
   ExternalJobRunner *jobRunner = Program::getApp()->getJobRunner();

   saveRemoveFile(FAILED_NODES_PATH);

   Mutex mutex;
   SafeMutexLock mutexLock(&mutex);

   Job *job = jobRunner->addJob(BASH + " " + SCRIPT_INSTALL_PARALLEL + " " + package + " " +
      FAILED_NODES_PATH, &mutex);

   while(!job->finished)
      job->jobFinishedCond.wait(&mutex);

   mutexLock.unlock();

   if (job->returnCode != 0)
   {
      jobRunner->delJob(job->id);
      return -1;
   }

   jobRunner->delJob(job->id);

   if(checkFailedNodesFile(failedNodes) )
      return -1;

   return 0;
}

int SetupFunctions::uninstallSinglePackage(std::string package, std::string host)
{
   ExternalJobRunner *jobRunner = Program::getApp()->getJobRunner();

   Mutex mutex;
   SafeMutexLock mutexLock(&mutex);

   Job *job = jobRunner->addJob(BASH + " " + SCRIPT_UNINSTALL + " " + package + " " + host, &mutex);

   while(!job->finished)
      job->jobFinishedCond.wait(&mutex);

   mutexLock.unlock();

   if (job->returnCode != 0)
   {
      jobRunner->delJob(job->id);
      return -1;
   }

   jobRunner->delJob(job->id);
   return 0;
}

int SetupFunctions::uninstallPackage(std::string package, StringList *failedNodes)
{
   ExternalJobRunner *jobRunner = Program::getApp()->getJobRunner();

   saveRemoveFile(FAILED_NODES_PATH);

   Mutex mutex;
   SafeMutexLock mutexLock(&mutex);

   Job *job = jobRunner->addJob(BASH + " " + SCRIPT_UNINSTALL_PARALLEL + " " + package + " " +
      FAILED_NODES_PATH, &mutex);

   while(!job->finished)
      job->jobFinishedCond.wait(&mutex);

   mutexLock.unlock();

   if (job->returnCode != 0)
   {
      jobRunner->delJob(job->id);
      return -1;
   }

   jobRunner->delJob(job->id);

   if(checkFailedNodesFile(failedNodes) )
      return -1;

   return 0;
}

int SetupFunctions::startService(std::string service, StringList *failedNodes)
{
   ExternalJobRunner *jobRunner = Program::getApp()->getJobRunner();

   Mutex mutex;
   SafeMutexLock mutexLock(&mutex);

   Job *job = jobRunner->addJob(BASH + " " + SCRIPT_START_PARALLEL + " " + service, &mutex);

   while(!job->finished)
      job->jobFinishedCond.wait(&mutex);

   mutexLock.unlock();

   std::ifstream outFile((job->outputFile).c_str());
   if (!(outFile.is_open()))
   {
      jobRunner->delJob(job->id);
      return -2;
   }

   while (!outFile.eof())
   {
      std::string line;
      getline(outFile, line);
      if (StringTk::trim(line) != "")
      {
         failedNodes->push_back(line);
      }
   }

   outFile.close();

   jobRunner->delJob(job->id);

   if (!failedNodes->empty())
   {
      return -1;
   }

   return 0;
}

int SetupFunctions::startService(std::string service, std::string host)
{
   ExternalJobRunner *jobRunner = Program::getApp()->getJobRunner();

   Mutex mutex;
   SafeMutexLock mutexLock(&mutex);

   Job *job = jobRunner->addJob(BASH + " " + SCRIPT_START_PARALLEL + " " + service + " " + host,
      &mutex);

   while(!job->finished)
      job->jobFinishedCond.wait(&mutex);

   mutexLock.unlock();

   int retVal = job->returnCode;
   jobRunner->delJob(job->id);

   return retVal;
}

int SetupFunctions::stopService(std::string service, StringList *failedNodes)
{
   ExternalJobRunner *jobRunner = Program::getApp()->getJobRunner();

   Mutex mutex;
   SafeMutexLock mutexLock(&mutex);

   Job *job = jobRunner->addJob(BASH + " " + SCRIPT_STOP_PARALLEL + " " + service, &mutex);

   while(!job->finished)
      job->jobFinishedCond.wait(&mutex);

   mutexLock.unlock();

   std::ifstream outFile((job->outputFile).c_str());
   if (!(outFile.is_open()))
   {
      jobRunner->delJob(job->id);
      return -2;
   }

   while (!outFile.eof())
   {
      std::string line;
      getline(outFile, line);
      if (StringTk::trim(line) != "")
      {
         failedNodes->push_back(line);
      }
   }

   outFile.close();

   jobRunner->delJob(job->id);

   if (!failedNodes->empty())
   {
      return -1;
   }

   return 0;
}


int SetupFunctions::stopService(std::string service, std::string host)
{
   ExternalJobRunner *jobRunner = Program::getApp()->getJobRunner();

   Mutex mutex;
   SafeMutexLock mutexLock(&mutex);

   Job *job = jobRunner->addJob(BASH + " " + SCRIPT_STOP + " " + service + " " + host, &mutex);

   while(!job->finished)
      job->jobFinishedCond.wait(&mutex);

   mutexLock.unlock();

   int retVal = job->returnCode;
   jobRunner->delJob(job->id);

   return retVal;
}

int SetupFunctions::checkStatus(std::string service, std::string host, std::string *status)
{
   ExternalJobRunner *jobRunner = Program::getApp()->getJobRunner();

   Mutex mutex;
   SafeMutexLock mutexLock(&mutex);

   Job *job = jobRunner->addJob(BASH + " " + SCRIPT_CHECK_STATUS + " " + service
      + " " + host, &mutex);

   while(!job->finished)
      job->jobFinishedCond.wait(&mutex);

   mutexLock.unlock();

   if (job->returnCode != 0)
   {
      jobRunner->delJob(job->id);
      return -1;
   }

   std::ifstream outFile((job->outputFile).c_str());
   if (!(outFile.is_open()))
   {
      jobRunner->delJob(job->id);
      return -2;
   }

   while (!outFile.eof())
   {
      std::string line;
      getline(outFile, line);
      *status += line;
   }

   outFile.close();

   jobRunner->delJob(job->id);

   return 0;
}

int SetupFunctions::checkStatus(std::string service, StringList *runningHosts,
   StringList *stoppedHosts)
{
   ExternalJobRunner *jobRunner = Program::getApp()->getJobRunner();

   Mutex mutex;
   SafeMutexLock mutexLock(&mutex);

   Job *job = jobRunner->addJob(BASH + " " + SCRIPT_CHECK_STATUS_PARALLEL + " " + service, &mutex);

   while(!job->finished)
      job->jobFinishedCond.wait(&mutex);

   mutexLock.unlock();

   std::ifstream outFile((job->outputFile).c_str());
   if (!(outFile.is_open()))
   {
      jobRunner->delJob(job->id);
      return -2;
   }

   while (!outFile.eof())
   {
      std::string line;
      getline(outFile, line);
      StringList parsedLine;
      StringTk::explode(line, ',', &parsedLine);

      if (parsedLine.size() < 2)
         continue;

      if (StringTk::strToBool(parsedLine.back()))
      {
         runningHosts->push_back(parsedLine.front());
      }
      else
      {
         stoppedHosts->push_back(parsedLine.front());
      }
   }

   outFile.close();

   jobRunner->delJob(job->id);

   return 0;
}

int SetupFunctions::checkDistriArch()
{
   ExternalJobRunner *jobRunner = Program::getApp()->getJobRunner();

   Mutex mutex;
   SafeMutexLock mutexLock(&mutex);

   Job *job = jobRunner->addJob(BASH + " " + SCRIPT_SYSTEM_INFO_PARALLEL + " ", &mutex);

   while(!job->finished)
      job->jobFinishedCond.wait(&mutex);

   mutexLock.unlock();

   if (job->returnCode != 0)
   {
      jobRunner->delJob(job->id);
      return -1;
   }

   jobRunner->delJob(job->id);
   return 0;
}

void SetupFunctions::restartAdmon()
{
   Program::setRestart(true);
   Program::getApp()->stopComponents();
}

int SetupFunctions::checkSSH(StringList *hosts, StringList *failedNodes)
{
   ExternalJobRunner *jobRunner = Program::getApp()->getJobRunner();

   std::string hostList;

   for (StringListIter iter = hosts->begin(); iter != hosts->end(); iter++)
   {
      hostList = *iter + "," + hostList;
   }

   Mutex mutex;
   SafeMutexLock mutexLock(&mutex);

   Job *job = jobRunner->addJob(BASH + " " + SCRIPT_CHECK_SSH_PARALLEL + " " + hostList, &mutex);

   while(!job->finished)
      job->jobFinishedCond.wait(&mutex);

   mutexLock.unlock();

   if (job->returnCode != 0)
   {
      jobRunner->delJob(job->id);
      return -1;
   }

   std::ifstream failedNodesFile(FAILED_SSH_CONNECTIONS.c_str() );
   if (!(failedNodesFile.is_open()))
   {
      return -2;
   }

   while (!failedNodesFile.eof())
   {
      std::string line;
      getline(failedNodesFile, line);
      if (StringTk::trim(line) != "")
      {
         failedNodes->push_back(line);
      }
   }

   failedNodesFile.close();
   if (!failedNodes->empty())
   {
      return -1;
   }

   saveRemoveFile(FAILED_SSH_CONNECTIONS);

   jobRunner->delJob(job->id);

   return 0;
}

int SetupFunctions::getLogFile(NodeType nodeType, uint16_t nodeNumID, uint lines,
   std::string *outStr, std::string nodeID)
{
   *outStr = "";

   // get the path to the log file

   std::string logFilePath;

   if (nodeType == NODETYPE_Mgmt)
   {
      NodeStoreMgmtEx *nodeStore = Program::getApp()->getMgmtNodes();
      Node *node = nodeStore->referenceNode(nodeNumID);
      if (node == NULL)
         return -1;

      logFilePath = ( (MgmtNodeEx*)node)->getGeneralInformation().logFilePath;
      nodeID = ( (MgmtNodeEx*)node)->getID();
      nodeStore->releaseNode(&node);
   }
   else
   if (nodeType == NODETYPE_Meta)
   {
      NodeStoreMetaEx *nodeStore = Program::getApp()->getMetaNodes();
      Node *node =  nodeStore->referenceNode(nodeNumID);
      if (node == NULL)
         return -1;

      logFilePath = ( (MetaNodeEx*)node)->getGeneralInformation().logFilePath;
      nodeID = ( (MetaNodeEx*)node)->getID();
      nodeStore->releaseNode(&node);
   }
   else
   if (nodeType == NODETYPE_Storage)
   {
      NodeStoreStorageEx *nodeStore = Program::getApp()->getStorageNodes();
      Node *node =  nodeStore->referenceNode(nodeNumID);
      if (node == NULL)
         return -1;

      logFilePath = ( (StorageNodeEx*)node)->getGeneralInformation().logFilePath;
      nodeID = ( (StorageNodeEx*)node)->getID();
      nodeStore->releaseNode(&node);
   }
   else
   if (nodeType == NODETYPE_Client)
   {
      logFilePath = CLIENT_LOGFILEPATH;

      StringList idParts;
      StringTk::explode(nodeID, '-', &idParts);
      if(idParts.size() == 3)
         nodeID = idParts.back();
   }

   ExternalJobRunner *jobRunner = Program::getApp()->getJobRunner();

   Mutex mutex;
   SafeMutexLock mutexLock(&mutex);

   std::string cmdLine = BASH + " " + SCRIPT_GET_REMOTE_FILE + " " + nodeID + " " + logFilePath +
      " " + StringTk::uintToStr(lines);
   Job *job = jobRunner->addJob(cmdLine, &mutex);

   while(!job->finished)
      job->jobFinishedCond.wait(&mutex);

   mutexLock.unlock();

   if(job->returnCode == 0)
   {
      SetupFunctions::getFileContents(job->outputFile, outStr);
   }
   else
   {
      jobRunner->delJob(job->id);
      return -1;
   }

   jobRunner->delJob(job->id);
   return 0;
}

int SetupFunctions::updateAdmonConfig()
{
   ExternalJobRunner *jobRunner = Program::getApp()->getJobRunner();

   Mutex mutex;
   SafeMutexLock mutexLock(&mutex);

   std::string cfgFile = Program::getApp()->getConfig()->getCfgFile();

   Job *job = jobRunner->addJob(BASH + " " + SCRIPT_WRITE_CONFIG + " localhost " + cfgFile + " " +
      SETUP_LOG_PATH, &mutex);

   while(!job->finished)
      job->jobFinishedCond.wait(&mutex);

   mutexLock.unlock();

   Program::getApp()->getConfig()->resetMgmtdDaemon();

   if (job->returnCode != 0)
   {
      jobRunner->delJob(job->id);
      return -1;
   }

   jobRunner->delJob(job->id);
   return 0;
}

/**
 * @return number of failed node or -1 on error
 */
int SetupFunctions::checkFailedNodesFile(StringList *failedNodes)
{
   // no failed nodes written to failedNodes file ==> no failed nodes
   if(!StorageTk::pathExists(FAILED_NODES_PATH) )
      return 0;

   std::ifstream failedNodesFile(FAILED_NODES_PATH.c_str() );
   if (!(failedNodesFile.is_open() ) )
   {
      return -1;
   }

   while (!failedNodesFile.eof() )
   {
      std::string line;
      getline(failedNodesFile, line);

      if (StringTk::trim(line) != "")
      {
         failedNodes->push_back(line);
      }
   }

   saveRemoveFile(FAILED_NODES_PATH);

   return failedNodes->size();
}
