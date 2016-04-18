#ifndef REQUESTEXCEEDEDQUOTAMSG_H_
#define REQUESTEXCEEDEDQUOTAMSG_H_


#include <common/net/message/NetMessage.h>
#include <common/storage/quota/QuotaData.h>
#include <common/Common.h>

class RequestExceededQuotaMsg: public NetMessage
{
   public:
      RequestExceededQuotaMsg(QuotaDataType idType, QuotaLimitType exType)
      : NetMessage(NETMSGTYPE_RequestExceededQuota)
      {
         this->quotaDataType = idType;
         this->exceededType = exType;
      }

      /**
       * For deserialization only
       */
      RequestExceededQuotaMsg() : NetMessage(NETMSGTYPE_RequestExceededQuota)
      {
      }


   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         unsigned retVal =  NETMSG_HEADER_LENGTH;

         retVal += Serialization::serialLenInt();                                // quotaDataType

         retVal += Serialization::serialLenInt();                                // exceededType

         return retVal;
      }


   private:
      int quotaDataType;
      int exceededType;


   public:
      // getters and setters
      QuotaDataType getQuotaDataType()
      {
         return (QuotaDataType) this->quotaDataType;
      }

      QuotaLimitType getExceededType()
      {
         return (QuotaLimitType) this->exceededType;
      }
};

#endif /* REQUESTEXCEEDEDQUOTAMSG_H_ */
