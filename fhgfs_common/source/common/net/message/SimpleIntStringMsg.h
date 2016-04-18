#ifndef SIMPLEINTSTRINGMSG_H_
#define SIMPLEINTSTRINGMSG_H_

#include "NetMessage.h"

/**
 * Simple message containing an integer value and a string (e.g. int error code and human-readable
 * explantion with more details as string).
 */
class SimpleIntStringMsg : public NetMessage
{
   protected:
      /**
       * @param strValue just a reference
       */
      SimpleIntStringMsg(unsigned short msgType, int intValue, const char* strValue) :
         NetMessage(msgType)
      {
         this->intValue = intValue;

         this->strValue = strValue;
         this->strValueLen = strlen(strValue);
      }

      /**
       * @param strValue just a reference
       */
      SimpleIntStringMsg(unsigned short msgType, int intValue, std::string& strValue) :
         NetMessage(msgType)
      {
         this->intValue = intValue;

         this->strValue = strValue.c_str();
         this->strValueLen = strValue.length();
      }

      /**
       * For deserialization only!
       */
      SimpleIntStringMsg(unsigned short msgType) : NetMessage(msgType)
      {
      }

      virtual void serializePayload(char* buf)
      {
         size_t bufPos = 0;

         bufPos += Serialization::serializeInt(&buf[bufPos], intValue);

         bufPos += Serialization::serializeStr(&buf[bufPos], strValueLen, strValue);
      }

      virtual bool deserializePayload(const char* buf, size_t bufLen)
      {
         size_t bufPos = 0;

         { // intValue
            unsigned deserLen;

            bool deserRes = Serialization::deserializeInt(
               &buf[bufPos], bufLen - bufPos, &intValue, &deserLen);

            if(unlikely(!deserRes) )
               return false;

            bufPos += deserLen;
         }

         { // strValue
            unsigned deserLen;

            bool deserRes = Serialization::deserializeStr(
               &buf[bufPos], bufLen - bufPos, &strValueLen, &strValue, &deserLen);

            if(unlikely(!deserRes) )
               return false;

            bufPos += deserLen;
         }

         return true;
      }

      virtual unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenInt() + // intValue
            Serialization::serialLenStr(strValueLen); // strValue
      }


   private:
      int intValue;

      const char* strValue;
      unsigned strValueLen;


   public:
      // getters & setters

      int getIntValue() const
      {
         return intValue;
      }

      const char* getStrValue() const
      {
         return strValue;
      }

};


#endif /* SIMPLEINTSTRINGMSG_H_ */
