// control messages
#include <common/net/message/control/AckMsgEx.h>
#include <common/net/message/control/GenericResponseMsg.h>
// helperd messages
#include <common/net/message/helperd/GetHostByNameRespMsg.h>
#include <common/net/message/helperd/LogRespMsg.h>
// nodes messages
#include <common/net/message/nodes/GetNodesRespMsg.h>
#include <common/net/message/nodes/GetMirrorBuddyGroupsRespMsg.h>
#include <common/net/message/nodes/GetStatesAndBuddyGroupsRespMsg.h>
#include <common/net/message/nodes/GetTargetMappingsRespMsg.h>
#include <common/net/message/nodes/GetTargetStatesRespMsg.h>
#include <common/net/message/nodes/HeartbeatRequestMsgEx.h>
#include <common/net/message/nodes/HeartbeatMsgEx.h>
#include <common/net/message/nodes/MapTargetsMsgEx.h>
#include <common/net/message/nodes/RefreshTargetStatesMsgEx.h>
#include <common/net/message/nodes/RemoveNodeMsgEx.h>
#include <common/net/message/nodes/RemoveNodeRespMsg.h>
#include <common/net/message/nodes/SetMirrorBuddyGroupMsgEx.h>
// storage messages
#include <common/net/message/storage/creating/MkDirRespMsg.h>
#include <common/net/message/storage/creating/MkFileRespMsg.h>
#include <common/net/message/storage/creating/MkLocalFileRespMsg.h>
#include <common/net/message/storage/creating/RmDirRespMsg.h>
#include <common/net/message/storage/creating/HardlinkRespMsg.h>
#include <common/net/message/storage/creating/UnlinkFileRespMsg.h>
#include <common/net/message/storage/creating/UnlinkLocalFileRespMsg.h>
#include <common/net/message/storage/listing/ListDirFromOffsetRespMsg.h>
#include <common/net/message/storage/attribs/ListXAttrRespMsg.h>
#include <common/net/message/storage/attribs/GetXAttrRespMsg.h>
#include <common/net/message/storage/attribs/RemoveXAttrRespMsg.h>
#include <common/net/message/storage/attribs/SetXAttrRespMsg.h>
#include <common/net/message/storage/attribs/RefreshEntryInfoRespMsg.h>
#include <common/net/message/storage/moving/RenameRespMsg.h>
#include <common/net/message/storage/lookup/LookupIntentRespMsg.h>
#include <common/net/message/storage/attribs/SetAttrRespMsg.h>
#include <common/net/message/storage/attribs/StatRespMsg.h>
#include <common/net/message/storage/StatStoragePathRespMsg.h>
#include <common/net/message/storage/TruncFileRespMsg.h>
// session messages
#include <common/net/message/session/opening/OpenFileRespMsg.h>
#include <common/net/message/session/opening/CloseFileRespMsg.h>
#include <common/net/message/session/opening/CloseLocalFileRespMsg.h>
#include <common/net/message/session/rw/WriteLocalFileRespMsg.h>
#include <common/net/message/session/FSyncLocalFileRespMsg.h>
#include <common/net/message/session/locking/FLockAppendRespMsg.h>
#include <common/net/message/session/locking/FLockEntryRespMsg.h>
#include <common/net/message/session/locking/FLockRangeRespMsg.h>
#include <common/net/message/session/locking/LockGrantedMsgEx.h>

#include <common/net/message/SimpleMsg.h>
#include "NetMessageFactory.h"


/**
 * The standard way to create message objects from serialized message buffers.
 *
 * @return NetMessage which must be deleted by the caller
 * (msg->msgType is NETMSGTYPE_Invalid on error)
 */
NetMessage* NetMessageFactory_createFromBuf(App* app, char* recvBuf, size_t bufLen)
{
   NetMessageHeader header;
   NetMessage* msg;
   fhgfs_bool checkCompatRes;

   char* msgPayloadBuf;
   size_t msgPayloadBufLen;
   fhgfs_bool deserPayloadRes;

   // decode the message header

   __NetMessage_deserializeHeader(recvBuf, bufLen, &header);

   // create the message object for the given message type

   msg = NetMessageFactory_createFromMsgType(header.msgType);

   if(unlikely(NetMessage_getMsgType(msg) == NETMSGTYPE_Invalid) )
   {
      printk_fhgfs_debug(KERN_NOTICE,
         "Received an invalid or unhandled message. "
         "Message type (from raw header): %hu", header.msgType);

      return msg;
   }

   // apply message feature flags and check compatibility

   NetMessage_setMsgHeaderFeatureFlags(msg, header.msgFeatureFlags);

   checkCompatRes = NetMessage_checkHeaderFeatureFlagsCompat(msg);
   if(unlikely(!checkCompatRes) )
   { // incompatible feature flag was set => log error with msg type
      printk_fhgfs(KERN_NOTICE,
         "Received a message with incompatible feature flags. "
         "Message type: %hu; Flags (hex): %X",
         header.msgType, NetMessage_getMsgHeaderFeatureFlags(msg) );

      _NetMessage_setMsgType(msg, NETMSGTYPE_Invalid);
      return msg;
   }

   // apply message compat feature flags (can be ignored by receiver, e.g. if unknown)

   NetMessage_setMsgHeaderCompatFeatureFlags(msg, header.msgCompatFeatureFlags);

   // deserialize message payload

   msgPayloadBuf = recvBuf + NETMSG_HEADER_LENGTH;
   msgPayloadBufLen = bufLen - NETMSG_HEADER_LENGTH;

   deserPayloadRes = msg->deserializePayload(msg, msgPayloadBuf, msgPayloadBufLen);
   if(unlikely(!deserPayloadRes) )
   {
      printk_fhgfs_debug(KERN_NOTICE,
         "Failed to decode message. "
         "Message type: %hu", header.msgType);

      _NetMessage_setMsgType(msg, NETMSGTYPE_Invalid);
      return msg;
   }

   return msg;
}

