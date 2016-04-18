#include <common/net/message/storage/GetStorageTargetInfoRespMsg.h>
#include <common/toolkit/MessagingTk.h>
#include <program/Program.h>
#include "GetStorageTargetInfoMsgEx.h"


bool GetStorageTargetInfoMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   const char* logContext("GetStorageTargetInfoMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername(); 
   LOG_DEBUG(logContext, Log_DEBUG, "Received a GetStorageTargetInfoMsg from: " + peer);

   App* app = Program::getApp();

   UInt16List targetIDs;
   StorageTargetInfoList targetInfoList;

   std::string targetPathStr;
   int64_t sizeTotal = 0;
   int64_t sizeFree = 0;
   int64_t inodesTotal = 0;
   int64_t inodesFree = 0;

   parseTargetIDs(&targetIDs);

   // (note: empty targetIDs means "get all targetIDs")
   if(!targetIDs.empty() && (*targetIDs.begin() != app->getLocalNodeNumID() ) )
   { // invalid nodeID given
      LogContext(logContext).logErr(
         "Unknown targetID: " + StringTk::uintToStr(*targetIDs.begin() ) );

      // Report default initialized target state (OFFLINE/GOOD).
   }
   else
   {
      targetPathStr = app->getMetaPath();

      getStatInfo(&sizeTotal, &sizeFree, &inodesTotal, &inodesFree);

      // Note: ConsistencyState will always be GOOD (since we have no buddy mirroring yet).
   }

   StorageTargetInfo targetInfo(app->getLocalNodeNumID(), targetPathStr, sizeTotal, sizeFree,
      inodesTotal, inodesFree, TargetConsistencyState_GOOD);
   targetInfoList.push_back(targetInfo);


   // send response

   GetStorageTargetInfoRespMsg respMsg(&targetInfoList);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0,
      (struct sockaddr*)fromAddr, sizeof(struct sockaddr_in) );


   // update stats

   app->getNodeOpStats()->updateNodeOp(sock->getPeerIP(), MetaOpCounter_STATFS,
      getMsgHeaderUserID() );

   return true;      
}

void GetStorageTargetInfoMsgEx::getStatInfo(int64_t* outSizeTotal, int64_t* outSizeFree,
   int64_t* outInodesTotal, int64_t* outInodesFree)
{
   const char* logContext = "GetStorageTargetInfoMsg (stat path)";

   std::string targetPathStr = Program::getApp()->getMetaPath();

   bool statSuccess = StorageTk::statStoragePath(targetPathStr, outSizeTotal, outSizeFree,
      outInodesTotal, outInodesFree);

   if(unlikely(!statSuccess) )
   { // error
      LogContext(logContext).logErr("Unable to statfs() storage path: " + targetPathStr +
         " (SysErr: " + System::getErrString() );

      *outSizeTotal = -1;
      *outSizeFree = -1;
      *outInodesTotal = -1;
      *outInodesFree = -1;
   }

   // read and use value from manual free space override file (if it exists)
   StorageTk::statStoragePathOverride(targetPathStr, outSizeFree, outInodesFree);
}
