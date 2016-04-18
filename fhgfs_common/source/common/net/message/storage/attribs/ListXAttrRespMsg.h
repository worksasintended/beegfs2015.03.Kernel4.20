#ifndef LISTXATTRRESPMSG_H_
#define LISTXATTRRESPMSG_H_

#include <common/app/log/LogContext.h>
#include <common/net/message/NetMessage.h>
#include <common/Common.h>

class ListXAttrRespMsg : public NetMessage
{
   public:
      ListXAttrRespMsg(const StringVector& value, int size, int returnCode)
         : NetMessage(NETMSGTYPE_ListXAttrResp),
           value(value), size(size), returnCode(returnCode)
      {
      }

   protected:
      /**
       * For deserialization only.
       */
      ListXAttrRespMsg() : NetMessage(NETMSGTYPE_ListXAttrResp) {}

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      virtual unsigned calcMessageLength()
      {
         unsigned msgLen = NETMSG_HEADER_LENGTH +
            Serialization::serialLenStringVec(&value) + // value
            Serialization::serialLenInt() + // size
            Serialization::serialLenInt(); // returnCode

         return msgLen;
      }

   private:
      StringVector value;
      int size;
      int returnCode;


   public:
      // getters & setters
      const StringVector& getValue() const
      {
         return this->value;
      }

      int getReturnCode() const
      {
         return this->returnCode;
      }

      int getSize() const
      {
         return this->size;
      }
};

#endif /*LISTXATTRRESPMSG_H_*/
