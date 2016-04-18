#include "GamRecalledFilesMsg.h"

bool GamRecalledFilesMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   { // targetID
      unsigned targetIDBufLen;
      if ( !Serialization::deserializeUShort(&buf[bufPos], bufLen - bufPos, &(this->targetID),
         &targetIDBufLen) )
         return false;

      bufPos += targetIDBufLen;
   }

   { // chunkIDVec
      if ( !Serialization::deserializeStringVecPreprocess(&buf[bufPos], bufLen - bufPos,
         &chunkIDVecElemNum, &chunkIDVecStart, &chunkIDVecLen) )
         return false;

      bufPos += chunkIDVecLen;
   }
   return true;
}

void GamRecalledFilesMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // targetID
   bufPos += Serialization::serializeUShort(&buf[bufPos], targetID);

   // chunkIDList
   bufPos += Serialization::serializeStringVec(&buf[bufPos], chunkIDVec);
}

TestingEqualsRes GamRecalledFilesMsg::testingEquals(NetMessage *msg)
{
   if ( msg->getMsgType() == this->getMsgType() )
   {
      GamRecalledFilesMsg* msgCast = (GamRecalledFilesMsg *) msg;
      StringVector receivedChunkIDVec;
      msgCast->parseChunkIDVec(&receivedChunkIDVec);
      if (getTargetID() != msgCast->getTargetID())
         return TestingEqualsRes_FALSE;
      if (*chunkIDVec == receivedChunkIDVec)
         return TestingEqualsRes_FALSE;
   }
   else
      return TestingEqualsRes_FALSE;

   return TestingEqualsRes_TRUE;
}

