#include "MkFileMsg.h"


void MkFileMsg_serializePayload(NetMessage* this, char* buf)
{
   MkFileMsg* thisCast = (MkFileMsg*)this;

   size_t bufPos = 0;
   
   // userID
   
   bufPos += Serialization_serializeUInt(&buf[bufPos], thisCast->userID);

   // groupID
   
   bufPos += Serialization_serializeUInt(&buf[bufPos], thisCast->groupID);

   // mode
   
   bufPos += Serialization_serializeInt(&buf[bufPos], thisCast->mode);

   // umask
   if (NetMessage_isMsgHeaderFeatureFlagSet(this, MKFILEMSG_FLAG_UMASK) )
      bufPos += Serialization_serializeInt(&buf[bufPos], thisCast->umask);

   // optional stripe hints
   if(NetMessage_isMsgHeaderFeatureFlagSet( (NetMessage*)this, MKFILEMSG_FLAG_STRIPEHINTS) )
   {
      // numtargets
      bufPos += Serialization_serializeUInt(&buf[bufPos], thisCast->numtargets);

      // chunksize
      bufPos += Serialization_serializeUInt(&buf[bufPos], thisCast->chunksize);
   }

   // parentInfoPtr

   bufPos += EntryInfo_serialize(thisCast->parentInfoPtr, &buf[bufPos]);

   // newFileName

   bufPos += Serialization_serializeStrAlign4(&buf[bufPos],
      thisCast->newFileNameLen, thisCast->newFileName);

   // preferredTargets
   
   bufPos += Serialization_serializeUInt16List(&buf[bufPos], thisCast->preferredTargets);
}


unsigned MkFileMsg_calcMessageLength(NetMessage* this)
{
   MkFileMsg* thisCast = (MkFileMsg*)this;

   unsigned stripeHintsLen = 0;
   unsigned umaskLen = 0;

   if(NetMessage_isMsgHeaderFeatureFlagSet( (NetMessage*)this, MKFILEMSG_FLAG_STRIPEHINTS) )
   { // optional stripe hints are given
      stripeHintsLen =
         Serialization_serialLenUInt() + // numtargets
         Serialization_serialLenUInt(); // chunksize
   }

   if (NetMessage_isMsgHeaderFeatureFlagSet(this, MKFILEMSG_FLAG_UMASK) )
      umaskLen = Serialization_serialLenInt(); // umask

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenUInt() + // userID
      Serialization_serialLenUInt() + // groupID
      Serialization_serialLenInt()  + // mode
      umaskLen                      + // optional umask
      stripeHintsLen                + // optional stripe hints
      EntryInfo_serialLen(thisCast->parentInfoPtr)                   +  // parentInfoPtr
      Serialization_serialLenStrAlign4(thisCast->newFileNameLen)     +  // newFileName
      Serialization_serialLenUInt16List(thisCast->preferredTargets);    // preferredTargets
}


