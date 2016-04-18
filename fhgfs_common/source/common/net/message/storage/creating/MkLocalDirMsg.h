#ifndef MKLOCALDIRMSG_H_
#define MKLOCALDIRMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/striping/StripePattern.h>
#include <common/storage/HsmFileMetaData.h>
#include <common/storage/Path.h>


#define MKLOCALDIRMSG_FLAG_MIRRORED    1


class MkLocalDirMsg : public NetMessage
{
   friend class AbstractNetMessageFactory;

   public:

      /**
       * @param entryInfo just a reference, so do not free it as long as you use this object!
       */
      MkLocalDirMsg(EntryInfo* entryInfo, unsigned userID, unsigned groupID, int mode,
         StripePattern* pattern, uint16_t parentNodeID,
         const CharVector& defaultACLXAttr, const CharVector& accessACLXAttr) :
            NetMessage(NETMSGTYPE_MkLocalDir),
            defaultACLXAttr(defaultACLXAttr), accessACLXAttr(accessACLXAttr)
      {
         this->entryInfoPtr = entryInfo;
         this->userID = userID;
         this->groupID = groupID;
         this->mode = mode;
         this->pattern = pattern;
         this->parentNodeID = parentNodeID;
      }


   protected:
      /**
       * For deserialization only!
       */
      MkLocalDirMsg() : NetMessage(NETMSGTYPE_MkLocalDir) {}


      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         unsigned msgLength = NETMSG_HEADER_LENGTH               +
            Serialization::serialLenUInt()                       + // userID
            Serialization::serialLenUInt()                       + // groupID
            Serialization::serialLenInt()                        + // mode
            entryInfoPtr->serialLen()                            + // entryInfo
            pattern->getSerialPatternLength()                    +
            Serialization::serialLenUShort()                     + // parentNodeID
            Serialization::serialLenCharVector(&defaultACLXAttr) + // defaultACLXAttr
            Serialization::serialLenCharVector(&accessACLXAttr);   // accessACLXAttr

         if(isMsgHeaderFeatureFlagSet(MKLOCALDIRMSG_FLAG_MIRRORED) )
            msgLength += Serialization::serialLenUShort();

      #ifdef BEEGFS_HSM_DEPRECATED
         msgLength += Serialization::serialLenUShort();  // hsmCollocationID
      #endif

         return msgLength;
      }

      unsigned getSupportedHeaderFeatureFlagsMask() const
      {
         return MKLOCALDIRMSG_FLAG_MIRRORED;
      }


   private:
      unsigned userID;
      unsigned groupID;
      int mode;
      uint16_t parentNodeID;
      uint16_t mirrorNodeID;
   
      // for serialization
      EntryInfo* entryInfoPtr; // not owned by this object!
      StripePattern* pattern;  // not owned by this object!

      // for deserialization
      EntryInfo entryInfo;
      StripePatternHeader patternHeader;
      const char* patternStart;

      // for HSM integration
      HsmCollocationID hsmCollocationID;

      // ACLs
      CharVector defaultACLXAttr;
      CharVector accessACLXAttr;


   public:

      /**
       * @return must be deleted by the caller; might be of type STRIPEPATTERN_Invalid
       */
      StripePattern* createPattern()
      {
         return StripePattern::createFromBuf(patternStart, patternHeader);
      }

      // getters & setters
      unsigned getUserID() const
      {
         return userID;
      }

      unsigned getGroupID() const
      {
         return groupID;
      }

      int getMode() const
      {
         return mode;
      }

      uint16_t getParentNodeID() const
      {
         return this->parentNodeID;
      }

      uint16_t getMirrorNodeID() const
      {
         return this->mirrorNodeID;
      }

      void setMirrorNodeID(uint16_t mirrorNodeID)
      {
         addMsgHeaderFeatureFlag(MKLOCALDIRMSG_FLAG_MIRRORED);

         this->mirrorNodeID = mirrorNodeID;
      }

      EntryInfo* getEntryInfo(void)
      {
         return &this->entryInfo;
      }

      HsmCollocationID getHsmCollocationID()
      {
         return this->hsmCollocationID;
      }

      void setHsmCollocationID(HsmCollocationID hsmCollocationID)
      {
         this->hsmCollocationID = hsmCollocationID;
      }

      const CharVector& getDefaultACLXAttr() const
      {
         return this->defaultACLXAttr;
      }

      const CharVector& getAccessACLXAttr() const
      {
         return this->accessACLXAttr;
      }
};

#endif /*MKLOCALDIRMSG_H_*/
