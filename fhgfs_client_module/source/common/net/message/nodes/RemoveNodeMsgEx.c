#include <common/nodes/Node.h>
#include <components/DatagramListener.h>
#include <nodes/NodeStoreEx.h>
#include <common/toolkit/SocketTk.h>
#include <common/net/msghelpers/MsgHelperAck.h>
#include <app/App.h>
#include "RemoveNodeRespMsg.h"
#include "RemoveNodeMsgEx.h"


void RemoveNodeMsgEx_serializePayload(NetMessage* this, char* buf)
{
   RemoveNodeMsgEx* thisCast = (RemoveNodeMsgEx*)this;

   size_t bufPos = 0;

   // nodeType
   bufPos += Serialization_serializeShort(&buf[bufPos], thisCast->nodeType);

   // nodeNumID
   bufPos += Serialization_serializeUShort(&buf[bufPos], thisCast->nodeNumID);

   // nodeID
   bufPos += Serialization_serializeStr(&buf[bufPos], thisCast->nodeIDLen, thisCast->nodeID);

   // ackID
   bufPos += Serialization_serializeStr(&buf[bufPos], thisCast->ackIDLen, thisCast->ackID);
}

fhgfs_bool RemoveNodeMsgEx_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   RemoveNodeMsgEx* thisCast = (RemoveNodeMsgEx*)this;

   size_t bufPos = 0;

   unsigned nodeTypeBufLen;
   unsigned nodeNumIDBufLen;
   unsigned nodeBufLen;
   unsigned ackBufLen;

   // nodeType

   if(!Serialization_deserializeShort(&buf[bufPos], bufLen-bufPos,
      &thisCast->nodeType, &nodeTypeBufLen) )
      return fhgfs_false;

   bufPos += nodeTypeBufLen;

   // nodeNumID

   if(!Serialization_deserializeUShort(&buf[bufPos], bufLen-bufPos,
      &thisCast->nodeNumID, &nodeNumIDBufLen) )
      return fhgfs_false;

   bufPos += nodeTypeBufLen;

   // nodeID

   if(!Serialization_deserializeStr(&buf[bufPos], bufLen-bufPos,
      &thisCast->nodeIDLen, &thisCast->nodeID, &nodeBufLen) )
      return fhgfs_false;

   bufPos += nodeBufLen;

   // ackID

   if(!Serialization_deserializeStr(&buf[bufPos], bufLen-bufPos,
      &thisCast->ackIDLen, &thisCast->ackID, &ackBufLen) )
      return fhgfs_false;

   bufPos += ackBufLen;
   
   return fhgfs_true;
}

unsigned RemoveNodeMsgEx_calcMessageLength(NetMessage* this)
{
   RemoveNodeMsgEx* thisCast = (RemoveNodeMsgEx*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenShort() + // nodeType
      Serialization_serialLenUShort() + // nodeNumID
      Serialization_serialLenStr(thisCast->nodeIDLen) +
      Serialization_serialLenStr(thisCast->ackIDLen);
}

fhgfs_bool __RemoveNodeMsgEx_processIncoming(NetMessage* this, struct App* app,
   fhgfs_sockaddr_in* fromAddr, struct Socket* sock, char* respBuf, size_t bufLen)
{
   Logger* log = App_getLogger(app);
   const char* logContext = "RemoveNodeMsg incoming";

   RemoveNodeMsgEx* thisCast = (RemoveNodeMsgEx*)this;

   const char* peer;
   NodeType nodeType = (NodeType)RemoveNodeMsgEx_getNodeType(thisCast);
   uint16_t nodeID = RemoveNodeMsgEx_getNodeNumID(thisCast);
   
   RemoveNodeRespMsg respMsg;
   unsigned respLen;
   fhgfs_bool serializeRes;
   ssize_t sendRes;

   peer = fromAddr ?
      SocketTk_ipaddrToStr(&fromAddr->addr) : StringTk_strDup(Socket_getPeername(sock) );
   LOG_DEBUG_FORMATTED(log, Log_DEBUG, logContext,
      "Received a RemoveNodeMsg from: %s; Node: %s %hu",
      peer, Node_nodeTypeToStr(nodeType), nodeID);
   os_kfree(peer);

   
   if(nodeType == NODETYPE_Meta)
   {
      NodeStoreEx* nodes = App_getMetaNodes(app);
      if(NodeStoreEx_deleteNode(nodes, nodeID) )
         Logger_logFormatted(log, Log_WARNING, logContext, "Removed metadata node: %hu", nodeID);
   }
   else
   if(nodeType == NODETYPE_Storage)
   {
      NodeStoreEx* nodes = App_getStorageNodes(app);
      if(NodeStoreEx_deleteNode(nodes, nodeID) )
         Logger_logFormatted(log, Log_WARNING, logContext, "Removed storage node: %hu", nodeID);
   }
   else
   { // should never happen
      Logger_logFormatted(log, Log_WARNING, logContext,
         "Received removal request for invalid node type: %d (%s)",
         (int)nodeType, Node_nodeTypeToStr(nodeType) );
   }
   
   
   // send response
   if(!MsgHelperAck_respondToAckRequest(app, RemoveNodeMsgEx_getAckID(thisCast),
      fromAddr, sock, respBuf, bufLen) )
   {
      RemoveNodeRespMsg_initFromValue(&respMsg, 0);
      
      respLen = NetMessage_getMsgLength( (NetMessage*)&respMsg);
      serializeRes = NetMessage_serialize( (NetMessage*)&respMsg, respBuf, bufLen);
      if(unlikely(!serializeRes) )
      {
         Logger_logErrFormatted(log, logContext, "Unable to serialize response");
         goto err_resp_uninit;
      }
      
      if(fromAddr)
      { // datagram => sync via dgramLis send method
         DatagramListener* dgramLis = App_getDatagramListener(app);
         sendRes = DatagramListener_sendto(dgramLis, respBuf, respLen, 0, fromAddr);
      }
      else
         sendRes = sock->sendto(sock, respBuf, respLen, 0, NULL);
      
      if(unlikely(sendRes <= 0) )
         Logger_logErrFormatted(log, logContext, "Send error. ErrCode: %lld", (long long)sendRes);
   
   err_resp_uninit:   
      RemoveNodeRespMsg_uninit( (NetMessage*)&respMsg);
   }

   return fhgfs_true;
}

