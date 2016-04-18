#include <common/net/message/nodes/MapTargetsRespMsg.h>
#include <common/net/msghelpers/MsgHelperAck.h>
#include <common/storage/StorageErrors.h>
#include <common/toolkit/MessagingTk.h>
#include <program/Program.h>
#include "MapTargetsMsgEx.h"


bool MapTargetsMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("MapTargetsMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG_CONTEXT(log, Log_DEBUG, std::string("Received a MapTargetsMsg from: ") + peer);

   App* app = Program::getApp();
   Config* cfg = app->getConfig();
   NodeStoreServers* storageNodes = app->getStorageNodes();
   TargetMapper* targetMapper = app->getTargetMapper();
   InternodeSyncer* syncer = app->getInternodeSyncer();

   uint16_t nodeID = getNodeID();
   UInt16List targetIDs;

   FhgfsOpsErr responseError = FhgfsOpsErr_SUCCESS;

   parseTargetIDs(&targetIDs);

   //LOG_DEBUG_CONTEXT(log, Log_WARNING, "Mapping " +
   //   StringTk::uintToStr(targetIDs.size() ) + " targets "
   //   "to node: " + storageNodes->getNodeIDWithTypeStr(nodeID) ); // debug in

   bool isNodeActive = storageNodes->isNodeActive(nodeID);
   if(unlikely(!isNodeActive) )
   { // santiy check: unknown nodeID here should never happen
      log.logErr("Refusing to map targets to unknown storage server ID: " +
         StringTk::uintToStr(nodeID) );

      responseError = FhgfsOpsErr_UNKNOWNNODE;
      goto send_response;
   }

   // walk over all targets and map them to the given nodeID

   for(UInt16ListConstIter iter = targetIDs.begin(); iter != targetIDs.end(); iter++)
   {
      if(unlikely(!*iter) )
      { // santiy check: undefined targetNumID here should never happen
         log.logErr("Refusing to map target with undefined numeric ID to "
            "node: " + StringTk::uint64ToStr(nodeID) );

         responseError = FhgfsOpsErr_UNKNOWNTARGET;
         continue;
      }

      if(!cfg->getSysAllowNewTargets() )
      { // no new targets allowed => check if target is new
         uint16_t existingNodeID = targetMapper->getNodeID(*iter);
         if(!existingNodeID)
         { // target is new => reject
            log.log(Log_WARNING, "Registration of new targets disabled. "
               "Rejecting storage target: " + StringTk::uintToStr(*iter) + " "
               "(from: " + peer + ")");

            responseError = FhgfsOpsErr_UNKNOWNTARGET;
            continue;
         }
      }


      bool wasNewTarget = targetMapper->mapTarget(*iter, nodeID);
      if(wasNewTarget)
      {
         LOG_DEBUG_CONTEXT(log, Log_WARNING, "Mapping "
            "target " + StringTk::uintToStr(*iter) +
            " => " +
            storageNodes->getNodeIDWithTypeStr(nodeID) );

         // async notification of other nodes (one target at a time to keep UDP msgs small)
         app->getHeartbeatMgr()->notifyAsyncAddedTarget(*iter, nodeID);
      }
   }

   // force update of capacity pools
   /* (because maybe server was restarted and thus might have been in the emergency pool while the
      target was unreachable) */
   syncer->setForcePoolsUpdate();

send_response:
   // send response

   if(!MsgHelperAck::respondToAckRequest(this, fromAddr, sock,
      respBuf, bufLen, app->getDatagramListener() ) )
   {
      MapTargetsRespMsg respMsg(responseError);
      respMsg.serialize(respBuf, bufLen);

      if(fromAddr)
      { // datagram => sync via dgramLis send method
         app->getDatagramListener()->sendto(respBuf, respMsg.getMsgLength(), 0,
            (struct sockaddr*)fromAddr, sizeof(*fromAddr) );
      }
      else
         sock->sendto(respBuf, respMsg.getMsgLength(), 0, NULL, 0);
   }

   return true;
}

