#ifndef CLOSELOCALFILERESPMSG_H_
#define CLOSELOCALFILERESPMSG_H_

#include <common/net/message/NetMessage.h>

/**
 * This message implementation is outdated!!
 */

//struct CloseLocalFileRespMsg;
//typedef struct CloseLocalFileRespMsg CloseLocalFileRespMsg;
//
//static inline void CloseLocalFileRespMsg_init(CloseLocalFileRespMsg* this);
//static inline void CloseLocalFileRespMsg_initFromStorageVersion(CloseLocalFileRespMsg* this,
//   int result, int64_t filesize, uint64_t storageVersion);
//static inline CloseLocalFileRespMsg* CloseLocalFileRespMsg_construct(void);
//static inline CloseLocalFileRespMsg* CloseLocalFileRespMsg_constructFromStorageVersion(
//   int result, int64_t filesize, uint64_t storageVersion);
//static inline void CloseLocalFileRespMsg_uninit(NetMessage* this);
//static inline void CloseLocalFileRespMsg_destruct(NetMessage* this);
//
//// virtual functions
//extern void CloseLocalFileRespMsg_serializePayload(NetMessage* this, char* buf);
//extern fhgfs_bool CloseLocalFileRespMsg_deserializePayload(NetMessage* this, const char* buf,
//   size_t bufLen);
//extern unsigned CloseLocalFileRespMsg_calcMessageLength(NetMessage* this);
//
//// getters & setters
//static inline int CloseLocalFileRespMsg_getResult(CloseLocalFileRespMsg* this);
//static inline int64_t CloseLocalFileRespMsg_getFilesize(CloseLocalFileRespMsg* this);
//static inline uint64_t CloseLocalFileRespMsg_getStorageVersion(
//   CloseLocalFileRespMsg* this);
//
//
//struct CloseLocalFileRespMsg
//{
//   NetMessage netMessage;
//
//   int result;
//   int64_t filesize;
//   uint64_t storageVersion;
//};
//
//
//void CloseLocalFileRespMsg_init(CloseLocalFileRespMsg* this)
//{
//   NetMessage_init( (NetMessage*)this, NETMSGTYPE_CloseChunkFileResp);
//
//   // assign virtual functions
//   ( (NetMessage*)this)->uninit = CloseLocalFileRespMsg_uninit;
//
//   ( (NetMessage*)this)->serializePayload = CloseLocalFileRespMsg_serializePayload;
//   ( (NetMessage*)this)->deserializePayload = CloseLocalFileRespMsg_deserializePayload;
//   ( (NetMessage*)this)->calcMessageLength = CloseLocalFileRespMsg_calcMessageLength;
//}
//
//void CloseLocalFileRespMsg_initFromStorageVersion(CloseLocalFileRespMsg* this, int result,
//   int64_t filesize, uint64_t storageVersion)
//{
//   CloseLocalFileRespMsg_init(this);
//
//   this->result = result;
//   this->filesize = filesize;
//   this->storageVersion = storageVersion;
//}
//
//CloseLocalFileRespMsg* CloseLocalFileRespMsg_construct()
//{
//   struct CloseLocalFileRespMsg* this = os_kmalloc(sizeof(struct CloseLocalFileRespMsg) );
//
//   CloseLocalFileRespMsg_init(this);
//
//   return this;
//}
//
//CloseLocalFileRespMsg* CloseLocalFileRespMsg_constructFromStorageVersion(
//   int result, int64_t filesize, uint64_t storageVersion)
//{
//   struct CloseLocalFileRespMsg* this = os_kmalloc(sizeof(struct CloseLocalFileRespMsg) );
//
//   CloseLocalFileRespMsg_initFromStorageVersion(this, result, filesize, storageVersion);
//
//   return this;
//}
//
//void CloseLocalFileRespMsg_uninit(NetMessage* this)
//{
//   NetMessage_uninit(this);
//}
//
//void CloseLocalFileRespMsg_destruct(NetMessage* this)
//{
//   CloseLocalFileRespMsg_uninit(this);
//
//   os_kfree(this);
//}
//
//
//int CloseLocalFileRespMsg_getResult(CloseLocalFileRespMsg* this)
//{
//   return this->result;
//}
//
//int64_t CloseLocalFileRespMsg_getFilesize(CloseLocalFileRespMsg* this)
//{
//   return this->filesize;
//}
//
//uint64_t CloseLocalFileRespMsg_getStorageVersion(CloseLocalFileRespMsg* this)
//{
//   return this->storageVersion;
//}

#endif /*CLOSELOCALFILERESPMSG_H_*/
