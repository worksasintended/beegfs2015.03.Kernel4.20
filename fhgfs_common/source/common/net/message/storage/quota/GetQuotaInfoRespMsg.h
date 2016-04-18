#ifndef GETQUOTAINFORESPMSG_H_
#define GETQUOTAINFORESPMSG_H_


#include <common/net/message/NetMessage.h>
#include <common/storage/quota/QuotaData.h>
#include <common/Common.h>

#define GETQUOTAINFORESPMSG_MAX_SIZE_FOR_QUOTA_DATA   ( (unsigned)(NETMSG_MAX_PAYLOAD_SIZE - 1024) )
#define GETQUOTAINFORESPMSG_MAX_ID_COUNT              ( (unsigned) \
   (GETQUOTAINFORESPMSG_MAX_SIZE_FOR_QUOTA_DATA / sizeof(QuotaData) ) )


class GetQuotaInfoRespMsg: public NetMessage
{
   public:
      GetQuotaInfoRespMsg(QuotaDataList* quotaData)
         : NetMessage(NETMSGTYPE_GetQuotaInfoResp)
      {
         this->quotaData = quotaData;
      }

      /**
       * For deserialization only
       */
      GetQuotaInfoRespMsg() : NetMessage(NETMSGTYPE_GetQuotaInfoResp)
      {
      }

   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenQuotaDataList(quotaData);     // quotaData
      }


   private:
      QuotaDataList* quotaData;

      // for deserialization
      unsigned quotaDataElemNum;
      const char* quotaDataStart;
      unsigned quotaDataBufLen;


   public:
      //getter and setter
      void parseQuotaDataList(QuotaDataList* outQuotaDataList)
      {
         Serialization::deserializeQuotaDataList(
            this->quotaDataBufLen, this->quotaDataElemNum, this->quotaDataStart, outQuotaDataList);
      }
};

#endif /* GETQUOTAINFORESPMSG_H_ */
