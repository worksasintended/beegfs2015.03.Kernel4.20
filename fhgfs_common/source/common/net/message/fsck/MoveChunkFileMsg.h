#ifndef MOVECHUNKFILESMSG_H
#define MOVECHUNKFILESMSG_H

#include <common/net/message/NetMessage.h>

class MoveChunkFileMsg: public NetMessage
{
   public:
      /*
       * @param chunkName the name of the chunk (i.e. the "ID")
       * @param targetID
       * @param mirroredFromTargetID
       * @param oldPath relative to storeStorageDirectory and mirror chunk Path
       * @param newPath relative to storeStorageDirectory and mirror chunk Path
       * @param overwriteExisting if true, the destination file will be overwritten if it exists
       */
      MoveChunkFileMsg(std::string& chunkName, uint16_t targetID, uint16_t mirroredFromTargetID,
         std::string& oldPath, std::string& newPath, bool overwriteExisting=false):
         NetMessage(NETMSGTYPE_MoveChunkFile)
      {
         this->chunkName = chunkName.c_str();
         this->chunkNameLen = strlen(this->chunkName);
         this->oldPath = oldPath.c_str();
         this->oldPathLen = strlen(this->oldPath);
         this->newPath = newPath.c_str();
         this->newPathLen = strlen(this->newPath);

         this->targetID = targetID;
         this->mirroredFromTargetID = mirroredFromTargetID;

         this->overwriteExisting = overwriteExisting;
      }

      MoveChunkFileMsg() : NetMessage(NETMSGTYPE_MoveChunkFile)
      {
      }

      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH +
            Serialization::serialLenStrAlign4(chunkNameLen) + // chunkName
            Serialization::serialLenStrAlign4(oldPathLen) + // oldPath
            Serialization::serialLenStrAlign4(newPathLen) + // newPath
            Serialization::serialLenUInt16() + //targetID
            Serialization::serialLenUInt16() + //mirroredFromTargetID
            Serialization::serialLenBool(); // overwriteExisting
      }

   private:
      const char* chunkName;
      unsigned chunkNameLen;
      const char* oldPath;
      unsigned oldPathLen;
      const char* newPath;
      unsigned newPathLen;

      uint16_t targetID;
      uint16_t mirroredFromTargetID;

      bool overwriteExisting;

   public:
      std::string getChunkName() const
      {
         return chunkName;
      }

      uint16_t getTargetID() const
      {
         return targetID;
      }

      uint16_t getMirroredFromTargetID() const
      {
         return mirroredFromTargetID;
      }

      std::string getOldPath() const
      {
         return oldPath;
      }

      std::string getNewPath() const
      {
         return newPath;
      }

      bool getOverwriteExisting() const
      {
         return overwriteExisting;
      }

    // inliner
    virtual TestingEqualsRes testingEquals(NetMessage* msg)
    {
       MoveChunkFileMsg* msgIn = (MoveChunkFileMsg*) msg;

       if ( this->chunkName != msgIn->getChunkName() )
          return TestingEqualsRes_FALSE;

       if ( this->targetID != msgIn->getTargetID() )
          return TestingEqualsRes_FALSE;

       if ( this->mirroredFromTargetID != msgIn->getMirroredFromTargetID() )
          return TestingEqualsRes_FALSE;

       if ( this->oldPath != msgIn->getOldPath() )
          return TestingEqualsRes_FALSE;

       if ( this->newPath != msgIn->getNewPath() )
          return TestingEqualsRes_FALSE;

       if ( this->overwriteExisting != msgIn->getOverwriteExisting() )
          return TestingEqualsRes_FALSE;

       return TestingEqualsRes_TRUE;
    }

};

#endif /*MOVECHUNKFILESMSG_H*/
