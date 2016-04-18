#include "GamRecalledFilesMetaMsg.h"

bool GamRecalledFilesMetaMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   // entryInfoList
   if ( !Serialization::deserializeEntryInfoListPreprocess(&buf[bufPos], bufLen - bufPos,
      &entryInfoListElemNum, &entryInfoListStart, &entryInfoListLen) )
      return false;

   bufPos += entryInfoListLen;

   return true;
}

void GamRecalledFilesMetaMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // chunkIDList
   bufPos += Serialization::serializeEntryInfoList(&buf[bufPos], entryInfoList);
}

TestingEqualsRes GamRecalledFilesMetaMsg::testingEquals(NetMessage *msg)
{
   // TODO: finish check
/*   if ( msg->getMsgType() == this->getMsgType() )
   {
      GamRecalledFilesMetaMsg* msgCast = (GamRecalledFilesMetaMsg *) msg;
   }
   else
      return TestingEqualsRes_FALSE; */

   return TestingEqualsRes_TRUE;
}

