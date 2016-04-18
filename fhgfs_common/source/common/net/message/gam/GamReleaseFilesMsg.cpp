#include "GamReleaseFilesMsg.h"

bool GamReleaseFilesMsg::deserializePayload(const char* buf, size_t bufLen)
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

   { // urgency
      unsigned urgencyLen;

      if ( !Serialization::deserializeUInt8(&buf[bufPos], bufLen - bufPos, &(this->urgency),
         &urgencyLen) )
         return false;

      bufPos += urgencyLen;
   }

   return true;
}

void GamReleaseFilesMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // targetID
   bufPos += Serialization::serializeUShort(&buf[bufPos], this->targetID);

   // chunkIDList
   bufPos += Serialization::serializeStringVec(&buf[bufPos], this->chunkIDVec);

   // urgency
   bufPos += Serialization::serializeUInt8(&buf[bufPos], this->urgency);
}

TestingEqualsRes GamReleaseFilesMsg::testingEquals(NetMessage *msg)
{
   if ( msg->getMsgType() == this->getMsgType() )
   {
      GamReleaseFilesMsg* msgCast = (GamReleaseFilesMsg *) msg;
      StringVector receivedChunkIDVec;
      msgCast->parseChunkIDVec(&receivedChunkIDVec);

      if (getTargetID() != msgCast->getTargetID())
         return TestingEqualsRes_FALSE;

      if (*chunkIDVec == receivedChunkIDVec)
         return TestingEqualsRes_FALSE;

      if (urgency != msgCast->getUrgency())
         return TestingEqualsRes_FALSE;
   }
   else
      return TestingEqualsRes_FALSE;

   return TestingEqualsRes_TRUE;
}

