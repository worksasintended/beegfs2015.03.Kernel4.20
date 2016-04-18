#ifndef OPENFILERESPMSG_H_
#define OPENFILERESPMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/striping/StripePattern.h>
#include <common/storage/PathInfo.h>
#include <common/Common.h>


class OpenFileRespMsg : public NetMessage
{
   public:
      /**
       * @param fileHandleID just a reference, so do not free it as long as you use this object!
       */
      OpenFileRespMsg(int result, const char* fileHandleID, StripePattern* pattern,
         PathInfo* pathInfo) :
         NetMessage(NETMSGTYPE_OpenFileResp)
      {
         this->result = result;

         this->fileHandleID = fileHandleID;
         this->fileHandleIDLen = strlen(fileHandleID);

         this->pattern = pattern;

         this->pathInfo.set(pathInfo);
      }

      /**
       * @param fileHandleID just a reference, so do not free it as long as you use this object!
       */
      OpenFileRespMsg(int result, std::string& fileHandleID, StripePattern* pattern,
         PathInfo* pathInfo) : NetMessage(NETMSGTYPE_OpenFileResp)
      {
         this->result = result;

         this->fileHandleID = fileHandleID.c_str();
         this->fileHandleIDLen = fileHandleID.length();

         this->pattern = pattern;

         this->pathInfo.set(pathInfo);
      }

      /**
       * For deserialization only!
       */
      OpenFileRespMsg() : NetMessage(NETMSGTYPE_OpenFileResp) { }
   

   protected:

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      virtual unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenInt()                      +  // result
            Serialization::serialLenStrAlign4(fileHandleIDLen) +
            pathInfo.serialLen()                               +
            pattern->getSerialPatternLength();
      }
      
   private:
      int result;
      unsigned fileHandleIDLen;
      const char* fileHandleID;
      PathInfo pathInfo;

      // for serialization
      StripePattern* pattern; // not owned by this object!
      
      // for deserialization
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

      // getters & setters
      int getResult() const
      {
         return result;
      }
      
      const char* getFileHandleID()
      {
         return fileHandleID;
      }
      
      PathInfo* getPathInfo()
      {
         return &this->pathInfo;
      }
};

#endif /*OPENFILERESPMSG_H_*/
