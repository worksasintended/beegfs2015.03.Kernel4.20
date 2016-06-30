#ifndef COMMON_NET_MESSAGE_STORAGE_QUOTA_GETDEFAULTQUOTARESPMSG_H_
#define COMMON_NET_MESSAGE_STORAGE_QUOTA_GETDEFAULTQUOTARESPMSG_H_


#include <common/net/message/NetMessage.h>
#include <common/storage/quota/QuotaDefaultLimits.h>



class GetDefaultQuotaRespMsg: public NetMessage
{
   public:
      /**
       * For deserialization only
       */
      GetDefaultQuotaRespMsg() : NetMessage(NETMSGTYPE_GetDefaultQuotaResp) {};
      GetDefaultQuotaRespMsg(QuotaDefaultLimits defaultLimits) :
         NetMessage(NETMSGTYPE_GetDefaultQuotaResp), limits(defaultLimits) {};
      virtual ~GetDefaultQuotaRespMsg() {};


   private:
      QuotaDefaultLimits limits;


   public:
      void serializePayload(char* buf)
      {
         limits.serialize(buf);
      }

      bool deserializePayload(const char* buf, size_t bufLen)
      {
         unsigned outLen;
         return limits.deserialize(buf, bufLen, &outLen);
      }

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            limits.serialLen();
      }

      QuotaDefaultLimits& getDefaultLimits()
      {
         return limits;
      }
};

#endif /* COMMON_NET_MESSAGE_STORAGE_QUOTA_GETDEFAULTQUOTARESPMSG_H_ */
