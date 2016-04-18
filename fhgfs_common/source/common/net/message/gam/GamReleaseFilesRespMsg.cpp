#include "GamReleaseFilesRespMsg.h"

bool GamReleaseFilesRespMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;
   
   // chunkIDVec
   if ( !Serialization::deserializeStringVecPreprocess(&buf[bufPos], bufLen - bufPos,
      &chunkIDVecElemNum, &chunkIDVecStart, &chunkIDVecLen) )
      return false;

   bufPos += chunkIDVecLen;

   return true;
}

void GamReleaseFilesRespMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // chunkIDList
   bufPos += Serialization::serializeStringVec(&buf[bufPos], chunkIDVec);
}

TestingEqualsRes GamReleaseFilesRespMsg::testingEquals(NetMessage *msg)
{
   // TODO: write comparison
   /*
   if ( msg->getMsgType() == this->getMsgType() )
   {
      GamReleaseFilesRespMsg* msgCast = (GamReleaseFilesRespMsg *) msg;
      StringList receivedChunkIDVec;
      msgCast->parseChunkIDVec(&receivedChunkIDVec);
      if (*chunkIDVec == receivedChunkIDVec)

         return TestingEqualsRes_FALSE;
   }
   else
      return TestingEqualsRes_FALSE; */

   return TestingEqualsRes_TRUE;
}

