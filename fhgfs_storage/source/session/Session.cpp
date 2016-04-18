#include <common/toolkit/serialization/Serialization.h>
#include "Session.h"


/* Merges the SessionLocalFiles of the given session into this session, only not existing
 * SessionLocalFiles will be added to the existing session
 * @param session the session which will be merged with this session
 */
void Session::mergeSessionLocalFiles(Session* session)
{
   this->getLocalFiles()->mergeSessionLocalFiles(session->getLocalFiles());
}

unsigned Session::serializeForTarget(char* buf, uint16_t targetID)
{
   size_t bufPos = 0;

   // sessionID
   bufPos += Serialization::serializeStr(&buf[bufPos], this->sessionID.length(),
      this->sessionID.c_str());

   // localFiles
   bufPos += this->localFiles.serializeForTarget(&buf[bufPos], targetID);

   return bufPos;
}

bool Session::deserializeForTarget(const char* buf, size_t bufLen, unsigned* outLen,
   uint16_t targetID)
{
   LogContext log("Session (deserialize)");

   unsigned bufPos = 0;

   // sessionID
   unsigned sessionIDBufLen;
   const char* sessionIDChar;
   unsigned sessionIDLen;

   if(!Serialization::deserializeStr(&buf[bufPos], bufLen-bufPos,
      &sessionIDLen, &sessionIDChar, &sessionIDBufLen) )
      return false;

   this->sessionID.assign(sessionIDChar, sessionIDLen);
   bufPos += sessionIDBufLen;

   // localFiles
   unsigned localFilesLen;

   if (!this->localFiles.deserializeForTarget(&buf[bufPos], bufLen-bufPos, &localFilesLen,
      targetID))
   {
      log.logErr("Could not deserialize SessionLocalFileStore for session: " + sessionID);
      return false;
   }

   bufPos += localFilesLen;
   *outLen = bufPos;;

   return true;
}

unsigned Session::serialLenForTarget(uint16_t targetID)
{
   size_t bufPos = 0;

   bufPos += Serialization::serialLenStr(this->sessionID.length());     // sessionID
   bufPos += this->localFiles.serialLenForTarget(targetID);             // localFiles

   return bufPos;
}
