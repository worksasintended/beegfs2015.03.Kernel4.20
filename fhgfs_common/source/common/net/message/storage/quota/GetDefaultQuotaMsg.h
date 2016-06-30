#ifndef COMMON_NET_MESSAGE_STORAGE_QUOTA_GETDEFAULTQUOTAMSG_H_
#define COMMON_NET_MESSAGE_STORAGE_QUOTA_GETDEFAULTQUOTAMSG_H_



class GetDefaultQuotaMsg : public NetMessage
{
   public:
      GetDefaultQuotaMsg() : NetMessage(NETMSGTYPE_GetDefaultQuota) {};
      virtual ~GetDefaultQuotaMsg() {};

      void serializePayload(char* buf) {};

      bool deserializePayload(const char* buf, size_t bufLen)
      {
         return true;
      }

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH;
      }
};

#endif /* COMMON_NET_MESSAGE_STORAGE_QUOTA_GETDEFAULTQUOTAMSG_H_ */
