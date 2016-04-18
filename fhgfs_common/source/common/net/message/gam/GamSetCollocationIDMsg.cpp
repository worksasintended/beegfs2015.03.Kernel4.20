#include "GamSetCollocationIDMsg.h"

bool GamSetCollocationIDMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   // dirEntryIDs
   if ( !Serialization::deserializeStringVecPreprocess(&buf[bufPos], bufLen - bufPos,
      &dirEntryIDsElemNum, &dirEntryIDsStart, &dirEntryIDsLen) )
      return false;

   bufPos += dirEntryIDsLen;

   // collocationIDs
   if (! Serialization::deserializeUShortVectorPreprocess(&buf[bufPos], bufLen - bufPos,
      &collocationIDsElemNum, &collocationIDsStart, &collocationIDsLen) )
      return false;

   bufPos += collocationIDsLen;

   return true;
}

void GamSetCollocationIDMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // dirEntryIDs
   bufPos += Serialization::serializeStringVec(&buf[bufPos], dirEntryIDs);

   // collocationID
   bufPos += Serialization::serializeUShortVector(&buf[bufPos], collocationIDs);
}

TestingEqualsRes GamSetCollocationIDMsg::testingEquals(NetMessage *msg)
{
   if ( msg->getMsgType() == this->getMsgType() )
   {
      GamSetCollocationIDMsg* msgCast = (GamSetCollocationIDMsg *) msg;
      StringVector receivedDirEntryIDs;
      UShortVector receivedCollocationIDs;

      msgCast->parseDirEntryIDs(&receivedDirEntryIDs);
      msgCast->parseCollocationIDs(&receivedCollocationIDs);

      if (*dirEntryIDs != receivedDirEntryIDs)
         return TestingEqualsRes_FALSE;
      if (*collocationIDs != receivedCollocationIDs )
         return TestingEqualsRes_FALSE;
   }
   else
      return TestingEqualsRes_FALSE;

   return TestingEqualsRes_TRUE;
}

