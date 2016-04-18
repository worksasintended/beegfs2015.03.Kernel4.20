#include "StatRespMsg.h"


fhgfs_bool StatRespMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   StatRespMsg* thisCast = (StatRespMsg*)this;

   size_t bufPos = 0;
   
   { // result
      unsigned resultFieldLen;

      if(!Serialization_deserializeInt(&buf[bufPos], bufLen-bufPos, &thisCast->result,
         &resultFieldLen) )
         return fhgfs_false;

      bufPos += resultFieldLen;
   }

   { // statData
      unsigned statLen;

      if(!StatData_deserialize(&buf[bufPos], bufLen-bufPos, &thisCast->statData, &statLen) )
         return fhgfs_false;

      bufPos += statLen;
   }

   if(NetMessage_isMsgHeaderFeatureFlagSet(this, STATRESPMSG_FLAG_HAS_PARENTINFO) )
   {
      {  // parentEntryID
         unsigned fieldLen;
         unsigned strLen;

         if (!Serialization_deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
            &strLen, &thisCast->parentEntryID, &fieldLen) )
            return fhgfs_false;

         bufPos += fieldLen;
      }

      {
         // parentNodeID
         unsigned fieldLen;

         if (!Serialization_deserializeUShort(&buf[bufPos], bufLen-bufPos,
            &thisCast->parentNodeID, &fieldLen) )
            return fhgfs_false;

         bufPos += fieldLen;
      }
   }


   return fhgfs_true;
}

unsigned StatRespMsg_getSupportedHeaderFeatureFlagsMask(NetMessage* this)
{
   return STATRESPMSG_FLAG_HAS_PARENTINFO;
}
