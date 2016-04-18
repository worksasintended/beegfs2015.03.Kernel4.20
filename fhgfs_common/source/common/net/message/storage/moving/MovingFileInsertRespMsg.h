#ifndef MOVINGFILEINSERTRESPMSG_H_
#define MOVINGFILEINSERTRESPMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/striping/StripePattern.h>
#include <common/Common.h>


class MovingFileInsertRespMsg : public NetMessage
{
   public:
      /**
       * @param inodeBuf inode of overwritten file, might be NULL if none (and inodeBufLen must also
       *    be 0 in that case).
       */
      MovingFileInsertRespMsg(FhgfsOpsErr result, unsigned inodeBufLen, char* inodeBuf) :
         NetMessage(NETMSGTYPE_MovingFileInsertResp)
      {
         this->result      = result;
         this->inodeBufLen = inodeBufLen;
         this->inodeBuf    = inodeBuf;
      }

      /**
       * Constructor for deserialization only.
       */
      MovingFileInsertRespMsg() : NetMessage(NETMSGTYPE_MovingFileInsertResp)
      {
      }
   

   protected:

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      virtual unsigned calcMessageLength()
      {
         unsigned msgLen =
            NETMSG_HEADER_LENGTH             +
            Serialization::serialLenInt()    + // result
            Serialization::serialLenUInt()   + // inodeBufLen
            this->inodeBufLen;                 // value of inodeBufLen

         return msgLen;
      }
      
   private:
      int result;
      unsigned inodeBufLen; // might be 0 if there was no file overwritten
      char* inodeBuf;       // might be NULL if there was no file overwritten


   public:
      // getters & setters

      FhgfsOpsErr getResult() const
      {
         return (FhgfsOpsErr)this->result;
      }
      
      unsigned getInodeBufLen() const
      {
         return this->inodeBufLen;
      }

      const char* getInodeBuf() const
      {
         return this->inodeBuf;
      }

};

#endif /*MOVINGFILEINSERTRESPMSG_H_*/
