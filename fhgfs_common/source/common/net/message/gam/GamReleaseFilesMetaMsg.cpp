#include "GamReleaseFilesMetaMsg.h"

bool GamReleaseFilesMetaMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   // entryInfoList
   if ( !Serialization::deserializeEntryInfoListPreprocess(&buf[bufPos], bufLen - bufPos,
      &entryInfoListElemNum, &entryInfoListStart, &entryInfoListLen) )
      return false;

   bufPos += entryInfoListLen;

   // urgency
   unsigned urgencyLen;

   if ( !Serialization::deserializeUInt8(&buf[bufPos], bufLen - bufPos, &urgency, &urgencyLen) )
      return false;

   bufPos += urgencyLen;

   return true;
}

void GamReleaseFilesMetaMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // chunkIDList
   bufPos += Serialization::serializeEntryInfoList(&buf[bufPos], entryInfoList);

   // urgency
   bufPos += Serialization::serializeUInt8(&buf[bufPos], urgency);
}

TestingEqualsRes GamReleaseFilesMetaMsg::testingEquals(NetMessage *msg)
{
   // TODO: finish check
   if ( msg->getMsgType() == this->getMsgType() )
   {
      GamReleaseFilesMetaMsg* msgCast = (GamReleaseFilesMetaMsg *) msg;
      if (urgency != msgCast->getUrgency())
         return TestingEqualsRes_FALSE;
   }
   else
      return TestingEqualsRes_FALSE;

   return TestingEqualsRes_TRUE;
}

