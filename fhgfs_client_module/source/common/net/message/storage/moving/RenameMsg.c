#include "RenameMsg.h"


void RenameMsg_serializePayload(NetMessage* this, char* buf)
{
   RenameMsg* thisCast = (RenameMsg*)this;

   size_t bufPos = 0;
   
   //entryType
   bufPos += Serialization_serializeUInt(&buf[bufPos], (unsigned) thisCast->entryType);

   // fromDirInfo
   bufPos += EntryInfo_serialize(thisCast->fromDirInfo, &buf[bufPos]);

   // toDirInfo
   bufPos += EntryInfo_serialize(thisCast->toDirInfo, &buf[bufPos]);
   
   // oldName
   bufPos += Serialization_serializeStrAlign4(&buf[bufPos],
      thisCast->oldNameLen, thisCast->oldName);

   // newName
   bufPos += Serialization_serializeStrAlign4(&buf[bufPos],
      thisCast->newNameLen, thisCast->newName);

}

unsigned RenameMsg_calcMessageLength(NetMessage* this)
{
   RenameMsg* thisCast = (RenameMsg*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenUInt()                           + // entryType
      EntryInfo_serialLen(thisCast->fromDirInfo)              + // fromDirInfo
      EntryInfo_serialLen(thisCast->toDirInfo)                + // toDirInfo
      Serialization_serialLenStrAlign4(thisCast->oldNameLen)  + // oldName
      Serialization_serialLenStrAlign4(thisCast->newNameLen);   // newName
}


