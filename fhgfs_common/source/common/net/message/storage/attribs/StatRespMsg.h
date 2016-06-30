#ifndef STATRESPMSG_H_
#define STATRESPMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/Metadata.h>
#include <common/storage/StatData.h>
#include <common/Common.h>


#define STATRESPMSG_COMPAT_FLAG_HAS_PARENTOWNERNODEID    1 /* deprecated, no longer used
                                                              (due to missing parentEntryID) */

#define STATRESPMSG_FLAG_HAS_PARENTINFO                  1 /* msg includes parentOwnerNodeID and
                                                              parentEntryID */


class StatRespMsg : public NetMessage
{
   public:
      StatRespMsg(int result, StatData statData) :
         NetMessage(NETMSGTYPE_StatResp)
      {
         this->result = result;
         this->statData = statData;
      }

      /**
       * For deserialization only.
       */
      StatRespMsg() : NetMessage(NETMSGTYPE_StatResp)
      {
         this->parentNodeID = 0;
      }
   
   protected:

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      virtual unsigned calcMessageLength()
      {
         unsigned msgLen = NETMSG_HEADER_LENGTH       +
            Serialization::serialLenInt()             +  // result
            this->statData.serialLen(false, true, true); // statData

         if(isMsgHeaderFeatureFlagSet(STATRESPMSG_FLAG_HAS_PARENTINFO) )
         {
            msgLen += Serialization::serialLenStrAlign4(this->parentEntryID.length() );
            msgLen += Serialization::serialLenUInt16(); // parentNodeID
         }

         return msgLen;
      }
      
      virtual unsigned getSupportedHeaderFeatureFlagsMask() const
      {
         return STATRESPMSG_FLAG_HAS_PARENTINFO;
      }


   private:
      int result;
      StatData statData;

      uint16_t parentNodeID;
      std::string parentEntryID;

   public:
      // getters & setters
      int getResult()
      {
         return result;
      }

      StatData* getStatData()
      {
         return &this->statData;
      }

      void addParentInfo(uint16_t parentNodeID, std::string parentEntryID)
      {
         this->parentNodeID  = parentNodeID;
         this->parentEntryID = parentEntryID;

         addMsgHeaderFeatureFlag(STATRESPMSG_FLAG_HAS_PARENTINFO);
      }

      uint16_t getParentNodeID()
      {
         return this->parentNodeID;
      }
};

#endif /*STATRESPMSG_H_*/
