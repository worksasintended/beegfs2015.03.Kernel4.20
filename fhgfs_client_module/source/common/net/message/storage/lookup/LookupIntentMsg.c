#include "LookupIntentMsg.h"


void LookupIntentMsg_serializePayload(NetMessage* this, char* buf)
{
   LookupIntentMsg* thisCast = (LookupIntentMsg*)this;

   size_t bufPos = 0;

   // intentFlags
   bufPos += Serialization_serializeInt(&buf[bufPos], thisCast->intentFlags);

   // parentInfo
   bufPos += EntryInfo_serialize(thisCast->parentInfoPtr, &buf[bufPos]);

   // entryName
   bufPos += Serialization_serializeStrAlign4(&buf[bufPos], thisCast->entryNameLen,
      thisCast->entryName);

   if (thisCast->intentFlags & LOOKUPINTENTMSG_FLAG_REVALIDATE)
   {
      // entryInfo
      bufPos += EntryInfo_serialize(thisCast->entryInfoPtr, &buf[bufPos]);
   }

   if(thisCast->intentFlags & LOOKUPINTENTMSG_FLAG_OPEN)
   {
      // accessFlags
      bufPos += Serialization_serializeUInt(&buf[bufPos], thisCast->accessFlags);

      // sessionID
      bufPos += Serialization_serializeStrAlign4(&buf[bufPos], thisCast->sessionIDLen,
         thisCast->sessionID);
   }

   if(thisCast->intentFlags & LOOKUPINTENTMSG_FLAG_CREATE)
   {
      // userID
      bufPos += Serialization_serializeUInt(&buf[bufPos], thisCast->userID);

      // groupID
      bufPos += Serialization_serializeUInt(&buf[bufPos], thisCast->groupID);

      // mode
      bufPos += Serialization_serializeInt(&buf[bufPos], thisCast->mode);

      // umask
      if (NetMessage_isMsgHeaderFeatureFlagSet(this, LOOKUPINTENTMSG_FLAG_UMASK) )
         bufPos += Serialization_serializeInt(&buf[bufPos], thisCast->umask);

      // preferredTargets
      bufPos += Serialization_serializeUInt16List(&buf[bufPos], thisCast->preferredTargets);
   }

}

unsigned LookupIntentMsg_calcMessageLength(NetMessage* this)
{
   LookupIntentMsg* thisCast = (LookupIntentMsg*)this;

   unsigned msgLength = NETMSG_HEADER_LENGTH                     +
      Serialization_serialLenInt()                               +   // intentFlags
      EntryInfo_serialLen(thisCast->parentInfoPtr)               +   // parentInfo
      Serialization_serialLenStrAlign4(thisCast->entryNameLen);      // entryName

   if (thisCast->intentFlags & LOOKUPINTENTMSG_FLAG_REVALIDATE)
      msgLength += EntryInfo_serialLen(thisCast->entryInfoPtr);      // entryInfo

   if(thisCast->intentFlags & LOOKUPINTENTMSG_FLAG_OPEN)
   {
      msgLength += Serialization_serialLenUInt()                   + // accessFlags
         Serialization_serialLenStrAlign4(thisCast->sessionIDLen);   // sessionID
   }

   if(thisCast->intentFlags & LOOKUPINTENTMSG_FLAG_CREATE)
   {
      msgLength += Serialization_serialLenUInt() + // userID
         Serialization_serialLenUInt() + // groupID
         Serialization_serialLenInt(); // mode

      if (NetMessage_isMsgHeaderFeatureFlagSet(this, LOOKUPINTENTMSG_FLAG_UMASK) )
         msgLength += Serialization_serialLenInt(); // umask

      msgLength += Serialization_serialLenUInt16List(thisCast->preferredTargets); // preferredTargets
   }

   return msgLength;
}


