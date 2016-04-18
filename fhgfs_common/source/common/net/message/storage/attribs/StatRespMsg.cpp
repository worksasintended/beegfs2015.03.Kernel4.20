#include "StatRespMsg.h"



void StatRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // result
   bufPos += Serialization::serializeInt(&buf[bufPos], result);

   // statData
   bufPos += this->statData.serialize(false, true, true, &buf[bufPos]);

   if(isMsgHeaderFeatureFlagSet(STATRESPMSG_FLAG_HAS_PARENTINFO) )
   {
      // parentEntryID
      bufPos += Serialization::serializeStrAlign4(&buf[bufPos],
         this->parentEntryID.length(), this->parentEntryID.c_str() );

      // parentNodeID
      bufPos += Serialization::serializeUInt16(&buf[bufPos], this->parentNodeID);
   }
}

bool StatRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   { // result
      unsigned resultFieldLen;
      if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos,
         &this->result, &resultFieldLen) )
         return false;

      bufPos += resultFieldLen;
   }


   { // statData
      unsigned statLen;

      if(!this->statData.deserialize(false, true, true, &buf[bufPos], bufLen-bufPos, &statLen) )
         return false;

      bufPos += statLen;
   }

   if(isMsgHeaderFeatureFlagSet(STATRESPMSG_FLAG_HAS_PARENTINFO) )
   {
      {  // parentEntryID
         unsigned fieldLen;

         if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
            &this->parentEntryID, &fieldLen) )
            return false;

         bufPos += fieldLen;
      }

      {  // parentNodeID
         unsigned fieldLen;

         if (!Serialization::deserializeUInt16(&buf[bufPos], bufLen-bufPos,
            &this->parentNodeID, &fieldLen) )
            return false;

         bufPos += fieldLen;
      }

   }

   return true;
}


