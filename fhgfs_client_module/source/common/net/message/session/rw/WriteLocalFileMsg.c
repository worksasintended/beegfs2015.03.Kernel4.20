#include "WriteLocalFileMsg.h"


void WriteLocalFileMsg_serializePayload(NetMessage* this, char* buf)
{
   WriteLocalFileMsg* thisCast = (WriteLocalFileMsg*)this;

   size_t bufPos = 0;
   
   // offset
   bufPos += Serialization_serializeInt64(&buf[bufPos], thisCast->offset);

   // count
   bufPos += Serialization_serializeInt64(&buf[bufPos], thisCast->count);
   
   // accessFlags
   bufPos += Serialization_serializeUInt(&buf[bufPos], thisCast->accessFlags);

   if(NetMessage_isMsgHeaderFeatureFlagSet(this, WRITELOCALFILEMSG_FLAG_USE_QUOTA))
   {
      // userID
      bufPos += Serialization_serializeUInt(&buf[bufPos], thisCast->userID);

      // groupID
      bufPos += Serialization_serializeUInt(&buf[bufPos], thisCast->groupID);
   }

   // fileHandleID
   bufPos += Serialization_serializeStrAlign4(&buf[bufPos], thisCast->fileHandleIDLen,
      thisCast->fileHandleID);

   // sessionID
   bufPos += Serialization_serializeStrAlign4(&buf[bufPos], thisCast->sessionIDLen,
      thisCast->sessionID);

   // pathInfo
   bufPos += PathInfo_serialize(thisCast->pathInfo, &buf[bufPos]);

   // targetID
   bufPos += Serialization_serializeUShort(&buf[bufPos], thisCast->targetID);
}

unsigned WriteLocalFileMsg_calcMessageLength(NetMessage* this)
{
   WriteLocalFileMsg* thisCast = (WriteLocalFileMsg*)this;

   unsigned retVal = 0;

   retVal += NETMSG_HEADER_LENGTH;
   retVal += Serialization_serialLenInt64();                    // offset
   retVal += Serialization_serialLenInt64();                    // count
   retVal += Serialization_serialLenUInt();                     // accessFlags

   if(NetMessage_isMsgHeaderFeatureFlagSet(this, WRITELOCALFILEMSG_FLAG_USE_QUOTA) )
   {
      retVal += Serialization_serialLenUInt(); // userID
      retVal += Serialization_serialLenUInt(); // groupID
   }

   retVal += Serialization_serialLenStrAlign4(thisCast->fileHandleIDLen);
   retVal += Serialization_serialLenStrAlign4(thisCast->sessionIDLen);
   retVal += PathInfo_serialLen(thisCast->pathInfo);              // pathInfo
   retVal += Serialization_serialLenUShort();                    // targetID

   return retVal;
}


