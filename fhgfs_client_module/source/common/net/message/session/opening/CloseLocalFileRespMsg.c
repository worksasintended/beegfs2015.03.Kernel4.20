#include "CloseLocalFileRespMsg.h"


//void CloseLocalFileRespMsg_serializePayload(NetMessage* this, char* buf)
//{
//   CloseLocalFileRespMsg* thisCast = (CloseLocalFileRespMsg*)this;
//
//   size_t bufPos = 0;
//
//   // result
//   bufPos += Serialization_serializeInt(&buf[bufPos], thisCast->result);
//
//   // filesize
//   bufPos += Serialization_serializeInt64(&buf[bufPos], thisCast->filesize);
//
//   // storageVersion
//   bufPos += Serialization_serializeUInt64(&buf[bufPos], thisCast->storageVersion);
//}
//
//fhgfs_bool CloseLocalFileRespMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
//{
//   CloseLocalFileRespMsg* thisCast = (CloseLocalFileRespMsg*)this;
//
//   size_t bufPos = 0;
//
//   unsigned resultFieldLen;
//   unsigned filesizeFieldLen;
//   unsigned versionFieldLen;
//
//   // result
//   if(!Serialization_deserializeInt(&buf[bufPos], bufLen-bufPos, &thisCast->result,
//      &resultFieldLen) )
//      return fhgfs_false;
//
//   bufPos += resultFieldLen;
//
//   // filesize
//   if(!Serialization_deserializeInt64(&buf[bufPos], bufLen-bufPos, &thisCast->filesize,
//      &filesizeFieldLen) )
//      return fhgfs_false;
//
//   bufPos += filesizeFieldLen;
//
//   // storageVersion
//   if(!Serialization_deserializeUInt64(&buf[bufPos], bufLen-bufPos,
//      &thisCast->storageVersion, &versionFieldLen) )
//      return fhgfs_false;
//
//   bufPos += versionFieldLen;
//
//   return fhgfs_true;
//}
//
//unsigned CloseLocalFileRespMsg_calcMessageLength(NetMessage* this)
//{
//   //CloseLocalFileRespMsg* thisCast = (CloseLocalFileRespMsg*)this;
//
//   return NETMSG_HEADER_LENGTH +
//      Serialization_serialLenInt() + // result
//      Serialization_serialLenInt64() + // filesize
//      Serialization_serialLenUInt64(); // storageVersion
//}


