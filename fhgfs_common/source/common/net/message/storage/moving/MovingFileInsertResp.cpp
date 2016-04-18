#include "MovingFileInsertRespMsg.h"

void MovingFileInsertRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // result
   bufPos += Serialization::serializeInt(&buf[bufPos], this->result);
   
   // inodeBufLen
   bufPos += Serialization::serializeUInt(&buf[bufPos], this->inodeBufLen);
   
   // inodeBuf
   if (this->inodeBufLen)
   {
      memcpy(&buf[bufPos], inodeBuf, inodeBufLen);
      bufPos += inodeBufLen;
   }
}

bool MovingFileInsertRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   { // result
      unsigned resultFieldLen;
      if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos, &this->result,
         &resultFieldLen) )
         return false;

      bufPos += resultFieldLen;
   }
   
   { // inodeBufLen
      unsigned serialBufFieldLen;
      if (!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
         &this->inodeBufLen, &serialBufFieldLen) )
         return false;

      bufPos += serialBufFieldLen;
   }

   // inodeBuf
   if(!this->inodeBufLen)
      this->inodeBuf = NULL;
   else
   { // we got an inode
      this->inodeBuf = (char *) &buf[bufPos];

      bufPos += this->inodeBufLen;
   }

   return true;
}


