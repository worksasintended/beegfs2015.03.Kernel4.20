#ifndef COMMON_NET_MESSAGE_STORAGE_QUOTA_SETDEFAULTQUOTAMSG_H_
#define COMMON_NET_MESSAGE_STORAGE_QUOTA_SETDEFAULTQUOTAMSG_H_


#include <common/net/message/NetMessage.h>
#include <common/storage/quota/QuotaData.h>



class SetDefaultQuotaMsg : public NetMessage
{
   public:
      SetDefaultQuotaMsg() : NetMessage(NETMSGTYPE_SetDefaultQuota) {};
      SetDefaultQuotaMsg(QuotaDataType newType, uint64_t newSize, uint64_t newInodes) :
         NetMessage(NETMSGTYPE_SetDefaultQuota), type(newType), size(newSize), inodes(newInodes) {};
      virtual ~SetDefaultQuotaMsg() {};

   protected:
      int type;            // enum QuotaDataType
      uint64_t size;       // defined size limit or used size (bytes)
      uint64_t inodes;     // defined inodes limit or used inodes (counter)

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         unsigned retVal =  NETMSG_HEADER_LENGTH;

         retVal += Serialization::serialLenInt();        // type

         retVal += Serialization::serialLenUInt64();     // size

         retVal += Serialization::serialLenUInt64();     // inodes

         return retVal;
      }


   public:
      //getter and setter

      uint64_t getSize() const
      {
         return size;
      }

      uint64_t getInodes() const
      {
         return inodes;
      }

      QuotaDataType getType() const
      {
         return (QuotaDataType)type;
      }

      void updateLimits(QuotaDataType newType, uint64_t newSize, uint64_t newInodes)
      {
         type = newType;
         size = newSize;
         inodes = newInodes;
      }
};

#endif /* COMMON_NET_MESSAGE_STORAGE_QUOTA_SETDEFAULTQUOTAMSG_H_ */
