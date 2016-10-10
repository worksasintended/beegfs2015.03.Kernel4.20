#ifndef REQUESTEXCEEDEDQUOTARESPMSG_H_
#define REQUESTEXCEEDEDQUOTARESPMSG_H_


#include <common/net/message/storage/quota/SetExceededQuotaMsg.h>
#include <common/storage/quota/QuotaData.h>
#include <common/Common.h>


#define REQUESTEXCEEDEDQUOTARESPMSG_MAX_SIZE_FOR_QUOTA_DATA    (\
   SETEXCEEDEDQUOTAMSG_MAX_SIZE_FOR_QUOTA_DATA)
#define REQUESTEXCEEDEDQUOTARESPMSG_MAX_ID_COUNT               (SETEXCEEDEDQUOTAMSG_MAX_ID_COUNT)
#define REQUESTEXCEEDEDQUOTARESPMSG_FLAG_QUOTA_CONFIG      1 /* contains the quota enforcement
                                                                configuration from the mgmtd */

class RequestExceededQuotaRespMsg: public SetExceededQuotaMsg
{
   public:
      RequestExceededQuotaRespMsg(QuotaDataType idType, QuotaLimitType exType, int error)
      : SetExceededQuotaMsg(idType, exType, NETMSGTYPE_RequestExceededQuotaResp), error(error)
      {
      }

      /**
       * For deserialization only
       */
      RequestExceededQuotaRespMsg() : SetExceededQuotaMsg(NETMSGTYPE_RequestExceededQuotaResp),
         error(FhgfsOpsErr_INVAL)
      {
      }

      FhgfsOpsErr getError()
      {
         return (FhgfsOpsErr)error;
      }

      void setError(FhgfsOpsErr newError)
      {
         error = newError;
      }

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         unsigned retVal = SetExceededQuotaMsg::calcMessageLength();

         if(isMsgHeaderCompatFeatureFlagSet(REQUESTEXCEEDEDQUOTARESPMSG_FLAG_QUOTA_CONFIG) )
            retVal += Serialization::serialLenInt();                     // error

         return retVal;
      }

   protected:
      unsigned getSupportedHeaderFeatureFlagsMask() const
      {
         return REQUESTEXCEEDEDQUOTARESPMSG_FLAG_QUOTA_CONFIG;
      }

   private:
      int error;

};

#endif /* REQUESTEXCEEDEDQUOTARESPMSG_H_ */
