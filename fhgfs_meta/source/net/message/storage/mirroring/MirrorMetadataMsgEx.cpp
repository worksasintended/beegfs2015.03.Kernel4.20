#include <common/app/log/LogContext.h>
#include <common/net/message/storage/mirroring/MirrorMetadataRespMsg.h>
#include <common/storage/StorageErrors.h>
#include <program/Program.h>
#include "MirrorMetadataMsgEx.h"


void MirrorMetadataMsgEx::serializePayload(char* buf)
{
   size_t bufPos = 0;

   unsigned currentTaskListNumElems = 0; // just for sanity check

   // taskListNumElems
   bufPos += Serialization::serializeUInt(&buf[bufPos], taskListNumElems);

   // taskList elements
   for(MirrorerTaskListIter iter = taskList->begin(); iter != taskList->end(); iter++)
   {
      bufPos += (*iter)->serialize(&buf[bufPos] );
      currentTaskListNumElems++;
   }

#ifdef BEEGFS_DEBUG
   // sanity checks...

   if(unlikely(currentTaskListNumElems != taskListNumElems) )
      LogContext(__func__).logErr("Warning: currentTaskListNumElems != taskListNumElems. "
         "currentTaskListNumElems: " + StringTk::uintToStr(currentTaskListNumElems) + "; "
         "taskListNumElems: " + StringTk::uintToStr(taskListNumElems) );

   // (subtract taskListNumElems serial len from bufPos)
   if(unlikely( (bufPos - Serialization::serialLenUInt() ) != taskListSerialLen) )
      LogContext(__func__).logErr("Warning: taskListSerialLen does not match expected bufPos. "
         "bufPos: " + StringTk::uintToStr(bufPos) + "; "
         "taskListSerialLen: " + StringTk::uintToStr(taskListSerialLen) );
#endif // BEEGFS_DEBUG
}

bool MirrorMetadataMsgEx::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   unsigned currentTaskListNumElems = 0; // just for sanity check

   { // deserTaskListNumElems
      unsigned fieldBufLen;

      if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
         &deserTaskListNumElems, &fieldBufLen) )
         return false;

      bufPos += fieldBufLen;
   }

   // taskList elements
   for(unsigned i=0; i < deserTaskListNumElems; i++)
   {
      unsigned taskBufLen;

      // add new task element to list
      deserTaskList.push_back(new MirrorerTask() );

      // deserialize task element in list
      bool deserTaskRes = deserTaskList.back()->deserialize(
         &buf[bufPos], bufLen-bufPos, &taskBufLen);

      if(unlikely(!deserTaskRes) )
      { // deserialization failed
         delete(deserTaskList.back() );
         deserTaskList.pop_back();
         return false;
      }

      bufPos += taskBufLen;

      currentTaskListNumElems++;
   }

   return true;
}

bool MirrorMetadataMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   #ifdef BEEGFS_DEBUG
      const char* logContext = "MirrorMetadataMsg incoming";

      std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
      LOG_DEBUG(logContext, Log_DEBUG, std::string("Received a MirrorMetadataMsg from: ") + peer);
   #endif // BEEGFS_DEBUG

   MirrorerTaskList* taskList = getTaskList();
   unsigned taskListNumElems = getTaskListNumElems();

   LOG_DEBUG(logContext, Log_DEBUG, "NumTasks: " + StringTk::uintToStr(taskListNumElems) );
   IGNORE_UNUSED_VARIABLE(taskListNumElems);

   FhgfsOpsErr mirrorRes = FhgfsOpsErr_SUCCESS;

   // walk all tasks and call their executeTask() method
   for(MirrorerTaskListIter iter = taskList->begin(); iter != taskList->end(); iter++)
   {
      FhgfsOpsErr taskRes = (*iter)->executeTask();

      // on error, use the first error from the task batch as response result
      if(unlikely(taskRes != FhgfsOpsErr_SUCCESS) )
         if(mirrorRes == FhgfsOpsErr_SUCCESS)
            mirrorRes = taskRes;
   }

   if(mirrorRes != FhgfsOpsErr_SUCCESS)
      LOG_DEBUG(logContext, Log_DEBUG,
         std::string("Error result: ") + FhgfsOpsErrTk::toErrString(mirrorRes) );

   MirrorMetadataRespMsg respMsg(mirrorRes);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );

   App* app = Program::getApp();
   app->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_MIRRORMETADATA,
      getMsgHeaderUserID() );

   return true;
}
