#include "SimpleIntStringMsg.h"

void SimpleIntStringMsg_serializePayload(NetMessage* this, char* buf)
{
   SimpleIntStringMsg* thisCast = (SimpleIntStringMsg*)this;

   size_t bufPos = 0;

   bufPos += Serialization_serializeInt(&buf[bufPos], thisCast->intValue);

   bufPos += Serialization_serializeStr(&buf[bufPos], thisCast->strValueLen, thisCast->strValue);
}

fhgfs_bool SimpleIntStringMsg_deserializePayload(NetMessage* this, const char* buf, size_t bufLen)
{
   SimpleIntStringMsg* thisCast = (SimpleIntStringMsg*)this;

   size_t bufPos = 0;

   { // intValue
      fhgfs_bool deserRes;
      unsigned deserLen;

      deserRes = Serialization_deserializeInt(&buf[bufPos], bufLen-bufPos, &thisCast->intValue,
         &deserLen);
      if(unlikely(!deserRes) )
         return fhgfs_false;

      bufPos += deserLen;
   }

   { // strValue
      fhgfs_bool deserRes;
      unsigned deserLen;

      deserRes = Serialization_deserializeStr(&buf[bufPos], bufLen-bufPos, &thisCast->strValueLen,
         &thisCast->strValue, &deserLen);
      if(unlikely(!deserRes) )
         return fhgfs_false;

      bufPos += deserLen;
   }

   return fhgfs_true;
}

unsigned SimpleIntStringMsg_calcMessageLength(NetMessage* this)
{
   SimpleIntStringMsg* thisCast = (SimpleIntStringMsg*)this;

   return NETMSG_HEADER_LENGTH +
      Serialization_serialLenInt() + // intValue
      Serialization_serialLenStr(thisCast->strValueLen); // strValue
}




