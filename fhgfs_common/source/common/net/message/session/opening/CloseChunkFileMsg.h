#ifndef CLOSECHUNKFILEMSG_H_
#define CLOSECHUNKFILEMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/HsmFileMetaData.h>
#include <common/storage/PathInfo.h>


#define CLOSECHUNKFILEMSG_FLAG_NODYNAMICATTRIBS    1 /* skip retrieval of current dyn attribs */
#define CLOSECHUNKFILEMSG_FLAG_WRITEENTRYINFO      2 /* write entryinfo as chunk xattrs */
#define CLOSECHUNKFILEMSG_FLAG_BUDDYMIRROR         4 /* given targetID is a buddymirrorgroup ID */
#define CLOSECHUNKFILEMSG_FLAG_BUDDYMIRROR_SECOND  8 /* secondary of group, otherwise primary */


class CloseChunkFileMsg : public NetMessage
{
   public:
      /**
       * @param sessionID just a reference, so do not free it as long as you use this object!
       * @param fileHandleID just a reference, so do not free it as long as you use this object!
       * @param parentID just a reference, so do not free it as long as you use this object!
       * @param pathInfo just a reference, so do not free it as long as you use this object!
       */
      CloseChunkFileMsg(std::string& sessionID, std::string& fileHandleID, uint16_t targetID,
         PathInfo* pathInfo) :
         NetMessage(NETMSGTYPE_CloseChunkFile)
      {
         this->sessionID = sessionID.c_str();
         this->sessionIDLen = sessionID.length();

         this->fileHandleID = fileHandleID.c_str();
         this->fileHandleIDLen = fileHandleID.length();

         this->targetID = targetID;

         this->pathInfoPtr = pathInfo;

         // the EntryInfo, which will be attached in the chunk's xattrs; not set per default
         this->entryInfoBuf = NULL;
         this->entryInfoBufLen = 0;

#ifdef BEEGFS_HSM_DEPRECATED
         this->hsmCollocationID = 0;
#endif
      }

      /**
       * For deserialization only!
       */
      CloseChunkFileMsg() : NetMessage(NETMSGTYPE_CloseChunkFile) { }

   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         unsigned msgLength =
            NETMSG_HEADER_LENGTH                               +
            Serialization::serialLenStrAlign4(sessionIDLen)    +
            Serialization::serialLenStrAlign4(fileHandleIDLen) +
            this->pathInfoPtr->serialLen()                     +
            Serialization::serialLenUShort();                     // targetID

         if (this->isMsgHeaderFeatureFlagSet(CLOSECHUNKFILEMSG_FLAG_WRITEENTRYINFO))
            msgLength += Serialization::serialLenUInt() + // entryInfoBufLen
               entryInfoBufLen; // entryInfoBuf

#ifdef BEEGFS_HSM_DEPRECATED
         msgLength +=
            Serialization::serialLenUShort(); // collocationID
#endif

         return msgLength;
      }

      unsigned getSupportedHeaderFeatureFlagsMask() const
      {
         return CLOSECHUNKFILEMSG_FLAG_NODYNAMICATTRIBS | CLOSECHUNKFILEMSG_FLAG_WRITEENTRYINFO |
            CLOSECHUNKFILEMSG_FLAG_BUDDYMIRROR | CLOSECHUNKFILEMSG_FLAG_BUDDYMIRROR_SECOND;
      }


   private:
      unsigned sessionIDLen;
      const char* sessionID;
      unsigned fileHandleIDLen;
      const char* fileHandleID;
      uint16_t targetID;

      // for serialization
      PathInfo *pathInfoPtr;

      // for deserialization
      PathInfo pathInfo;

      // for HSM integration
      unsigned entryInfoBufLen;
      const char* entryInfoBuf;

      unsigned short hsmCollocationID;

   public:
      // getters & setters

      const char* getSessionID() const
      {
         return sessionID;
      }
      
      const char* getFileHandleID() const
      {
         return fileHandleID;
      }

      uint16_t getTargetID() const
      {
         return targetID;
      }

      const char* getEntryInfoBuf() const
      {
         return entryInfoBuf;
      }

      unsigned getEntryInfoBufLen() const
      {
         return entryInfoBufLen;
      }

      void setEntryInfoBuf(const char* entryInfoBuf, unsigned entryInfoBufLen)
      {
         this->entryInfoBuf = entryInfoBuf;
         this->entryInfoBufLen = entryInfoBufLen;

         this->addMsgHeaderFeatureFlag(CLOSECHUNKFILEMSG_FLAG_WRITEENTRYINFO);
      }

      const PathInfo* getPathInfo() const
      {
         return &this->pathInfo;
      }

      /* for HSM integration */
      HsmCollocationID getHsmCollocationID() const
      {
         return this->hsmCollocationID;
      }

      void setHsmCollocationID(HsmCollocationID hsmCollocationID)
      {
         this->hsmCollocationID = hsmCollocationID;
      }

      virtual TestingEqualsRes testingEquals(NetMessage* msg)
      {
         CloseChunkFileMsg* msgIn = (CloseChunkFileMsg*) msg;

         if ( strcmp (this->sessionID, msgIn->getSessionID()) != 0 )
            return TestingEqualsRes_FALSE;

         if ( strcmp (this->fileHandleID, msgIn->getFileHandleID()) != 0 )
            return TestingEqualsRes_FALSE;

         if ( this->targetID != msgIn->getTargetID() )
            return TestingEqualsRes_FALSE;

         unsigned featureFlags = this->getMsgHeaderFeatureFlags();
         if ( featureFlags  != msgIn->getMsgHeaderFeatureFlags() )
            return TestingEqualsRes_FALSE;


         if (featureFlags & CLOSECHUNKFILEMSG_FLAG_WRITEENTRYINFO)
         {
            if ( this->entryInfoBufLen != msgIn->getEntryInfoBufLen() )
               return TestingEqualsRes_FALSE;

            if ( (entryInfoBuf != NULL) && (msgIn->getEntryInfoBuf() != NULL) )
            {
               int memcmpRes = memcmp(this->entryInfoBuf, msgIn->getEntryInfoBuf(),
                  this->entryInfoBufLen);
               if ( memcmpRes  != 0 )
                  return TestingEqualsRes_FALSE;
            }
            else
            if ( ! (entryInfoBuf == NULL) && (msgIn->getEntryInfoBuf() == NULL) )
               return TestingEqualsRes_FALSE;

#ifdef BEEGFS_HSM_DEPRECATED
            if ( this->hsmCollocationID != msgIn->getHsmCollocationID() )
               return TestingEqualsRes_FALSE;
#endif
         }

         return TestingEqualsRes_TRUE;
      }
};


#endif /*CLOSECHUNKFILEMSG_H_*/
