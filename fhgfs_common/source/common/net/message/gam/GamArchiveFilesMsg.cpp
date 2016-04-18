#include "GamArchiveFilesMsg.h"

bool GamArchiveFilesMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   // chunkIDs
   if ( !Serialization::deserializeStringVecPreprocess(&buf[bufPos], bufLen - bufPos,
      &chunkIDsElemNum, &chunkIDsStart, &chunkIDsLen) )
      return false;

   bufPos += chunkIDsLen;

   // collocationIDs
   if ( !Serialization::deserializeUShortVectorPreprocess(&buf[bufPos], bufLen - bufPos,
      &collocationIDsElemNum, &collocationIDsStart, &collocationIDsLen) )
      return false;

   bufPos += collocationIDsLen;

   return true;
}

void GamArchiveFilesMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // chunkIDs
   bufPos += Serialization::serializeStringVec(&buf[bufPos], chunkIDs);

   // collocationID
   bufPos += Serialization::serializeUShortVector(&buf[bufPos], collocationIDs);
}

TestingEqualsRes GamArchiveFilesMsg::testingEquals(NetMessage *msg)
{
   if ( msg->getMsgType() == this->getMsgType() )
   {
      GamArchiveFilesMsg* msgCast = (GamArchiveFilesMsg *) msg;
      StringVector receivedChunkIDs;
      UShortVector receivedCollocationIDs;

      msgCast->parseChunkIDs(&receivedChunkIDs);
      msgCast->parseCollocationIDs(&receivedCollocationIDs);

      if (*chunkIDs != receivedChunkIDs)
         return TestingEqualsRes_FALSE;
      if (*collocationIDs != receivedCollocationIDs )
         return TestingEqualsRes_FALSE;
   }
   else
      return TestingEqualsRes_FALSE;

   return TestingEqualsRes_TRUE;
}