/**
 * Special deserializer for pre-alloc'ed message objects.
 *
 * @param outMsg prealloc'ed msg of the expected type; must be initialized with the corresponding
 * deserialization-initializer; must later be uninited/destructed by the caller, no matter whether
 * this succeeds or not
 * @param expectedMsgType the type of the pre-alloc'ed message object
 * @return fhgfs_false on error (e.g. if real msgType does not match expectedMsgType)
 */
fhgfs_bool NetMessageFactory_deserializeFromBuf(App* app, char* recvBuf, size_t bufLen,
   NetMessage* outMsg, unsigned short expectedMsgType)
{
   NetMessageHeader header;

   fhgfs_bool checkCompatRes;
   char* msgPayloadBuf;
   size_t msgPayloadBufLen;
   fhgfs_bool deserPayloadRes;

   // decode the message header

   __NetMessage_deserializeHeader(recvBuf, bufLen, &header);

   // verify expected message type

   if(unlikely(header.msgType != expectedMsgType) )
      return fhgfs_false;

   // apply message feature flags and check compatibility

   NetMessage_setMsgHeaderFeatureFlags(outMsg, header.msgFeatureFlags);

   checkCompatRes = NetMessage_checkHeaderFeatureFlagsCompat(outMsg);
   if(unlikely(!checkCompatRes) )
   { // incompatible feature flag was set => log error with msg type
      printk_fhgfs(KERN_NOTICE,
         "Received a message with incompatible feature flags. "
         "Message type: %hu; Flags (hex): %X",
         header.msgType, NetMessage_getMsgHeaderFeatureFlags(outMsg) );

      _NetMessage_setMsgType(outMsg, NETMSGTYPE_Invalid);
      return fhgfs_false;
   }

   // apply message compat feature flags (can be ignored by receiver, e.g. if unknown)

   NetMessage_setMsgHeaderCompatFeatureFlags(outMsg, header.msgCompatFeatureFlags);

   // deserialize message payload

   msgPayloadBuf = recvBuf + NETMSG_HEADER_LENGTH;
   msgPayloadBufLen = bufLen - NETMSG_HEADER_LENGTH;

   deserPayloadRes = outMsg->deserializePayload(outMsg, msgPayloadBuf, msgPayloadBufLen);
   if(unlikely(!deserPayloadRes) )
   {
      printk_fhgfs_debug(KERN_NOTICE,
         "Failed to decode message. "
         "Message type: %hu", header.msgType);

      _NetMessage_setMsgType(outMsg, NETMSGTYPE_Invalid);
      return fhgfs_false;
   }


   return fhgfs_true;
}


/**
 * @return NetMessage that must be deleted by the caller
 * (msg->msgType is NETMSGTYPE_Invalid on error)
 */
