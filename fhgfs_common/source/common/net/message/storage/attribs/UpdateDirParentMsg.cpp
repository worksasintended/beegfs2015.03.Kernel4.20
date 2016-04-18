#include "UpdateDirParentMsg.h"

void UpdateDirParentMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // entryInfoPtr
   bufPos += this->entryInfoPtr->serialize(&buf[bufPos]);

   // parentOwnerNodeID
   bufPos += Serialization::serializeUInt16(&buf[bufPos], this->parentOwnerNodeID);
}

bool UpdateDirParentMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   { // entryInfo
      unsigned parentInfoBufLen;

      if ( !this->entryInfo.deserialize(&buf[bufPos], bufLen - bufPos, &parentInfoBufLen) )
         return false;

      bufPos += parentInfoBufLen;
   }

   { // parentOwnerNodeID
      unsigned fieldLen;

      if (!Serialization::deserializeUInt16(&buf[bufPos], bufLen - bufPos, &this->parentOwnerNodeID,
         &fieldLen) )
         return false;

      bufPos += fieldLen;
   }


   return true;
}


