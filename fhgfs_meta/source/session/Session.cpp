#include "Session.h"

/* Merges the SessionFiles of the given session into this session, only not existing
 * SessionFiles will be added to the existing session
 * @param session the session which will be merged with this session
 */
void Session::mergeSessionFiles(Session* session)
{
   this->getFiles()->mergeSessionFiles(session->getFiles() );
}

unsigned Session::serialize(char* buf)
{
   size_t bufPos = 0;

   // sessionID
   bufPos += Serialization::serializeStr(&buf[bufPos], this->sessionID.length(),
      this->sessionID.c_str());

   // files
   bufPos += this->files.serialize(&buf[bufPos]);

   return bufPos;
}

bool Session::deserialize(const char* buf, size_t bufLen, unsigned* outLen)
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

   // files
   unsigned filesLen;

   if (!this->files.deserialize(&buf[bufPos], bufLen-bufPos, &filesLen) )
   {
      log.logErr("Could not deserialize SessionFileStore for session: " + sessionID);
      return false;
   }

   bufPos += filesLen;

   *outLen = bufPos;

   return true;
}

unsigned Session::serialLen()
{
   size_t len = 0;

   len += Serialization::serialLenStr(this->sessionID.length() );    // sessionID
   len += this->files.serialLen();                                   // files

   return len;
}

bool sessionMetaEquals(Session* first, Session* second, bool disableInodeCheck)
{
   if(!first || !second)
      return false;

   // sessionID;
   if(first->sessionID != second->sessionID)
      return false;

   // files;
   if(!sessionFileStoreEquals(first->files, second->files, disableInodeCheck) )
      return false;

   return true;
}
