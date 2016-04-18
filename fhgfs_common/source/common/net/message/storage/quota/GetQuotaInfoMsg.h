#ifndef GETQUOTAINFOMSG_H_
#define GETQUOTAINFOMSG_H_


#include <common/net/message/NetMessage.h>
#include <common/storage/quota/QuotaData.h>
#include <common/storage/quota/GetQuotaConfig.h>
#include <common/Common.h>



#define GETQUOTAINFOMSG_FEATURE_QUOTA_PER_TARGET         1

/*
 * enum for the different quota query types
 */
enum QuotaQueryType
{
   GetQuotaInfo_QUERY_TYPE_NONE = 0,
   GetQuotaInfo_QUERY_TYPE_SINGLE_ID = 1,
   GetQuotaInfo_QUERY_TYPE_ID_RANGE = 2,
   GetQuotaInfo_QUERY_TYPE_ID_LIST = 3,
   GetQuotaInfo_QUERY_TYPE_ALL_ID = 4
};


class GetQuotaInfoMsg: public NetMessage
{
   public:
      GetQuotaInfoMsg(QuotaDataType type) : NetMessage(NETMSGTYPE_GetQuotaInfo)
      {
         this->type = type;
         this->targetSelection = GETQUOTACONFIG_ALL_TARGETS_ONE_REQUEST;
         this->targetNumID = 0;
      }

      /*
       * deserialization only
       */
      GetQuotaInfoMsg() : NetMessage(NETMSGTYPE_GetQuotaInfo)
      {
      }


   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
          unsigned retVal =  NETMSG_HEADER_LENGTH;
          retVal += Serialization::serialLenInt();                         // queryType

          retVal += Serialization::serialLenInt();                         // type

          if(queryType == GetQuotaInfo_QUERY_TYPE_ID_RANGE)
          {
             retVal += Serialization::serialLenUInt();                     // idRangeStart
             retVal += Serialization::serialLenUInt();                     // idRangeEnd
          }

          if(queryType == GetQuotaInfo_QUERY_TYPE_ID_LIST)
             retVal += Serialization::serialLenUIntList(&this->idList);     // idList

          if(queryType == GetQuotaInfo_QUERY_TYPE_SINGLE_ID)
             retVal += Serialization::serialLenUInt();                     // idRangeStart

          retVal += Serialization::serialLenUInt();                        // targetSelection

          retVal += Serialization::serialLenUInt16();                      // targetNumID

          return retVal;
      }


   private:
      int queryType;          // QuotaQueryType
      int type;               // QuotaDataType
      unsigned idRangeStart;
      unsigned idRangeEnd;
      UIntList idList;
      unsigned targetSelection;
      uint16_t targetNumID;

      // for deserialization
      unsigned idListElemNum;
      const char* idListListStart;
      unsigned idListBufLen;


   public:
      //getter and setter
      unsigned getIDRangeStart() const
      {
         return this->idRangeStart;
      }

      unsigned getIDRangeEnd() const
      {
         return this->idRangeEnd;
      }

      QuotaDataType getType() const
      {
         return (QuotaDataType)type;
      }

      QuotaQueryType getQueryType() const
      {
         return (QuotaQueryType)queryType;
      }

      unsigned getID() const
      {
         return this->idRangeStart;
      }

      bool parseIDList(UIntList* outIDList)
      {
         return Serialization::deserializeUIntList(
            this->idListBufLen, this->idListElemNum, this->idListListStart, outIDList);
      }

      UIntList* getIDList()
      {
         return &this->idList;
      }

      void setQueryType(QuotaQueryType queryType)
      {
         this->queryType = queryType;
      }

      void setID(unsigned id)
      {
         this->queryType = GetQuotaInfo_QUERY_TYPE_SINGLE_ID;
         this->idRangeStart = id;
      }

      void setIDRange(unsigned idRangeStart, unsigned idRangeEnd)
      {
         this->queryType = GetQuotaInfo_QUERY_TYPE_ID_RANGE;
         this->idRangeStart = idRangeStart;
         this->idRangeEnd = idRangeEnd;
      }

      uint16_t getTargetNumID()
      {
         return this->targetNumID;
      }

      unsigned getTargetSelection() const
      {
         return this->targetSelection;
      }

      void setTargetSelectionSingleTarget(uint16_t newTargetNumID)
      {
         this->targetNumID = newTargetNumID;
         this->targetSelection = GETQUOTACONFIG_SINGLE_TARGET;
      }

      void setTargetSelectionAllTargetsOneRequestPerTarget(uint16_t newTargetNumID)
      {
         this->targetNumID = newTargetNumID;
         this->targetSelection = GETQUOTACONFIG_ALL_TARGETS_ONE_REQUEST_PER_TARGET;
      }

      void setTargetSelectionAllTargetsOneRequestForAllTargets()
      {
         this->targetNumID = 0;
         this->targetSelection = GETQUOTACONFIG_ALL_TARGETS_ONE_REQUEST;
      }
};

#endif /* GETQUOTAINFOMSG_H_ */
