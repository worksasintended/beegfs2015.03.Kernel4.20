#ifndef SETQUOTAMSG_H_
#define SETQUOTAMSG_H_


#include <common/net/message/NetMessage.h>
#include <common/storage/quota/QuotaData.h>
#include <common/Common.h>

#define SETQUOTAMSG_MAX_SIZE_FOR_QUOTA_DATA   ( (unsigned)(NETMSG_MAX_PAYLOAD_SIZE - 1024) )
#define SETQUOTAMSG_MAX_ID_COUNT              ( (unsigned) \
   (SETQUOTAMSG_MAX_SIZE_FOR_QUOTA_DATA / sizeof(QuotaData) ) )


class SetQuotaMsg: public NetMessage
{
   public:
      SetQuotaMsg() : NetMessage(NETMSGTYPE_SetQuota)
      {
      }

   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenQuotaDataList(&quotaData);     // quotaData
      }


   private:
      QuotaDataList quotaData;

      // for deserialization
      unsigned quotaDataElemNum;
      const char* quotaDataStart;
      unsigned quotaDataBufLen;


   public:
      //getter and setter
      void parseQuotaDataList(QuotaDataList *outQuotaDataList)
      {
         Serialization::deserializeQuotaDataList(
            this->quotaDataBufLen, this->quotaDataElemNum, this->quotaDataStart, outQuotaDataList);
      }

      void insertQuotaLimit(QuotaData limit)
      {
         this->quotaData.push_back(limit);
      }
};

#endif /* SETQUOTAMSG_H_ */
