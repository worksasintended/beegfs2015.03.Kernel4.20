#include "GamGetChunkCollocationIDMsg.h"

bool GamGetChunkCollocationIDMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   { // targetID
      unsigned targetIDBufLen;
      if ( !Serialization::deserializeUShort(&buf[bufPos], bufLen - bufPos, &(this->targetID),
         &targetIDBufLen) )
         return false;

      bufPos += targetIDBufLen;
   }

   { // dirEntryIDs
      if ( !Serialization::deserializeStringVecPreprocess(&buf[bufPos], bufLen - bufPos,
         &chunkIDsElemNum, &chunkIDsStart, &chunkIDsLen) )
         return false;

      bufPos += chunkIDsLen;
   }

   return true;
}

void GamGetChunkCollocationIDMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // targetID
   bufPos += Serialization::serializeUShort(&buf[bufPos], targetID);

   // dirEntryIDs
   bufPos += Serialization::serializeStringVec(&buf[bufPos], chunkIDs);
}

TestingEqualsRes GamGetChunkCollocationIDMsg::testingEquals(NetMessage *msg)
{
   if ( msg->getMsgType() == this->getMsgType() )
   {
      GamGetChunkCollocationIDMsg* msgCast = (GamGetChunkCollocationIDMsg *) msg;
      StringVector receivedChunkIDs;
      msgCast->parseChunkIDs(&receivedChunkIDs);
      if (getTargetID() != msgCast->getTargetID())
         return TestingEqualsRes_FALSE;
      if (*chunkIDs != receivedChunkIDs)
         return TestingEqualsRes_FALSE;
   }
   else
      return TestingEqualsRes_FALSE;

   return TestingEqualsRes_TRUE;
}

