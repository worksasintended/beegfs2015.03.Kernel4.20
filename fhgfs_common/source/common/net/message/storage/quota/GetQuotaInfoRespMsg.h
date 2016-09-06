#ifndef GETQUOTAINFORESPMSG_H_
#define GETQUOTAINFORESPMSG_H_


#include <common/net/message/NetMessage.h>
#include <common/storage/quota/QuotaData.h>
#include <common/storage/quota/Quota.h>
#include <common/Common.h>

#define GETQUOTAINFORESPMSG_MAX_SIZE_FOR_QUOTA_DATA   ( (unsigned)(NETMSG_MAX_PAYLOAD_SIZE - 1024) )
#define GETQUOTAINFORESPMSG_MAX_ID_COUNT              ( (unsigned) \
   ( (GETQUOTAINFORESPMSG_MAX_SIZE_FOR_QUOTA_DATA - sizeof(unsigned) ) / sizeof(QuotaData) ) )


class GetQuotaInfoRespMsg: public NetMessage
{
   public:
      /**
       * Constructor for quota limits (management server).
       */
      GetQuotaInfoRespMsg(QuotaDataList* quotaData)
         : NetMessage(NETMSGTYPE_GetQuotaInfoResp), quotaData(quotaData),
           quotaInodeSupport(QuotaInodeSupport_UNKNOWN)
      {
         addMsgHeaderCompatFeatureFlag(MsgCompatFlags::QUOTA_INODE_SUPPORT);
      };

      /**
       * Constructor for quota data (storage server).
       */
      GetQuotaInfoRespMsg(QuotaDataList* quotaData, QuotaInodeSupport inodeSupport)
         : NetMessage(NETMSGTYPE_GetQuotaInfoResp), quotaData(quotaData),
           quotaInodeSupport(inodeSupport)
      {
         addMsgHeaderCompatFeatureFlag(MsgCompatFlags::QUOTA_INODE_SUPPORT);
      };

      /**
       * For deserialization only
       */
      GetQuotaInfoRespMsg() : NetMessage(NETMSGTYPE_GetQuotaInfoResp)
      {
         quotaInodeSupport = QuotaInodeSupport_UNKNOWN;  // if the sender doesn't provide it
      }

      struct MsgCompatFlags
      {
         // add quota inode support level of the blockdevices
         static const unsigned QUOTA_INODE_SUPPORT = 1;
      };


   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         unsigned len = NETMSG_HEADER_LENGTH +
            Serialization::serialLenQuotaDataList(quotaData);     // quotaData

         if(isMsgHeaderCompatFeatureFlagSet(MsgCompatFlags::QUOTA_INODE_SUPPORT) )
         {
            len += Serialization::serialLenUInt();                // quotaInodeSupport
         }

         return len;
      }


   private:
      QuotaDataList* quotaData;
      uint32_t quotaInodeSupport;

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

      QuotaInodeSupport getQuotaInodeSupport()
      {
         return (QuotaInodeSupport)quotaInodeSupport;
      }
};

#endif /* GETQUOTAINFORESPMSG_H_ */
