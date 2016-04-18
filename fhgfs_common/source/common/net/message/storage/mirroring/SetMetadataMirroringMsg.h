#ifndef SETMETADATAMIRRORINGMSG_H_
#define SETMETADATAMIRRORINGMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/EntryInfo.h>
#include <common/Common.h>


/**
 * Enable metadata mirroring for a directory.
 */
class SetMetadataMirroringMsg : public NetMessage
{
   public:
      /**
       * @param path just a reference, so do not free it as long as you use this object!
       * @param pattern just a reference, so do not free it as long as you use this object!
       */
      SetMetadataMirroringMsg(EntryInfo* entryInfo) :
         NetMessage(NETMSGTYPE_SetMetadataMirroring)
      {
         this->entryInfoPtr = entryInfo;
      }

      /**
       * For deserialization only
       */
      SetMetadataMirroringMsg() : NetMessage(NETMSGTYPE_SetMetadataMirroring)
      {
      }
   
   protected:

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      virtual unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            entryInfoPtr->serialLen();
      }
      
   private:

      // for serialization
      EntryInfo* entryInfoPtr;

      // for deserialization
      EntryInfo entryInfo;

      
   public:
      // getters & setters

      EntryInfo* getEntryInfo(void)
      {
         return &this->entryInfo;
      }

};


#endif /*SETMETADATAMIRRORINGMSG_H_*/
