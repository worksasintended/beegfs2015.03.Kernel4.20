#ifndef COMMON_SETLOCALATTRRESPMSG_H_
#define COMMON_SETLOCALATTRRESPMSG_H_

#include <common/net/message/SimpleIntMsg.h>
#include <common/storage/striping/DynamicFileAttribs.h>

#define SETLOCALATTRRESPMSG_COMPAT_FLAG_HAS_ATTRS 1 // message contains DynAttrs of chunk

class SetLocalAttrRespMsg : public NetMessage
{
   public:
      /*
       * @param result the result of the set attr operation
       * @param dynamicAttribs The current dynamic attributes of the chunk, which are needed by
       *                       the metadata server in case of a xtime change; just a reference!
       */
      SetLocalAttrRespMsg(FhgfsOpsErr result, DynamicFileAttribs& dynamicAttribs) :
         NetMessage(NETMSGTYPE_SetLocalAttrResp),
         result(result),
         filesize(dynamicAttribs.fileSize),
         numBlocks(dynamicAttribs.numBlocks),
         modificationTimeSecs(dynamicAttribs.modificationTimeSecs),
         lastAccessTimeSecs(dynamicAttribs.lastAccessTimeSecs),
         storageVersion(dynamicAttribs.storageVersion)
      {
         addMsgHeaderCompatFeatureFlag(SETLOCALATTRRESPMSG_COMPAT_FLAG_HAS_ATTRS);
      }

      SetLocalAttrRespMsg(FhgfsOpsErr result) :
                    NetMessage(NETMSGTYPE_SetLocalAttrResp), result(result), filesize(0),
                    numBlocks(0), modificationTimeSecs(0), lastAccessTimeSecs(0), storageVersion(0)
      {
         // all initialization done in list
      }

      SetLocalAttrRespMsg() : NetMessage(NETMSGTYPE_SetLocalAttrResp)
      {
      }

      // getters & setters
      FhgfsOpsErr getResult() const
      {
         return result;
      }

      void getDynamicAttribs(DynamicFileAttribs *outDynamicAttribs) const
      {
         outDynamicAttribs->fileSize = filesize;
         outDynamicAttribs->lastAccessTimeSecs = lastAccessTimeSecs;
         outDynamicAttribs->modificationTimeSecs = modificationTimeSecs;
         outDynamicAttribs->numBlocks = numBlocks;
         outDynamicAttribs->storageVersion = storageVersion;
      }

   protected:
      virtual void serializePayload(char* buf);
      virtual bool deserializePayload(const char* buf, size_t bufLen);

      unsigned calcMessageLength()
      {
         unsigned len = NETMSG_HEADER_LENGTH +
            Serialization::serialLenInt(); // result

         if (isMsgHeaderCompatFeatureFlagSet(SETLOCALATTRRESPMSG_COMPAT_FLAG_HAS_ATTRS))
         {
            len += Serialization::serialLenInt64()   // filesize
                +  Serialization::serialLenInt64()   // numBlocks
                +  Serialization::serialLenInt64()   // modificationTimeSecs
                +  Serialization::serialLenInt64()   // lastAccessTimeSecs
                +  Serialization::serialLenUInt64(); // storageVersion
         }

         return len;
      }

   private:
      FhgfsOpsErr result;
      int64_t filesize;
      int64_t numBlocks;
      int64_t modificationTimeSecs;
      int64_t lastAccessTimeSecs;
      uint64_t storageVersion;
};

#endif /*COMMON_SETLOCALATTRRESPMSG_H_*/
