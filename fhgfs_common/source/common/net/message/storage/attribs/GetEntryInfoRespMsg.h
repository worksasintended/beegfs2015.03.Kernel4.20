#ifndef GETENTRYINFORESPMSG_H_
#define GETENTRYINFORESPMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/striping/StripePattern.h>
#include <common/storage/StorageErrors.h>
#include <common/storage/PathInfo.h>
#include <common/Common.h>

/**
 * Retrieve information about an entry, such as the stripe pattern or mirroring settings.
 * This is typically used by "fhgfs-ctl --getentryinfo" or the admon file browser.
 */
class GetEntryInfoRespMsg : public NetMessage
{
   public:
      /**
       * @param mirrorNodeID ID of metadata mirror node
       */
      GetEntryInfoRespMsg(FhgfsOpsErr result, StripePattern* pattern, uint16_t mirrorNodeID,
         PathInfo* pathInfoPtr) :  NetMessage(NETMSGTYPE_GetEntryInfoResp)
      {
         this->result = result;
         this->pattern = pattern;
         this->mirrorNodeID = mirrorNodeID;
         this->pathInfoPtr = pathInfoPtr;
      }

      /**
       * For deserialization only
       */
      GetEntryInfoRespMsg() : NetMessage(NETMSGTYPE_GetEntryInfoResp)
      {
      }
   
   protected:

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      virtual unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH               +
            Serialization::serialLenInt()          + // result
            pattern->getSerialPatternLength()      + // pattern
            this->pathInfoPtr->serialLen()         + // pathInfo
            Serialization::serialLenUInt16();        // mirrorNodeID
      }
      
   private:
      int result;
      uint16_t mirrorNodeID; // metadata mirror node (0 means "none")

      // for serialization
      StripePattern* pattern; // not owned by this object!
      PathInfo* pathInfoPtr;  // not owned by this object!
      
      // for deserialization
      StripePatternHeader patternHeader;
      const char* patternStart;
      PathInfo pathInfo;
      
   public:
      // inliners

      /**
       * @return must be deleted by the caller; might be of type STRIPEPATTERN_Invalid
       */
      StripePattern* createPattern()
      {
         return StripePattern::createFromBuf(patternStart, patternHeader);
      }

      // getters & setters
      
      FhgfsOpsErr getResult() const
      {
         return (FhgfsOpsErr)result;
      }
      
      uint16_t getMirrorNodeID() const
      {
         return mirrorNodeID;
      }

      PathInfo* getPathInfo()
      {
         return &this->pathInfo;
      }

};


#endif /*GETENTRYINFORESPMSG_H_*/
