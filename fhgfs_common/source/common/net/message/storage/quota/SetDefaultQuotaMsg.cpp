#include "SetDefaultQuotaMsg.h"



void SetDefaultQuotaMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // type
   bufPos += Serialization::serializeInt(&buf[bufPos], type);

   // size
   bufPos += Serialization::serializeUInt64(&buf[bufPos], size);

   //inodes
   bufPos += Serialization::serializeUInt64(&buf[bufPos], inodes);

}

bool SetDefaultQuotaMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   // type

   unsigned typeBufLen;

   if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos, &type, &typeBufLen) )
      return false;

   bufPos += typeBufLen;


   // size

   unsigned sizeBufLen;

   if(!Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos, &size, &sizeBufLen) )
      return false;

   bufPos += sizeBufLen;


   // inodes

   unsigned inodesBufLen;

   if(!Serialization::deserializeUInt64(&buf[bufPos], bufLen-bufPos, &inodes, &inodesBufLen) )
      return false;

   bufPos += inodesBufLen;

   return true;
}

