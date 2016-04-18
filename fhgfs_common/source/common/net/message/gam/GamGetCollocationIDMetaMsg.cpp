#include "GamGetCollocationIDMetaMsg.h"

bool GamGetCollocationIDMetaMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   // dirEntryIDs
   if ( !Serialization::deserializeEntryInfoListPreprocess(&buf[bufPos], bufLen - bufPos,
      &entryInfoListElemNum, &entryInfoListStart, &entryInfoListLen) )
      return false;

   bufPos += entryInfoListLen;

   return true;
}

void GamGetCollocationIDMetaMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // dirEntryIDs
   bufPos += Serialization::serializeEntryInfoList(&buf[bufPos], entryInfoList);
}

TestingEqualsRes GamGetCollocationIDMetaMsg::testingEquals(NetMessage *msg)
{
   // TODO
/*   if ( msg->getMsgType() == this->getMsgType() )
   {
      GamGetCollocationIDMetaMsg* msgCast = (GamGetCollocationIDMetaMsg*) msg;
   }
   else
      return TestingEqualsRes_FALSE; */

   return TestingEqualsRes_TRUE;
}

