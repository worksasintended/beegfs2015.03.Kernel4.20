#ifndef GETXATTRRESPMSG_H_
#define GETXATTRRESPMSG_H_

#include <common/app/log/LogContext.h>
#include <common/net/message/NetMessage.h>
#include <common/Common.h>

class GetXAttrRespMsg : public NetMessage
{
   public:
      GetXAttrRespMsg(const CharVector& value, int size, int returnCode)
         : NetMessage(NETMSGTYPE_GetXAttrResp),
           value(value), size(size), returnCode(returnCode)
      {
      }

   protected:
      /**
       * For deserialization only.
       */
      GetXAttrRespMsg() : NetMessage(NETMSGTYPE_GetXAttrResp) {}

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      virtual unsigned calcMessageLength()
      {
         unsigned msgLen = NETMSG_HEADER_LENGTH +
            Serialization::serialLenCharVector(&this->value) + // value
            Serialization::serialLenInt() + // size
            Serialization::serialLenInt(); // returnCode

         return msgLen;
      }

   private:
      CharVector value;
      int size;
      int returnCode;

   public:
      // getters & setters
      const CharVector& getValue() const
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

#endif /*GETXATTRRESPMSG_H_*/
