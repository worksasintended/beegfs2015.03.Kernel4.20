#ifndef CLOSELOCALFILEMSG_H_
#define CLOSELOCALFILEMSG_H_

#include <common/net/message/NetMessage.h>

// O U T D A T E D
// O U T D A T E D
// O U T D A T E D
// (still based on FD, not fileHandleID)
// (MDS now sends CloseChunkFileMsg to storage targets)


//struct CloseLocalFileMsg;
//typedef struct CloseLocalFileMsg CloseLocalFileMsg;
//
//static inline void CloseLocalFileMsg_init(CloseLocalFileMsg* this);
//static inline void CloseLocalFileMsg_initFromSession(CloseLocalFileMsg* this,
//   const char* sessionID, int fd);
//static inline CloseLocalFileMsg* CloseLocalFileMsg_construct(void);
//static inline CloseLocalFileMsg* CloseLocalFileMsg_constructFromSession(
//   const char* sessionID, int fd);
//static inline void CloseLocalFileMsg_uninit(NetMessage* this);
//static inline void CloseLocalFileMsg_destruct(NetMessage* this);
//
//// virtual functions
//extern void CloseLocalFileMsg_serializePayload(NetMessage* this, char* buf);
//extern fhgfs_bool CloseLocalFileMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen);
//extern unsigned CloseLocalFileMsg_calcMessageLength(NetMessage* this);
//
//// getters & setters
//static inline const char* CloseLocalFileMsg_getSessionID(CloseLocalFileMsg* this);
//static inline int CloseLocalFileMsg_getFD(CloseLocalFileMsg* this);
//
//
//struct CloseLocalFileMsg
//{
//   NetMessage netMessage;
//
//   unsigned sessionIDLen;
//   const char* sessionID;
//   int fd;
//};
//
//
//void CloseLocalFileMsg_init(CloseLocalFileMsg* this)
//{
//   NetMessage_init( (NetMessage*)this, NETMSGTYPE_CloseChunkFile);
//
//   // assign virtual functions
//   ( (NetMessage*)this)->uninit = CloseLocalFileMsg_uninit;
//
//   ( (NetMessage*)this)->serializePayload = CloseLocalFileMsg_serializePayload;
//   ( (NetMessage*)this)->deserializePayload = CloseLocalFileMsg_deserializePayload;
//   ( (NetMessage*)this)->calcMessageLength = CloseLocalFileMsg_calcMessageLength;
//}
//
///**
// * @param sessionID just a reference, so do not free it as long as you use this object!
// */
//void CloseLocalFileMsg_initFromSession(CloseLocalFileMsg* this, const char* sessionID, int fd)
//{
//   CloseLocalFileMsg_init(this);
//
//   this->sessionID = sessionID;
//   this->sessionIDLen = os_strlen(sessionID);
//
//   this->fd = fd;
//}
//
//CloseLocalFileMsg* CloseLocalFileMsg_construct()
//{
//   struct CloseLocalFileMsg* this = os_kmalloc(sizeof(struct CloseLocalFileMsg) );
//
//   CloseLocalFileMsg_init(this);
//
//   return this;
//}
//
///**
// * @param sessionID just a reference, so do not free it as long as you use this object!
// */
//CloseLocalFileMsg* CloseLocalFileMsg_constructFromSession(const char* sessionID, int fd)
//{
//   struct CloseLocalFileMsg* this = os_kmalloc(sizeof(struct CloseLocalFileMsg) );
//
//   CloseLocalFileMsg_initFromSession(this, sessionID, fd);
//
//   return this;
//}
//
//void CloseLocalFileMsg_uninit(NetMessage* this)
//{
//   NetMessage_uninit(this);
//}
//
//void CloseLocalFileMsg_destruct(NetMessage* this)
//{
//   CloseLocalFileMsg_uninit(this);
//
//   os_kfree(this);
//}
//
//
//const char* CloseLocalFileMsg_getSessionID(CloseLocalFileMsg* this)
//{
//   return this->sessionID;
//}
//
//int CloseLocalFileMsg_getFD(CloseLocalFileMsg* this)
//{
//   return this->fd;
//}

#endif /*CLOSELOCALFILEMSG_H_*/
