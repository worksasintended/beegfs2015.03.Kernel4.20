#include "GamReleaseFilesMetaRespMsg.h"

bool GamReleaseFilesMetaRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   // entryInfoList
   if ( !Serialization::deserializeEntryInfoListPreprocess(&buf[bufPos], bufLen - bufPos,
      &entryInfoListElemNum, &entryInfoListStart, &entryInfoListLen) )
      return false;

   bufPos += entryInfoListLen;

   return true;
}

void GamReleaseFilesMetaRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // chunkIDList
   bufPos += Serialization::serializeEntryInfoList(&buf[bufPos], entryInfoList);
}

TestingEqualsRes GamReleaseFilesMetaRespMsg::testingEquals(NetMessage *msg)
{
   // TODO: finish check
   /*
   if ( msg->getMsgType() == this->getMsgType() )
   {
      GamReleaseFilesMetaRespMsg* msgCast = (GamReleaseFilesMetaRespMsg *) msg;
   }
   else
      return TestingEqualsRes_FALSE; */

   return TestingEqualsRes_TRUE;
}

