#include <common/app/log/LogContext.h>
#include <common/app/AbstractApp.h>
#include <common/threading/PThread.h>
#include <common/net/message/NetMessage.h>
#include <common/toolkit/MessagingTk.h>
#include <common/toolkit/VersionTk.h>
#include <common/nodes/NodeStoreServers.h>
#include <common/storage/StorageErrors.h>
#include <common/net/message/session/rw/ReadLocalFileV2Msg.h>
#include "ReadLocalFileV2Work.h"


void ReadLocalFileV2Work::process(
   char* bufIn, unsigned bufInLen, char* bufOut, unsigned bufOutLen)
{
   TargetMapper* targetMapper = readInfo->targetMapper;
   NodeStoreServers* nodes = readInfo->storageNodes;
   FhgfsOpsErr resolveErr;
   
   Node* node = nodes->referenceNodeByTargetID(targetID, targetMapper, &resolveErr);
   if(unlikely(!node) )
   { // unable to resolve targetID
      *nodeResult = -resolveErr;
   }
   else
   { // got node reference => begin communication
      *nodeResult = communicate(node, bufIn, bufInLen, bufOut, bufOutLen);
      nodes->releaseNode(&node);
   }
   
   readInfo->counter->incCount();
}

int64_t ReadLocalFileV2Work::communicate(Node* node, char* bufIn, unsigned bufInLen,
   char* bufOut, unsigned bufOutLen)
{
   const char* logContext = "ReadFileV2 Work (communication)";
   NodeConnPool* connPool = node->getConnPool();
   const char* localNodeID = readInfo->localNodeID;

   int64_t retVal = -FhgfsOpsErr_COMMUNICATION;
   int64_t numReceivedFileBytes = 0;
   Socket* sock = NULL;
   
   try
   {
      // connect
      sock = connPool->acquireStreamSocket();

      // prepare and send message
      ReadLocalFileV2Msg readMsg(localNodeID, fileHandleID, targetID, pathInfoPtr,
         accessFlags, offset, size);

      if (this->firstWriteDoneForTarget)
         readMsg.addMsgHeaderFeatureFlag(READLOCALFILEMSG_FLAG_SESSION_CHECK);

      readMsg.serialize(bufOut, bufOutLen);
      sock->send(bufOut, readMsg.getMsgLength(), 0);
      
      // recv length info and file data loop
      // note: we return directly from within this loop if the received header indicates an end of
      //    transmission
      for( ; ; )
      {
         char dataLenBuf[sizeof(int64_t)]; // length info in fhgfs network byte order
         int64_t lengthInfo; // length info in fhgfs host byte order
         unsigned lengthInfoBufLen; // deserialization buf length for lengthInfo (ignored)

         sock->recvExactT(&dataLenBuf, sizeof(int64_t), 0, CONN_LONG_TIMEOUT);

         // got the length info response
         Serialization::deserializeInt64(
            dataLenBuf, sizeof(int64_t), &lengthInfo, &lengthInfoBufLen);

         if(lengthInfo <= 0)
         { // end of file data transmission
            if(unlikely(lengthInfo < 0) )
            { // error occurred
               retVal = lengthInfo;
            }
            else
            { // normal end of file data transmission
               retVal = numReceivedFileBytes;
            }

            connPool->releaseStreamSocket(sock);
            return retVal;
         }

         // buffer overflow check
         if(unlikely( (uint64_t)(numReceivedFileBytes + lengthInfo) > this->size) )
         {
            LogContext(logContext).logErr(
               std::string("Received a lengthInfo that would lead to a buffer overflow from ") +
                  node->getID() + ": " + StringTk::int64ToStr(lengthInfo) );

            retVal = -FhgfsOpsErr_INTERNAL;

            break;
         }

         // positive result => node is going to send some file data

         // receive announced dataPart
         sock->recvExactT(&(this->buf)[numReceivedFileBytes], lengthInfo, 0, CONN_LONG_TIMEOUT);

         numReceivedFileBytes += lengthInfo;

      } // end of recv file data + header loop

   }
   catch(SocketConnectException& e)
   {
      LogContext(logContext).log(2, std::string("Unable to connect to storage server: ") +
         node->getID() );

      retVal = -FhgfsOpsErr_COMMUNICATION;
   }
   catch(SocketException& e)
   {
      LogContext(logContext).logErr(
         std::string("Communication error. SocketException: ") + e.what() );
      LogContext(logContext).log(2, std::string("Sent request: handleID/offset/size: ") +
         fileHandleID + "/" + StringTk::int64ToStr(offset) + "/" + StringTk::int64ToStr(size) );

      retVal = -FhgfsOpsErr_COMMUNICATION;
   }
   
   // error clean up

   if(sock)
      connPool->invalidateStreamSocket(sock);

   return retVal;
}
