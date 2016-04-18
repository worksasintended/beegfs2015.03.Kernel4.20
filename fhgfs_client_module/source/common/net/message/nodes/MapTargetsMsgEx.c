#include <app/App.h>
#include <common/nodes/Node.h>
#include <common/toolkit/SocketTk.h>
#include <common/net/msghelpers/MsgHelperAck.h>
#include <common/toolkit/ListTk.h>
#include <nodes/NodeStoreEx.h>
#include <app/config/Config.h>
#include "MapTargetsMsgEx.h"


fhgfs_bool MapTargetsMsgEx_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   MapTargetsMsgEx* thisCast = (MapTargetsMsgEx*)this;

   size_t bufPos = 0;

   unsigned nodeBufLen;
   unsigned ackBufLen;

   // targetIDs

   if(!Serialization_deserializeUInt16ListPreprocess(&buf[bufPos], bufLen-bufPos,
      &thisCast->targetIDsElemNum, &thisCast->targetIDsListStart, &thisCast->targetIDsBufLen) )
      return fhgfs_false;

   bufPos += thisCast->targetIDsBufLen;

   // nodeID

   if(!Serialization_deserializeUShort(&buf[bufPos], bufLen-bufPos,
      &thisCast->nodeID, &nodeBufLen) )
      return fhgfs_false;

   bufPos += nodeBufLen;

   // ackID

   if(!Serialization_deserializeStr(&buf[bufPos], bufLen-bufPos,
      &thisCast->ackIDLen, &thisCast->ackID, &ackBufLen) )
      return fhgfs_false;

   bufPos += ackBufLen;


   return fhgfs_true;
}

unsigned MapTargetsMsgEx_calcMessageLength(NetMessage* this)
{
   MapTargetsMsgEx* thisCast = (MapTargetsMsgEx*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenUInt16List(thisCast->targetIDs) +
      Serialization_serialLenUShort() + // nodeID
      Serialization_serialLenStr(thisCast->ackIDLen);
}

fhgfs_bool __MapTargetsMsgEx_processIncoming(NetMessage* this, struct App* app,
   fhgfs_sockaddr_in* fromAddr, struct Socket* sock, char* respBuf, size_t bufLen)
{
   Logger* log = App_getLogger(app);
   const char* logContext = "MapTargetsMsg incoming";

   MapTargetsMsgEx* thisCast = (MapTargetsMsgEx*)this;

   const char* peer;

   TargetMapper* targetMapper = App_getTargetMapper(app);
   UInt16List targetIDs;
   UInt16ListIter iter;
   uint16_t nodeID = MapTargetsMsgEx_getNodeID(thisCast);


   peer = fromAddr ?
      SocketTk_ipaddrToStr(&fromAddr->addr) : StringTk_strDup(Socket_getPeername(sock) );
   LOG_DEBUG_FORMATTED(log, 4, logContext, "Received a MapTargetsMsg from: %s", peer);
   os_kfree(peer);

   IGNORE_UNUSED_VARIABLE(log);
   IGNORE_UNUSED_VARIABLE(logContext);


   UInt16List_init(&targetIDs);

   MapTargetsMsgEx_parseTargetIDs(thisCast, &targetIDs);

   for(UInt16ListIter_init(&iter, &targetIDs);
       !UInt16ListIter_end(&iter);
       UInt16ListIter_next(&iter) )
   {
      uint16_t targetID = UInt16ListIter_value(&iter);

      fhgfs_bool wasNewTarget = TargetMapper_mapTarget(targetMapper, targetID, nodeID);
      if(wasNewTarget)
      {
         LOG_DEBUG_FORMATTED(log, Log_WARNING, logContext, "Mapping target %hu => node %hu",
            targetID, nodeID);
      }
   }

   // send ack
   MsgHelperAck_respondToAckRequest(app, MapTargetsMsgEx_getAckID(thisCast), fromAddr, sock,
      respBuf, bufLen);

   // clean-up
   UInt16List_uninit(&targetIDs);

   return fhgfs_true;
}

