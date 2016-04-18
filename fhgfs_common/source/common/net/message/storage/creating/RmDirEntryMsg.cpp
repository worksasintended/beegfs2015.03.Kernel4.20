#include "RmDirEntryMsg.h"

bool RmDirEntryMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   {  // entryInfo
      unsigned fieldLen;

      if(!this->parentInfo.deserialize(&buf[bufPos], bufLen-bufPos, &fieldLen) )
         return false;

      bufPos += fieldLen; // not needed right now. included to avoid human errors later ;)
   }

   {  // entryName
      unsigned fieldLen;

      if (!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos, &this->entryName,
         &fieldLen) )
         return false;

      bufPos += fieldLen; // not needed right now. included to avoid human errors later ;)
   }

   return true;
}

void RmDirEntryMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // entryInfoPtr
   bufPos += parentInfoPtr->serialize(&buf[bufPos]);

   // entryName
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos],
      this->entryName.length(), this->entryName.c_str() );

}

