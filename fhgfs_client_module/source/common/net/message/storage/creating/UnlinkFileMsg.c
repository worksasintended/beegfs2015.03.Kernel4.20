#include "UnlinkFileMsg.h"


void UnlinkFileMsg_serializePayload(NetMessage* this, char* buf)
{
   UnlinkFileMsg* thisCast = (UnlinkFileMsg*)this;

   size_t bufPos = 0;
   
   // parentInf
   bufPos += EntryInfo_serialize(thisCast->parentInfoPtr, &buf[bufPos]);
   
   // delFileName
   bufPos += Serialization_serializeStrAlign4(&buf[bufPos],
      thisCast->delFileNameLen, thisCast->delFileName);
}


unsigned UnlinkFileMsg_calcMessageLength(NetMessage* this)
{
   UnlinkFileMsg* thisCast = (UnlinkFileMsg*)this;

   return NETMSG_HEADER_LENGTH                                   +
      EntryInfo_serialLen(thisCast->parentInfoPtr)               + // parentInfo
      Serialization_serialLenStrAlign4(thisCast->delFileNameLen);  // delFileName
}


