#include <common/net/message/nodes/GenericDebugRespMsg.h>
#include <common/net/msghelpers/MsgHelperGenericDebug.h>
#include <common/toolkit/MessagingTk.h>
#include <program/Program.h>
#include "GenericDebugMsgEx.h"


#define GENDBGMSG_OP_VERSION              "version"
#define GENDBGMSG_OP_LISTPOOLS            "listpools"


bool GenericDebugMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("GenericDebugMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG_CONTEXT(log, Log_DEBUG, std::string("Received a GenericDebugMsg from: ") + peer);

   LOG_DEBUG_CONTEXT(log, Log_SPAM, std::string("Command string: ") + getCommandStr() );

   std::string cmdRespStr = processCommand();


   // send response

   GenericDebugRespMsg respMsg(cmdRespStr.c_str() );
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return true;
}

/**
 * @return command response string
 */
std::string GenericDebugMsgEx::processCommand()
{
   App* app = Program::getApp();

   std::string responseStr;
   std::string operation;

   // load command string into a stream to allow us to use getline
   std::istringstream commandStream(getCommandStr() );

   // get operation type from command string
   std::getline(commandStream, operation, ' ');

   if(operation == GENDBGMSG_OP_VERSION)
      responseStr = processOpVersion(commandStream);
   else
   if(operation == GENDBGMSG_OP_VARLOGMESSAGES)
      responseStr = MsgHelperGenericDebug::processOpVarLogMessages(commandStream);
   else
   if(operation == GENDBGMSG_OP_VARLOGKERNLOG)
      responseStr = MsgHelperGenericDebug::processOpVarLogKernLog(commandStream);
   else
   if(operation == GENDBGMSG_OP_FHGFSLOG)
      responseStr = MsgHelperGenericDebug::processOpFhgfsLog(commandStream);
   else
   if(operation == GENDBGMSG_OP_LOADAVG)
      responseStr = MsgHelperGenericDebug::processOpLoadAvg(commandStream);
   else
   if(operation == GENDBGMSG_OP_DROPCACHES)
      responseStr = MsgHelperGenericDebug::processOpDropCaches(commandStream);
   else
   if(operation == GENDBGMSG_OP_GETLOGLEVEL)
      responseStr = MsgHelperGenericDebug::processOpGetLogLevel(commandStream);
   else
   if(operation == GENDBGMSG_OP_SETLOGLEVEL)
      responseStr = MsgHelperGenericDebug::processOpSetLogLevel(commandStream);
   else
   if(operation == GENDBGMSG_OP_NETOUT)
      responseStr = MsgHelperGenericDebug::processOpNetOut(commandStream,
         app->getMgmtNodes(), app->getMetaNodes(), app->getStorageNodes() );
   else
   if(operation == GENDBGMSG_OP_LISTMETASTATES)
      responseStr = MsgHelperGenericDebug::processOpListTargetStates(commandStream,
         app->getMetaStateStore() );
   else
   if(operation == GENDBGMSG_OP_LISTSTORAGESTATES)
      responseStr = MsgHelperGenericDebug::processOpListTargetStates(commandStream,
         app->getTargetStateStore() );
   else
   if(operation == GENDBGMSG_OP_QUOTAEXCEEDED)
      responseStr = processOpQuotaExceeded(commandStream);
   else
   if(operation == GENDBGMSG_OP_USEDQUOTA)
      responseStr = processOpUsedQuota(commandStream);
   else
   if(operation == GENDBGMSG_OP_LISTPOOLS)
      responseStr = processOpListPools(commandStream);
   else
      responseStr = "Unknown/invalid operation";

   return responseStr;
}


std::string GenericDebugMsgEx::processOpVersion(std::istringstream& commandStream)
{
   return BEEGFS_VERSION;
}

std::string GenericDebugMsgEx::processOpQuotaExceeded(std::istringstream& commandStream)
{
   App* app = Program::getApp();

   std::string returnString;

   if(!app->getConfig()->getQuotaEnableEnforcment() )
      return "No quota exceeded IDs on this management daemon because quota enforcement is"
         "disabled.";

   returnString = MsgHelperGenericDebug::processOpQuotaExceeded(commandStream,
      app->getQuotaManager()->getExceededQuotaStore() );

   return returnString;
}

std::string GenericDebugMsgEx::processOpUsedQuota(std::istringstream& commandStream)
{
   App* app = Program::getApp();
   std::string returnString;

   if(!app->getConfig()->getQuotaEnableEnforcment() )
      return "No quota data on this management daemon because quota enforcement is disabled.";

   QuotaManager* quotaManager = app->getQuotaManager();

   QuotaDataType quotaDataType = QuotaDataType_NONE;
   bool forEachTarget = false;

   // get parameter from command string
   std::string inputString;
   while(!commandStream.eof() )
   {
      std::getline(commandStream, inputString, ' ');

      if(inputString == "uid")
         quotaDataType = QuotaDataType_USER;
      else
      if(inputString == "gid")
         quotaDataType = QuotaDataType_GROUP;
      else
      if(inputString == "forEachTarget")
         forEachTarget = true;
   }

   // verify given parameters
   if(quotaDataType == QuotaDataType_NONE)
      return "Invalid or missing quota data type argument.";

   if(quotaDataType == QuotaDataType_USER)
      returnString = quotaManager->usedQuotaUserToString(!forEachTarget);
   else
   if(quotaDataType == QuotaDataType_GROUP)
      returnString = quotaManager->usedQuotaGroupToString(!forEachTarget);

   return returnString;
}


/**
 * List internal state of meta and storage capacity pools.
 */
std::string GenericDebugMsgEx::processOpListPools(std::istringstream& commandStream)
{
   // protocol: no arguments

   App* app = Program::getApp();
   NodeCapacityPools* metaPools = app->getMetaCapacityPools();
   TargetCapacityPools* storagePools = app->getStorageCapacityPools();
   NodeCapacityPools* storageBuddyPools = app->getStorageBuddyCapacityPools();

   std::ostringstream responseStream;

   {
      std::string metaPoolsState;

      metaPools->getStateAsStr(metaPoolsState);

      responseStream << "META POOLS STATE: " << std::endl <<
         metaPoolsState << std::endl;
   }

   {
      std::string storagePoolsState;

      storagePools->getStateAsStr(storagePoolsState);

      responseStream << "STORAGE POOLS STATE: " << std::endl <<
         storagePoolsState << std::endl;
   }

   {
      std::string storagePoolsState;

      storageBuddyPools->getStateAsStr(storagePoolsState);

      responseStream << "STORAGE BUDDY POOLS STATE: " << std::endl <<
         storagePoolsState << std::endl;
   }

   return responseStream.str();
}
