#include <app/App.h>
#include <common/net/message/nodes/StorageBenchControlMsgResp.h>
#include <components/benchmarker/StorageBenchOperator.h>
#include <program/Program.h>
#include "StorageBenchControlMsgEx.h"

bool StorageBenchControlMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
         char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   const char* logContext = "StorageBenchControlMsg incoming";

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG(logContext, Log_DEBUG, std::string("Received a StorageBenchControlMsg from: ") +
      peer);

   StorageBenchResultsMap results;
   int cmdErrorCode = STORAGEBENCH_ERROR_NO_ERROR;

   App* app = Program::getApp();
   StorageBenchOperator* storageBench = app->getStorageBenchOperator();

   UInt16List targetIDs;
   parseTargetIDs(&targetIDs);

   switch(getAction())
   {
      case StorageBenchAction_START:
      {
         cmdErrorCode = storageBench->initAndStartStorageBench(&targetIDs, getBlocksize(),
            getSize(), getThreads(), getType() );
      } break;

      case StorageBenchAction_STOP:
      {
         cmdErrorCode = storageBench->stopBenchmark();
      } break;

      case StorageBenchAction_STATUS:
      {
         storageBench->getStatusWithResults(&targetIDs, &results);
         cmdErrorCode = STORAGEBENCH_ERROR_NO_ERROR;
      } break;

      case StorageBenchAction_CLEANUP:
      {
         cmdErrorCode = storageBench->cleanup(&targetIDs);
      } break;

      default:
      {
         LogContext(logContext).logErr("unknown action!");
      } break;
   }

   int errorCode;

   // check if the last command from the fhgfs_cmd was successful,
   // if not send the error code of the command to the fhgfs_cmd
   // if it was successful, send the error code of the last run or acutely run of the benchmark
   if (cmdErrorCode != STORAGEBENCH_ERROR_NO_ERROR)
   {
      errorCode = cmdErrorCode;
   }
   else
   {
      errorCode = storageBench->getLastRunErrorCode();
   }

   // send response
   StorageBenchControlMsgResp respMsg(storageBench->getStatus(), getAction(),
      storageBench->getType(), errorCode, results);
   respMsg.serialize(respBuf, bufLen);

   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   return true;
}
