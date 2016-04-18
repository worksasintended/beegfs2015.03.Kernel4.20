#include <common/net/msghelpers/MsgHelperAck.h>
#include <common/nodes/MirrorBuddyGroupMapper.h>
#include <app/App.h>

#include "SetMirrorBuddyGroupMsgEx.h"

fhgfs_bool SetMirrorBuddyGroupMsgEx_deserializePayload(NetMessage* this, const char* buf,
   size_t bufLen)
{
   SetMirrorBuddyGroupMsgEx* thisCast = (SetMirrorBuddyGroupMsgEx*)this;

   size_t bufPos = 0;

   // primaryTargetID
   {
      unsigned primaryTargetIDBufLen;

      if (!Serialization_deserializeUShort(&buf[bufPos], bufLen-bufPos, &thisCast->primaryTargetID,
          &primaryTargetIDBufLen) )
         return fhgfs_false;

      bufPos += primaryTargetIDBufLen;
   }


   // secondaryTargetID
   {
      unsigned secondaryTargetIDBufLen;

      if (!Serialization_deserializeUShort(&buf[bufPos], bufLen-bufPos, &thisCast->secondaryTargetID,
          &secondaryTargetIDBufLen) )
         return fhgfs_false;

      bufPos += secondaryTargetIDBufLen;
   }


   // buddyGroupID
   {
      unsigned buddyGroupIDBufLen;

      if (!Serialization_deserializeUShort(&buf[bufPos], bufLen-bufPos, &thisCast->buddyGroupID,
          &buddyGroupIDBufLen) )
         return fhgfs_false;

      bufPos += buddyGroupIDBufLen;
   }


   // allowUpdate
   {
      unsigned allowUpdateBufLen;

      if (!Serialization_deserializeBool(&buf[bufPos], bufLen-bufPos, &thisCast->allowUpdate,
          &allowUpdateBufLen) )
         return fhgfs_false;

      bufPos += allowUpdateBufLen;
   }


   // ackID
   {
      unsigned ackBufLen;

      if (!Serialization_deserializeStr(&buf[bufPos], bufLen-bufPos, &thisCast->ackIDLen,
          &thisCast->ackID, &ackBufLen) )
         return fhgfs_false;

      bufPos += ackBufLen;
   }


   return fhgfs_true;
}

unsigned SetMirrorBuddyGroupMsgEx_calcMessageLength(NetMessage* this)
{
   SetMirrorBuddyGroupMsgEx* thisCast = (SetMirrorBuddyGroupMsgEx*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenUShort() + // primaryTargetID
      Serialization_serialLenUShort() + // secondaryTargetID
      Serialization_serialLenUShort() + // buddyGroupID
      Serialization_serialLenBool() + // allowUpdate
      Serialization_serialLenStr(thisCast->ackIDLen);
}

fhgfs_bool __SetMirrorBuddyGroupMsgEx_processIncoming(NetMessage* this, struct App* app,
   fhgfs_sockaddr_in* fromAddr, struct Socket* sock, char* respBuf, size_t bufLen)
{
   Logger* log = App_getLogger(app);
   MirrorBuddyGroupMapper* mirrorBuddyGroupMapper = App_getMirrorBuddyGroupMapper(app);
   TargetMapper* targetMapper = App_getTargetMapper(app);
   const char* logContext = "SetMirrorBuddyGroupMsg Incoming";
   FhgfsOpsErr addGroupResult;

   SetMirrorBuddyGroupMsgEx* thisCast = (SetMirrorBuddyGroupMsgEx*)this;

   const char* peer;

   uint16_t primaryTargetID = SetMirrorBuddyGroupMsgEx_getPrimaryTargetID(thisCast);
   uint16_t secondaryTargetID = SetMirrorBuddyGroupMsgEx_getSecondaryTargetID(thisCast);
   uint16_t buddyGroupID = SetMirrorBuddyGroupMsgEx_getBuddyGroupID(thisCast);
   fhgfs_bool allowUpdate = SetMirrorBuddyGroupMsgEx_getAllowUpdate(thisCast);

   addGroupResult = MirrorBuddyGroupMapper_addGroup(mirrorBuddyGroupMapper, targetMapper,
      buddyGroupID, primaryTargetID, secondaryTargetID, allowUpdate);

   if (addGroupResult == FhgfsOpsErr_SUCCESS)
      Logger_logFormatted(log, Log_NOTICE, logContext, "Added mirror group: Group ID: %hu; "
         "Primary target ID: %hu; Secondary target ID: %hu.", buddyGroupID, primaryTargetID,
         secondaryTargetID);
   else
      Logger_logFormatted(log, Log_WARNING, logContext, "Error adding mirror buddy group: Group ID:"
         " %hu; Primary target ID: %hu; Secondary target ID: %hu; Error: %s",
         FhgfsOpsErr_toErrString(addGroupResult) );

   peer = fromAddr ?
      SocketTk_ipaddrToStr(&fromAddr->addr) : StringTk_strDup(Socket_getPeername(sock) );

   // send Ack
   MsgHelperAck_respondToAckRequest(app, SetMirrorBuddyGroupMsgEx_getAckID(thisCast), fromAddr,
      sock, respBuf, bufLen);

   return fhgfs_true;
}
