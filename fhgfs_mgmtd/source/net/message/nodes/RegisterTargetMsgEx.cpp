#include <common/net/message/nodes/RegisterTargetRespMsg.h>
#include <common/net/msghelpers/MsgHelperAck.h>
#include <common/storage/StorageErrors.h>
#include <common/toolkit/MessagingTk.h>
#include <program/Program.h>
#include "RegisterTargetMsgEx.h"


bool RegisterTargetMsgEx::processIncoming(struct sockaddr_in* fromAddr, Socket* sock,
   char* respBuf, size_t bufLen, HighResolutionStats* stats)
{
   LogContext log("RegisterTargetMsg incoming");

   std::string peer = fromAddr ? Socket::ipaddrToStr(&fromAddr->sin_addr) : sock->getPeername();
   LOG_DEBUG_CONTEXT(log, Log_DEBUG, "Received a RegisterTargetMsg from: " + peer);

   App* app = Program::getApp();
   Config* cfg = app->getConfig();
   NumericIDMapper* targetNumIDMapper = app->getTargetNumIDMapper();

   std::string targetID = getTargetID();
   uint16_t targetNumID = getTargetNumID();

   uint16_t newTargetNumID;
   bool targetWasMapped;

   LOG_DEBUG_CONTEXT(log, Log_DEBUG,
      "Target: " + targetID + "; "
      "numID: " + StringTk::uintToStr(targetNumID) );

   if(!cfg->getSysAllowNewTargets() )
   { // no new targets allowed => check if target is new
      uint16_t existingNumID = targetNumIDMapper->getNumericID(targetID);
      if(!existingNumID)
      { // target is new => reject
         log.log(Log_WARNING, "Registration of new targets disabled. "
            "Rejecting storage target: " + targetID + " "
            "(from: " + peer + ")");

         newTargetNumID = 0;
         goto send_response;
      }
   }

   targetWasMapped = targetNumIDMapper->mapStringID(targetID, targetNumID, &newTargetNumID);

   if(!targetWasMapped)
   { // registration failed
      log.log(Log_CRITICAL, "Registration failed for target: " + targetID + "; "
         "numID: " + StringTk::uintToStr(targetNumID) );

      newTargetNumID = 0;
   }
   else
   { // registration successful
      if(!targetNumID) // new target registration
         log.log(Log_NOTICE, "New target registered: " + targetID + "; "
            "numID: " + StringTk::uintToStr(newTargetNumID) );
   }

   // send response
send_response:

   RegisterTargetRespMsg respMsg(newTargetNumID);
   respMsg.serialize(respBuf, bufLen);
   sock->sendto(respBuf, respMsg.getMsgLength(), 0, NULL, 0);

   return true;
}

