#include "MkDirMsg.h"


void MkDirMsg_serializePayload(NetMessage* this, char* buf)
{
   MkDirMsg* thisCast = (MkDirMsg*)this;

   size_t bufPos = 0;
   
   // userID
   
   bufPos += Serialization_serializeUInt(&buf[bufPos], thisCast->userID);

   // groupID
   
   bufPos += Serialization_serializeUInt(&buf[bufPos], thisCast->groupID);

   // mode
   
   bufPos += Serialization_serializeInt(&buf[bufPos], thisCast->mode);

   // parentInfo

   bufPos += EntryInfo_serialize(thisCast->parentInfo, &buf[bufPos]);

   // newDirName

   bufPos += Serialization_serializeStrAlign4(&buf[bufPos],
      thisCast->newDirNameLen, thisCast->newDirName);

   // preferredNodes
   
   bufPos += Serialization_serializeUInt16List(&buf[bufPos], thisCast->preferredNodes);
}

unsigned MkDirMsg_calcMessageLength(NetMessage* this)
{
   MkDirMsg* thisCast = (MkDirMsg*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenUInt() + // userID
      Serialization_serialLenUInt() + // groupID
      Serialization_serialLenInt() + // mode
      EntryInfo_serialLen(thisCast->parentInfo) + // parentEntryInfo
      Serialization_serialLenStrAlign4(thisCast->newDirNameLen) + // newDirName
      Serialization_serialLenUInt16List(thisCast->preferredNodes); // preferredNodes
}


