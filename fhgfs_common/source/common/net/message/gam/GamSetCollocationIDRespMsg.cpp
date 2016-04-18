#include "GamSetCollocationIDRespMsg.h"

bool GamSetCollocationIDRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   // dirEntryIDs
   if ( !Serialization::deserializeStringVecPreprocess(&buf[bufPos], bufLen - bufPos,
      &dirEntryIDsElemNum, &dirEntryIDsStart, &dirEntryIDsLen) )
      return false;

   bufPos += dirEntryIDsLen;

   return true;
}

void GamSetCollocationIDRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // dirEntryIDs
   bufPos += Serialization::serializeStringVec(&buf[bufPos], dirEntryIDs);
}

TestingEqualsRes GamSetCollocationIDRespMsg::testingEquals(NetMessage *msg)
{
   if ( msg->getMsgType() == this->getMsgType() )
   {
      GamSetCollocationIDRespMsg* msgCast = (GamSetCollocationIDRespMsg *) msg;
      StringVector receivedDirEntryIDs;
      msgCast->parseFailedDirEntryIDs(&receivedDirEntryIDs);
      if (*dirEntryIDs != receivedDirEntryIDs)
         return TestingEqualsRes_FALSE;
   }
   else
      return TestingEqualsRes_FALSE;

   return TestingEqualsRes_TRUE;
}

