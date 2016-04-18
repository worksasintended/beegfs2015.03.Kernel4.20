#include <common/app/log/LogContext.h>
#include <common/app/AbstractApp.h>
#include <common/threading/PThread.h>
#include <common/net/message/storage/mirroring/MirrorMetadataRespMsg.h>
#include <common/net/message/NetMessage.h>
#include <net/message/storage/mirroring/MirrorMetadataMsgEx.h>
#include <program/Program.h>
#include "MirrorMetadataWork.h"


void MirrorMetadataWork::process(char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen)
{
   communicate();

   // notify mirrorer about completion
   Program::getApp()->getMetadataMirrorer()->completeSubmittedTasks(nodeID);
}

FhgfsOpsErr MirrorMetadataWork::communicate()
{
   const char* logContext = "Mirror metadata work";

   App* app = Program::getApp();
   FhgfsOpsErr remoteRes;

   // prepare request

   MirrorMetadataMsgEx mirrorMsg(taskList, taskListNumElems, taskListSerialLen);

   RequestResponseArgs rrArgs(NULL, &mirrorMsg, NETMSGTYPE_MirrorMetadataResp);

   RequestResponseNode rrNode(nodeID, app->getMetaNodes() );
   rrNode.setTargetStates(app->getMetaStateStore() );

   // send request to node and receive response

   FhgfsOpsErr requestRes = MessagingTk::requestResponseNode(&rrNode, &rrArgs);

   if(unlikely(requestRes != FhgfsOpsErr_SUCCESS) )
   { // communication error
      LogContext(logContext).log(Log_WARNING,
         "Communication with metadata server failed. "
         "nodeID: " + StringTk::uintToStr(nodeID) );
      return requestRes;
   }

   // correct response type received
   MirrorMetadataRespMsg* mirrorRespMsg = (MirrorMetadataRespMsg*)rrArgs.outRespMsg;

   remoteRes = mirrorRespMsg->getResult();
   if(unlikely(remoteRes != FhgfsOpsErr_SUCCESS) )
   { // server returned an error
      LogContext(logContext).log(Log_WARNING,
         "Metadata mirroring failed on server: " + StringTk::uintToStr(nodeID) + "; "
         "Error: " + FhgfsOpsErrTk::toErrString(remoteRes) + "; "
         "NumTasks: " + StringTk::uintToStr(taskList->size() ) );
   }
   else
   { // mirroring successful
      LOG_DEBUG(logContext, Log_DEBUG,
         "Metadata mirroring successful on server: " + StringTk::uintToStr(nodeID) + "; "
         "NumTasks: " + StringTk::uintToStr(taskList->size() ) );
   }

   return remoteRes;
}



