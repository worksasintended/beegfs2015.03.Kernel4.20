#ifndef GENERICRESPONSEMSG_H_
#define GENERICRESPONSEMSG_H_

#include <common/net/message/SimpleIntStringMsg.h>


/**
 * Note: Remember to keep this in sync with client.
 */
enum GenericRespMsgCode
{
   GenericRespMsgCode_TRYAGAIN          =  0, /* requestor shall try again in a few seconds */
   GenericRespMsgCode_INDIRECTCOMMERR   =  1, /* indirect communication error and requestor should
      try again (e.g. msg forwarding to other server failed) */
   GenericRespMsgCode_INDIRECTCOMMERR_NOTAGAIN =  2, /* indirect communication error, and requestor
      shall not try again (e.g. msg forwarding to other server failed and the other server is marked
      offline) */
};


/**
 * A generic response that can be sent as a reply to any message. This special control message will
 * be handled internally by the requestors MessageTk::requestResponse...() method.
 *
 * This is intended to submit internal information (like asking for a communication retry) to the
 * requestors MessageTk through the control code. So the MessageTk caller on the requestor side
 * will never actually see this GenericResponseMsg.
 *
 * The message string is intended to provide additional human-readable information like why this
 * control code was submitted instead of the actually expected response msg.
 */
class GenericResponseMsg : public SimpleIntStringMsg
{
   public:
      /**
       * @param controlCode GenericRespMsgCode_...
       * @param logStr human-readable explanation or background information.
       */
      GenericResponseMsg(GenericRespMsgCode controlCode, const char* logStr) :
         SimpleIntStringMsg(NETMSGTYPE_GenericResponse, controlCode, logStr)
      {
      }

      /**
       * For deserialization only.
       */
      GenericResponseMsg() : SimpleIntStringMsg(NETMSGTYPE_GenericResponse)
      {
      }


   private:


   public:
      // getters & setters
      GenericRespMsgCode getControlCode() const
      {
         return (GenericRespMsgCode)getIntValue();
      }

      const char* getLogStr() const
      {
         return getStrValue();
      }

};


#endif /* GENERICRESPONSEMSG_H_ */
