#include "RmDirMsg.h"


void RmDirMsg_serializePayload(NetMessage* this, char* buf)
{
   RmDirMsg* thisCast = (RmDirMsg*)this;

   size_t bufPos = 0;
   
   // parentInfo
   bufPos += EntryInfo_serialize(thisCast->parentInfo, &buf[bufPos]);
   
   // delDirName
   bufPos += Serialization_serializeStrAlign4(&buf[bufPos],
      thisCast->delDirNameLen, thisCast->delDirName);
}


unsigned RmDirMsg_calcMessageLength(NetMessage* this)
{
   RmDirMsg* thisCast = (RmDirMsg*)this;

   return NETMSG_HEADER_LENGTH +
      EntryInfo_serialLen(thisCast->parentInfo)                   + // parentInfo
      Serialization_serialLenStrAlign4(thisCast->delDirNameLen);    // delDirName
}


