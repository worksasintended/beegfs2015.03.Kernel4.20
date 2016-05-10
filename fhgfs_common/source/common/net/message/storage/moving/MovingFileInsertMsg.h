#ifndef MOVINGFILEINSERTMSG_H_
#define MOVINGFILEINSERTMSG_H_

#include <common/net/message/NetMessage.h>
#include <common/storage/Path.h>
#include <common/toolkit/MessagingTk.h>

#define MOVINGFILEINSERTMSG_FLAG_HAS_XATTRS 1

/**
 * Used to tell a remote metadata server a file is to be moved between directories
 */
class MovingFileInsertMsg : public NetMessage
{
   friend class AbstractNetMessageFactory;

   public:
      typedef FhgfsOpsErr (*NextXAttrFn)(void* context, std::string& name, CharVector& value);

      /**
       * @param path just a reference, so do not free it as long as you use this object!
       * @param serialBuf serialized file;
       * just a reference, so do not free it as long as you use this object!
       */
      MovingFileInsertMsg(EntryInfo* fromFileInfo, EntryInfo* toDirInfo, std::string& newName,
         const char* serialBuf, unsigned serialBufLen) :  NetMessage(NETMSGTYPE_MovingFileInsert)
      {
         this->fromFileInfoPtr   = fromFileInfo;
         this->newName           = newName;
         this->toDirInfoPtr      = toDirInfo;

         this->serialBuf         = serialBuf;
         this->serialBufLen      = serialBufLen;
      }


   protected:
      MovingFileInsertMsg() : NetMessage(NETMSGTYPE_MovingFileInsert)
      {
      }


      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);
      
      unsigned calcMessageLength()
      {
         return NETMSG_HEADER_LENGTH                                   +
            this->fromFileInfoPtr->serialLen()                         +
            this->toDirInfoPtr->serialLen()                            + // toDirInfo
            Serialization::serialLenStrAlign4(this->newName.length() ) + // newName
            Serialization::serialLenUInt()                             + // serialBufLen
            serialBufLen;                                                // serial file buffer
      }

      unsigned getSupportedHeaderFeatureFlagsMask() const
      {
         return MOVINGFILEINSERTMSG_FLAG_HAS_XATTRS;
      }

   private:
      EntryInfo  fromFileInfo; // for deserialization
      EntryInfo* fromFileInfoPtr; // for serialization

      std::string newName;

      const char* serialBuf;
      unsigned serialBufLen;

      EntryInfo* toDirInfoPtr; // for serialization
      EntryInfo  toDirInfo; // for deserialization

      NextXAttrFn nextAttr;
      void* nextAttrContext;

      static FhgfsOpsErr streamToSocketFn(Socket* socket, void* context)
      {
         MovingFileInsertMsg* msg = static_cast<MovingFileInsertMsg*>(context);

         std::string name;
         CharVector value;

         while (true)
         {
            FhgfsOpsErr getRes = msg->nextAttr(msg->nextAttrContext, name, value);
            if (getRes == FhgfsOpsErr_SUCCESS)
               break;
            else if (getRes != FhgfsOpsErr_AGAIN)
            {
               uint32_t endMark;
               HOST_TO_LITTLE_ENDIAN_32(-1, endMark);
               socket->send(&endMark, sizeof(endMark), 0);
               return getRes;
            }

            uint32_t nameLen;
            HOST_TO_LITTLE_ENDIAN_32(name.size(), nameLen);
            socket->send(&nameLen, sizeof(nameLen), 0);
            socket->send(&name[0], nameLen, 0);

            uint64_t valueLen;
            HOST_TO_LITTLE_ENDIAN_64(value.size(), valueLen);
            socket->send(&valueLen, sizeof(valueLen), 0);
            socket->send(&value[0], value.size(), 0);
         }

         uint32_t endMark;
         HOST_TO_LITTLE_ENDIAN_32(0, endMark);
         socket->send(&endMark, sizeof(endMark), 0);

         return FhgfsOpsErr_SUCCESS;
      }

   public:

      // getters & setters
      const char* getSerialBuf()
      {
         return serialBuf;
      }
      
      EntryInfo* getToDirInfo()
      {
         return &this->toDirInfo;
      }

      std::string getNewName()
      {
         return this->newName;
      }

      EntryInfo* getFromFileInfo()
      {
         return &this->fromFileInfo;
      }

      void registerStreamoutHook(RequestResponseArgs& rrArgs, NextXAttrFn fn, void* context)
      {
         addMsgHeaderFeatureFlag(MOVINGFILEINSERTMSG_FLAG_HAS_XATTRS);
         nextAttr = fn;
         nextAttrContext = context;
         rrArgs.sendExtraData = &streamToSocketFn;
         rrArgs.extraDataContext = this;
      }

      FhgfsOpsErr getNextXAttr(Socket* socket, std::string& name, CharVector& value)
      {
         if (!isMsgHeaderFeatureFlagSet(MOVINGFILEINSERTMSG_FLAG_HAS_XATTRS))
            return FhgfsOpsErr_SUCCESS;

         uint32_t nameLen;
         ssize_t nameLenRes = socket->recvExactT(&nameLen, sizeof(nameLen), 0, CONN_SHORT_TIMEOUT);
         if (nameLenRes < 0 || size_t(nameLenRes) < sizeof(nameLen))
            return FhgfsOpsErr_COMMUNICATION;

         LITTLE_ENDIAN_TO_HOST_32(nameLen, nameLen);
         if (nameLen == 0)
            return FhgfsOpsErr_SUCCESS;
         else if (nameLen == uint32_t(-1))
            return FhgfsOpsErr_COMMUNICATION;

         if (nameLen > XATTR_NAME_MAX)
            return FhgfsOpsErr_RANGE;

         name.resize(nameLen);
         if (socket->recvExactT(&name[0], nameLen, 0, CONN_SHORT_TIMEOUT) != (ssize_t) name.size())
            return FhgfsOpsErr_COMMUNICATION;

         uint64_t valueLen;
         ssize_t valueLenRes = socket->recvExactT(&valueLen, sizeof(valueLen), 0,
               CONN_SHORT_TIMEOUT);
         if (valueLenRes < 0 || size_t(valueLenRes) != sizeof(valueLen))
            return FhgfsOpsErr_COMMUNICATION;

         LITTLE_ENDIAN_TO_HOST_64(valueLen, valueLen);
         if (valueLen > XATTR_SIZE_MAX)
            return FhgfsOpsErr_RANGE;

         value.resize(valueLen);
         if (socket->recvExactT(&value[0], valueLen, 0, CONN_SHORT_TIMEOUT) != ssize_t(valueLen))
            return FhgfsOpsErr_COMMUNICATION;

         return FhgfsOpsErr_AGAIN;
      }
};

#endif /*MOVINGFILEINSERTMSG_H_*/
