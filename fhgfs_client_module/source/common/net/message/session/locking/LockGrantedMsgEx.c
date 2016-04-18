#include <app/App.h>
#include <components/AckManager.h>
#include <common/toolkit/ackstore/AcknowledgmentStore.h>
#include <common/toolkit/SocketTk.h>
#include <common/net/msghelpers/MsgHelperAck.h>
#include "LockGrantedMsgEx.h"

/**
 * Serialization not implemented!
 */
void LockGrantedMsgEx_serializePayload(NetMessage* this, char* buf)
{
   BEEGFS_BUG_ON(1, "This method is not implemented and should never be called");
}

fhgfs_bool LockGrantedMsgEx_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   LockGrantedMsgEx* thisCast = (LockGrantedMsgEx*)this;

   size_t bufPos = 0;

   unsigned lockAckIDBufLen;
   unsigned ackIDBufLen;
   unsigned granterNodeIDBufLen;

   // lockAckID

   if(!Serialization_deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
      &thisCast->lockAckIDLen, &thisCast->lockAckID, &lockAckIDBufLen) )
      return fhgfs_false;

   bufPos += lockAckIDBufLen;

   // ackID

   if(!Serialization_deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
      &thisCast->ackIDLen, &thisCast->ackID, &ackIDBufLen) )
      return fhgfs_false;

   bufPos += ackIDBufLen;

   // granterNodeID

   if(!Serialization_deserializeUShort(&buf[bufPos], bufLen-bufPos,
      &thisCast->granterNodeID, &granterNodeIDBufLen) )
      return fhgfs_false;

   bufPos += granterNodeIDBufLen;

   return fhgfs_true;
}

unsigned LockGrantedMsgEx_calcMessageLength(NetMessage* this)
{
   LockGrantedMsgEx* thisCast = (LockGrantedMsgEx*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenStrAlign4(thisCast->lockAckIDLen) +
      Serialization_serialLenStrAlign4(thisCast->ackIDLen) +
      Serialization_serialLenUShort(); // granterNodeID
}

fhgfs_bool __LockGrantedMsgEx_processIncoming(NetMessage* this, struct App* app,
   fhgfs_sockaddr_in* fromAddr, struct Socket* sock, char* respBuf, size_t bufLen)
{
   const char* logContext = "LockGranted incoming";
   Logger* log = App_getLogger(app);
   AcknowledgmentStore* ackStore = App_getAckStore(app);
   AckManager* ackManager = App_getAckManager(app);

   LockGrantedMsgEx* thisCast = (LockGrantedMsgEx*)this;
   fhgfs_bool gotLockWaiter;

   #ifdef LOG_DEBUG_MESSAGES
      const char* peer;

      peer = fromAddr ?
         SocketTk_ipaddrToStr(&fromAddr->addr) : StringTk_strDup(Socket_getPeername(sock) );
      LOG_DEBUG_FORMATTED(log, Log_DEBUG, logContext, "Received a AckMsg from: %s", peer);

      os_kfree(peer);
   #endif // LOG_DEBUG_MESSAGES

   IGNORE_UNUSED_VARIABLE(log);
   IGNORE_UNUSED_VARIABLE(logContext);

   gotLockWaiter = AcknowledgmentStore_receivedAck(
      ackStore, LockGrantedMsgEx_getLockAckID(thisCast) );

   /* note: other than for standard acks, we send a ack repsonse to lock-grants only if someone was
      still registered to wait for the lock-grant. (we do this to make our handling of interrupts
      during lock waits easier, because we don't need lock canceling now.) */

   if(gotLockWaiter)
   { // lock waiter registered => send ack
      const char* ackID = LockGrantedMsgEx_getAckID(thisCast);
      uint16_t granterNodeID = LockGrantedMsgEx_getGranterNodeID(thisCast);

      MsgHelperAck_respondToAckRequest(app, LockGrantedMsgEx_getAckID(thisCast), fromAddr, sock,
         respBuf, bufLen);

      AckManager_addAckToQueue(ackManager, granterNodeID, ackID);
   }

   return fhgfs_true;
}

