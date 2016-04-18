#include <program/Program.h>
#include <common/net/message/helperd/LogRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include "LogMsgEx.h"


bool LogMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("LogMsgEx incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername(); 
   LOG_DEBUG_CONTEXT(log, 4, std::string("Received a LogMsg from: ") + peer);

   int logRes = 0;
   
   // Note: level 0 is reserved for error logging (logErr() )
   
   
   int logLevel = getLevel();
   Logger* logger = Program::getApp()->getLogger();
   const char* threadName = getThreadName();
   const char* context = getContext();
   const char* logMsg = getLogMsg();
   
   // note: we prepend an asterisk to remote threadNames, so that the user can see the difference
   std::string remoteThreadName = std::string("*") + threadName +
      "(" + StringTk::intToStr(getThreadID() ) + ")";
   
   if(logLevel)
   { // log standard message
      logger->logForcedWithThreadName(logLevel, remoteThreadName.c_str(), context, logMsg);
   }
   else
   { // log error message
      logger->logErrWithThreadName(remoteThreadName.c_str(), context, logMsg);
   }

   LogRespMsg respMsg(logRes);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return true;      
}


