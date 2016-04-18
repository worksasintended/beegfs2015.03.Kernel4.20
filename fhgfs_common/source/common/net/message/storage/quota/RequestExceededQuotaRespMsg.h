#ifndef REQUESTEXCEEDEDQUOTARESPMSG_H_
#define REQUESTEXCEEDEDQUOTARESPMSG_H_


#include <common/net/message/storage/quota/SetExceededQuotaMsg.h>
#include <common/storage/quota/QuotaData.h>
#include <common/Common.h>


#define REQUESTEXCEEDEDQUOTARESPMSG_MAX_SIZE_FOR_QUOTA_DATA    (\
   SETEXCEEDEDQUOTAMSG_MAX_SIZE_FOR_QUOTA_DATA)
#define REQUESTEXCEEDEDQUOTARESPMSG_MAX_ID_COUNT               (SETEXCEEDEDQUOTAMSG_MAX_ID_COUNT)


class RequestExceededQuotaRespMsg: public SetExceededQuotaMsg
{
   public:
      RequestExceededQuotaRespMsg(QuotaDataType idType, QuotaLimitType exType)
      : SetExceededQuotaMsg(idType, exType, NETMSGTYPE_RequestExceededQuotaResp)
      {
      }

      /**
       * For deserialization only
       */
      RequestExceededQuotaRespMsg() : SetExceededQuotaMsg(NETMSGTYPE_RequestExceededQuotaResp)
      {
      }
};

#endif /* REQUESTEXCEEDEDQUOTARESPMSG_H_ */
