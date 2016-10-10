#ifndef SETDIRPATTERNMSG_H_
#define SETDIRPATTERNMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/striping/StripePattern.h>
#include <common/storage/EntryInfo.h>
#include <common/Common.h>


class SetDirPatternMsg : public NetMessage
{
   public:
      struct Flags
      {
         static const uint32_t HAS_UID = 1;
      };

      /**
       * @param path just a reference, so do not free it as long as you use this object!
       * @param pattern just a reference, so do not free it as long as you use this object!
       */
      SetDirPatternMsg(EntryInfo* entryInfo, StripePattern* pattern) :
         NetMessage(NETMSGTYPE_SetDirPattern)
      {
         this->entryInfoPtr = entryInfo;

         this->pattern = pattern;
      }

      /**
       * For deserialization only
       */
      SetDirPatternMsg() : NetMessage(NETMSGTYPE_SetDirPattern)
      {
      }
   
   protected:

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      virtual unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            entryInfoPtr->serialLen() +
            pattern->getSerialPatternLength() +
            (isMsgHeaderFeatureFlagSet(Flags::HAS_UID)
               ? Serialization::serialLenUInt()
               : 0);
      }

      unsigned getSupportedHeaderFeatureFlagsMask() const
      {
         return Flags::HAS_UID;
      }
      
   private:
      uint32_t uid;

      // for serialization
      EntryInfo* entryInfoPtr;

      StripePattern* pattern; // not owned by this object!

      // for deserialization
      EntryInfo entryInfo;
      StripePatternHeader patternHeader;
      const char* patternStart;
      
   public:
      // inliners

      /**
       * @return must be deleted by the caller; might be of type STRIPEPATTERN_Invalid
       */
      StripePattern* createPattern()
      {
         return StripePattern::createFromBuf(patternStart, patternHeader);
      }

      EntryInfo* getEntryInfo()
      {
         return &this->entryInfo;
      }

      uint32_t getUID() const { return uid; }

      void setUID(uint32_t uid)
      {
         setMsgHeaderFeatureFlags(Flags::HAS_UID);
         this->uid = uid;
      }
};


#endif /*SETDIRPATTERNMSG_H_*/
