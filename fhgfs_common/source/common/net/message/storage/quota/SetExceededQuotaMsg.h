#ifndef SETEXCEEDEDQUOTAMSG_H_
#define SETEXCEEDEDQUOTAMSG_H_


#include <common/net/message/NetMessage.h>
#include <common/storage/quota/QuotaData.h>
#include <common/Common.h>


#define SETEXCEEDEDQUOTAMSG_MAX_SIZE_FOR_QUOTA_DATA   ( (unsigned)(NETMSG_MAX_PAYLOAD_SIZE - 1024) )
#define SETEXCEEDEDQUOTAMSG_MAX_ID_COUNT              ( (unsigned) \
   ( (SETEXCEEDEDQUOTAMSG_MAX_SIZE_FOR_QUOTA_DATA - (2 * sizeof(int) ) ) / sizeof(unsigned) ) )


class SetExceededQuotaMsg: public NetMessage
{
   public:
      SetExceededQuotaMsg(QuotaDataType idType, QuotaLimitType exType)
      : NetMessage(NETMSGTYPE_SetExceededQuota)
      {
         this->quotaDataType = idType;
         this->exceededType = exType;
      }

      SetExceededQuotaMsg(QuotaDataType idType, QuotaLimitType exType, unsigned short msgType)
      : NetMessage(msgType)
      {
         this->quotaDataType = idType;
         this->exceededType = exType;
      }

      /**
       * For deserialization only
       */
      SetExceededQuotaMsg() : NetMessage(NETMSGTYPE_SetExceededQuota)
      {
      }

      /**
       * For deserialization only
       */
      SetExceededQuotaMsg(unsigned short msgType) : NetMessage(msgType)
      {
      }

   private:
      int quotaDataType;         // QuotaDataType
      int exceededType;          // ExceededType
      UIntList exceededQuotaIDs;

      // for deserialization
      unsigned idListElemNum;
      const char* idListListStart;
      unsigned idListBufLen;


   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         unsigned retVal =  NETMSG_HEADER_LENGTH;

         retVal += Serialization::serialLenInt();                                // quotaDataType

         retVal += Serialization::serialLenInt();                                // exceededType

         retVal += Serialization::serialLenUIntList(&this->exceededQuotaIDs);    // exceededQuotaIDs

         return retVal;
      }


   public:
      //getters and setters

      QuotaDataType getQuotaDataType()
      {
         return (QuotaDataType) this->quotaDataType;
      }

      QuotaLimitType getExceededType()
      {
         return (QuotaLimitType) this->exceededType;
      }

      UIntList* getExceededQuotaIDs()
      {
         return &this->exceededQuotaIDs;
      }

      bool parseIDList(UIntList* outIDList)
      {
         return Serialization::deserializeUIntList(
            this->idListBufLen, this->idListElemNum, this->idListListStart, outIDList);
      }
};

#endif /* SETEXCEEDEDQUOTAMSG_H_ */
