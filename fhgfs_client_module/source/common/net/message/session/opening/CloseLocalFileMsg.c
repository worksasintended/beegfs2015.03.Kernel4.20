#include "CloseLocalFileMsg.h"

// O U T D A T E D
// O U T D A T E D
// O U T D A T E D
// (still based on FD, not fileHandleID)
// (MDS now sends CloseChunkFileMsg to storage targets)


//void CloseLocalFileMsg_serializePayload(NetMessage* this, char* buf)
//{
//   CloseLocalFileMsg* thisCast = (CloseLocalFileMsg*)this;
//
//   size_t bufPos = 0;
//
//   // sessionID
//
//   bufPos += Serialization_serializeStr(&buf[bufPos], thisCast->sessionIDLen, thisCast->sessionID);
//
//   // fd
//
//   bufPos += Serialization_serializeInt(&buf[bufPos], thisCast->fd);
//}
//
//fhgfs_bool CloseLocalFileMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
//{
//   CloseLocalFileMsg* thisCast = (CloseLocalFileMsg*)this;
//
//   size_t bufPos = 0;
//
//   unsigned strBufLen;
//   unsigned fdLen;
//
//   // sessionID
//
//   if(!Serialization_deserializeStr(&buf[bufPos], bufLen-bufPos,
//      &thisCast->sessionIDLen, &thisCast->sessionID, &strBufLen) )
//      return fhgfs_false;
//
//   bufPos += strBufLen;
//
//   // fd
//
//   if(!Serialization_deserializeInt(&buf[bufPos], bufLen-bufPos, &thisCast->fd, &fdLen) )
//      return fhgfs_false;
//
//   bufPos += fdLen;
//
//   return fhgfs_true;
//}
//
//unsigned CloseLocalFileMsg_calcMessageLength(NetMessage* this)
//{
//   CloseLocalFileMsg* thisCast = (CloseLocalFileMsg*)this;
//
//   return NETMSG_HEADER_LENGTH +
//      Serialization_serialLenStr(thisCast->sessionIDLen) +
//      Serialization_serialLenInt(); // fd
//}


