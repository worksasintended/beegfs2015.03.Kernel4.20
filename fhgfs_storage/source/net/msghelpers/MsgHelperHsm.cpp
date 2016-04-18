#include "MsgHelperHsm.h"

#include <program/Program.h>

#include <common/net/message/gam/GamArchiveFilesMsg.h>
#include <common/toolkit/SessionTk.h>

bool MsgHelperHsm::sendArchiveFilesMsg(uint16_t targetID, std::string entryID,
   uint16_t collocationID)
{
   const char* logContext = "MsgHelperHsm (notifyGam)";

   bool retVal = true;

   NodeStoreServersEx* hsmNodes = Program::getApp()->getHsmNodes();

   uint16_t hsmNodeID = targetID;
   Node* hsmNode = hsmNodes->referenceNode(hsmNodeID);

   if(!hsmNode)
   {
      LogContext(logContext).logErr("HSM node does not exist: " + StringTk::uintToStr(hsmNodeID));
      return false;
   }

   StringVector chunkIDs;
   UShortVector collocationIDs;

   chunkIDs.push_back(entryID);
   collocationIDs.push_back(collocationID);

   GamArchiveFilesMsg gamArchiveFilesMsg(&chunkIDs, &collocationIDs);

   NodeConnPool* connPool = hsmNode->getConnPool();

   Socket* hsmSock = NULL;
   char* sendBuf = NULL;

   // connect
   try
   {
      hsmSock = connPool->acquireStreamSocket();

      if(hsmSock)
      {
         // prepare sendBuf
         size_t sendBufLen = gamArchiveFilesMsg.getMsgLength();
         sendBuf = (char*) malloc(sendBufLen);

         bool serialRes = gamArchiveFilesMsg.serialize(sendBuf, sendBufLen);

         if(serialRes)
         {
            // send request to gam node
            hsmSock->send(sendBuf, sendBufLen, 0);
         }
         else
         {
            LogContext(logContext).logErr("Unable to serialize archive files message");
            retVal = false;
         }

         connPool->releaseStreamSocket(hsmSock);

         SAFE_FREE(sendBuf);
      }
      else
      {
         LogContext(logContext).logErr(
            "Unable to connect to HSM node: " + StringTk::uintToStr(hsmNodeID));
         retVal = false;
      }
   }
   catch (SocketException& e)
   {
      if(hsmSock)
         connPool->invalidateStreamSocket(hsmSock);

      LogContext(logContext).logErr(
         "Unable to communicate with HSM node: " + StringTk::uintToStr(hsmNodeID) + "; "
            + e.what());
      retVal = false;
   }

   hsmNodes->releaseNode(&hsmNode);

   return retVal;
}