NetMessage* NetMessageFactory_createFromMsgType(unsigned short msgType)
{
   NetMessage* msg;

   switch(msgType)
   {
      // control messages
      case NETMSGTYPE_Ack: { msg = (NetMessage*)AckMsgEx_construct(); } break;
      case NETMSGTYPE_GenericResponse: { msg = (NetMessage*)GenericResponseMsg_construct(); } break;
      // helperd messages
      case NETMSGTYPE_GetHostByNameResp: { msg = (NetMessage*)GetHostByNameRespMsg_construct(); } break;
      case NETMSGTYPE_LogResp: { msg = (NetMessage*)LogRespMsg_construct(); } break;
      // nodes messages
      case NETMSGTYPE_GetMirrorBuddyGroupsResp: { msg = (NetMessage*)GetMirrorBuddyGroupsRespMsg_construct(); } break;
      case NETMSGTYPE_GetNodesResp: { msg = (NetMessage*)GetNodesRespMsg_construct(); } break;
      case NETMSGTYPE_GetStatesAndBuddyGroupsResp: { msg = (NetMessage*)GetStatesAndBuddyGroupsRespMsg_construct(); } break;
      case NETMSGTYPE_GetTargetMappingsResp: { msg = (NetMessage*)GetTargetMappingsRespMsg_construct(); } break;
      case NETMSGTYPE_GetTargetStatesResp: { msg = (NetMessage*)GetTargetStatesRespMsg_construct(); } break;
      case NETMSGTYPE_HeartbeatRequest: { msg = (NetMessage*)HeartbeatRequestMsgEx_construct(); } break;
      case NETMSGTYPE_Heartbeat: { msg = (NetMessage*)HeartbeatMsgEx_construct(); } break;
      case NETMSGTYPE_MapTargets: { msg = (NetMessage*)MapTargetsMsgEx_construct(); } break;
      case NETMSGTYPE_RefreshTargetStates: { msg = (NetMessage*)RefreshTargetStatesMsgEx_construct(); } break;
      case NETMSGTYPE_RemoveNode: { msg = (NetMessage*)RemoveNodeMsgEx_construct(); } break;
      case NETMSGTYPE_RemoveNodeResp: { msg = (NetMessage*)RemoveNodeRespMsg_construct(); } break;
      case NETMSGTYPE_SetMirrorBuddyGroup: { msg = (NetMessage*)SetMirrorBuddyGroupMsgEx_construct(); } break;
      // storage messages
      case NETMSGTYPE_LookupIntentResp: { msg = (NetMessage*)LookupIntentRespMsg_construct(); } break;
      case NETMSGTYPE_MkDirResp: { msg = (NetMessage*)MkDirRespMsg_construct(); } break;
      case NETMSGTYPE_RmDirResp: { msg = (NetMessage*)RmDirRespMsg_construct(); } break;
      case NETMSGTYPE_MkFileResp: { msg = (NetMessage*)MkFileRespMsg_construct(); } break;
      case NETMSGTYPE_RefreshEntryInfoResp: { msg = (NetMessage*)RefreshEntryInfoRespMsg_construct(); } break;
      case NETMSGTYPE_RenameResp: { msg = (NetMessage*)RenameRespMsg_construct(); } break;
      case NETMSGTYPE_HardlinkResp: { msg = (NetMessage*)HardlinkRespMsg_construct(); }; break;
      case NETMSGTYPE_UnlinkFileResp: { msg = (NetMessage*)UnlinkFileRespMsg_construct(); } break;
      case NETMSGTYPE_MkLocalFileResp: { msg = (NetMessage*)MkLocalFileRespMsg_construct(); } break;
      case NETMSGTYPE_UnlinkLocalFileResp: { msg = (NetMessage*)UnlinkLocalFileRespMsg_construct(); } break;
      case NETMSGTYPE_ListDirFromOffsetResp: { msg = (NetMessage*)ListDirFromOffsetRespMsg_construct(); } break;
      case NETMSGTYPE_SetAttrResp: { msg = (NetMessage*)SetAttrRespMsg_construct(); } break;
      case NETMSGTYPE_StatResp: { msg = (NetMessage*)StatRespMsg_construct(); } break;
      case NETMSGTYPE_StatStoragePathResp: { msg = (NetMessage*)StatStoragePathRespMsg_construct(); } break;
      case NETMSGTYPE_TruncFileResp: { msg = (NetMessage*)TruncFileRespMsg_construct(); } break;
      case NETMSGTYPE_ListXAttrResp: { msg = (NetMessage*)ListXAttrRespMsg_construct(); } break;
      case NETMSGTYPE_GetXAttrResp: { msg = (NetMessage*)GetXAttrRespMsg_construct(); } break;
      case NETMSGTYPE_RemoveXAttrResp: { msg = (NetMessage*)RemoveXAttrRespMsg_construct(); } break;
      case NETMSGTYPE_SetXAttrResp: { msg = (NetMessage*)SetXAttrRespMsg_construct(); } break;
      // session messages
      case NETMSGTYPE_OpenFileResp: { msg = (NetMessage*)OpenFileRespMsg_construct(); } break;
      case NETMSGTYPE_CloseFileResp: { msg = (NetMessage*)CloseFileRespMsg_construct(); } break;
      case NETMSGTYPE_WriteLocalFileResp: { msg = (NetMessage*)WriteLocalFileRespMsg_construct(); } break;
      case NETMSGTYPE_FSyncLocalFileResp: { msg = (NetMessage*)FSyncLocalFileRespMsg_construct(); } break;
      case NETMSGTYPE_FLockAppendResp: { msg = (NetMessage*)FLockAppendRespMsg_construct(); } break;
      case NETMSGTYPE_FLockEntryResp: { msg = (NetMessage*)FLockEntryRespMsg_construct(); } break;
      case NETMSGTYPE_FLockRangeResp: { msg = (NetMessage*)FLockRangeRespMsg_construct(); } break;
      case NETMSGTYPE_LockGranted: { msg = (NetMessage*)LockGrantedMsgEx_construct(); } break;

      default:
      {
         msg = (NetMessage*)SimpleMsg_construct(NETMSGTYPE_Invalid);
      } break;
   }

   return msg;
}


