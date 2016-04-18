#include "GamGetCollocationIDMetaRespMsg.h"

bool GamGetCollocationIDMetaRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   // chunkIDs
   if ( !Serialization::deserializeEntryInfoListPreprocess(&buf[bufPos], bufLen - bufPos,
      &entryInfoListElemNum, &entryInfoListStart, &entryInfoListLen) )
      return false;

   bufPos += entryInfoListLen;

   // collocationIDs
   if (! Serialization::deserializeUShortVectorPreprocess(&buf[bufPos], bufLen - bufPos,
      &collocationIDsElemNum, &collocationIDsStart, &collocationIDsLen) )
      return false;

   bufPos += collocationIDsLen;

   return true;
}

void GamGetCollocationIDMetaRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // chunkIDs
   bufPos += Serialization::serializeEntryInfoList(&buf[bufPos], entryInfoList);

   // collocationID
   bufPos += Serialization::serializeUShortVector(&buf[bufPos], collocationIDs);
}

TestingEqualsRes GamGetCollocationIDMetaRespMsg::testingEquals(NetMessage *msg)
{
   if ( msg->getMsgType() == this->getMsgType() )
   {
      GamGetCollocationIDMetaRespMsg* msgCast = (GamGetCollocationIDMetaRespMsg*) msg;
      EntryInfoList receivedEntryInfoList;
      UShortVector receivedCollocationIDs;

      msgCast->parseEntryInfoList(&receivedEntryInfoList);
      msgCast->parseCollocationIDs(&receivedCollocationIDs);

      // TODO
     /* if (*entryInfoList != receivedEntryInfoList)
         return TestingEqualsRes_FALSE; */
      if (*collocationIDs != receivedCollocationIDs )
         return TestingEqualsRes_FALSE;
   }
   else
      return TestingEqualsRes_FALSE;

   return TestingEqualsRes_TRUE;
}

