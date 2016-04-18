#ifndef MKDIRMSG_H_
#define MKDIRMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/EntryInfo.h>


#define MKDIRMSG_FLAG_NOMIRROR   1 /* do not use mirror setting from parent (i.e. do not mirror) */
#define MKDIRMSG_FLAG_UMASK      2 /* message contains separate umask data */


class MkDirMsg : public NetMessage
{
   friend class AbstractNetMessageFactory;
   
   public:
      
      /**
       * @param parentEntryInfo just a reference, so do not free it as long as you use this object!
       * @param preferredNodes just a reference, so do not free it as long as you use this object!
       */
      MkDirMsg(EntryInfo* parentEntryInfo, std::string& newDirName,
         unsigned userID, unsigned groupID, int mode, int umask,
         UInt16List* preferredNodes) : NetMessage(NETMSGTYPE_MkDir)
      {
         this->parentEntryInfoPtr = parentEntryInfo;
         this->newDirName = newDirName.c_str();
         this->newDirNameLen = newDirName.length();
         this->userID = userID;
         this->groupID = groupID;
         this->mode = mode;
         this->umask = umask;
         this->preferredNodes = preferredNodes;

         if (umask != -1)
            addMsgHeaderFeatureFlag(MKDIRMSG_FLAG_UMASK);
      }

      /**
       * For deserialization only!
       */
      MkDirMsg() : NetMessage(NETMSGTYPE_MkDir) {}

      TestingEqualsRes testingEquals(NetMessage *msg);

   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         const unsigned umaskLen =
            isMsgHeaderFeatureFlagSet(MKDIRMSG_FLAG_UMASK) ? Serialization::serialLenInt() : 0;

         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenUInt() + // userID
            Serialization::serialLenUInt() + // groupID
            Serialization::serialLenInt() + // mode
            umaskLen + // optional umask
            this->parentEntryInfoPtr->serialLen() +
            Serialization::serialLenStrAlign4(this->newDirNameLen) +
            Serialization::serialLenUInt16List(preferredNodes); // preferredNodes
      }

      unsigned getSupportedHeaderFeatureFlagsMask() const
      {
         return MKDIRMSG_FLAG_NOMIRROR | MKDIRMSG_FLAG_UMASK;
      }

      void setMode(int mode, int umask)
      {
         this->mode = mode;
         this->umask = umask;

         if (umask != -1)
            addMsgHeaderFeatureFlag(MKDIRMSG_FLAG_UMASK);
         else
            unsetMsgHeaderFeatureFlag(MKDIRMSG_FLAG_UMASK);
      }

   private:
      
      unsigned userID;
      unsigned groupID;
      int mode;
      int umask;

      // serialization / deserialization
      const char* newDirName;
      unsigned newDirNameLen;

      // for serialization
      EntryInfo* parentEntryInfoPtr; // not owned by this object
      UInt16List* preferredNodes; // not owned by this object!

      // for deserialization
      EntryInfo parentEntryInfo;
      unsigned prefNodesElemNum;
      const char* prefNodesListStart;
      unsigned prefNodesBufLen;

      
   public:

      void parsePreferredNodes(UInt16List* outNodes)
      {
         Serialization::deserializeUInt16List(
            prefNodesBufLen, prefNodesElemNum, prefNodesListStart, outNodes);
      }

      // getters & setters
      unsigned getUserID()
      {
         return userID;
      }
      
      unsigned getGroupID()
      {
         return groupID;
      }
      
      int getMode()
      {
         return mode;
      }

      int getUmask()
      {
         return umask;
      }

      EntryInfo* getParentInfo(void)
      {
         return &this->parentEntryInfo;
      }

      const char* getNewDirName(void)
      {
         return this->newDirName;
      }

};

#endif /*MKDIRMSG_H_*/
