#include <program/Program.h>
#include <common/threading/SafeMutexLock.h>
#include "SessionStore.h"

/**
 * @param session belongs to the store after calling this method - so do not free it and don't
 * use it any more afterwards (re-get it from this store if you need it)
 */
void SessionStore::addSession(Session* session)
{
   std::string sessionID = session->getSessionID();

   SafeMutexLock mutexLock(&mutex);
   
   // is session in the store already?
   
   SessionMapIter iter = sessions.find(sessionID);
   if(iter != sessions.end() )
   {
      delete(session);
   }
   else
   { // session not in the store yet
      sessions.insert(SessionMapVal(sessionID, new SessionReferencer(session) ) );
   }

   mutexLock.unlock();

}

/**
 * Note: remember to call releaseSession()
 * 
 * @return NULL if no such session exists
 */
Session* SessionStore::referenceSession(std::string sessionID, bool addIfNotExists)
{
   Session* session;
   
   SafeMutexLock mutexLock(&mutex);
   
   SessionMapIter iter = sessions.find(sessionID);
   if(iter == sessions.end() )
   { // not found
      if(!addIfNotExists)
         session = NULL;
      else
      { // add as new session and reference it
         LogContext log("SessionStore (ref)");
         log.log(Log_DEBUG, std::string("Creating a new session. SessionID: ") + sessionID);
         
         Session* newSession = new Session(sessionID);
         SessionReferencer* sessionRefer = new SessionReferencer(newSession);
         sessions.insert(SessionMapVal(sessionID, sessionRefer) );
         session = sessionRefer->reference();
      }
   }
   else
   {
      SessionReferencer* sessionRefer = iter->second;
      session = sessionRefer->reference();
   }
   
   mutexLock.unlock();
   
   return session;
}

void SessionStore::releaseSession(Session* session)
{
   std::string sessionID = session->getSessionID();

   SafeMutexLock mutexLock(&mutex);
   
   SessionMapIter iter = sessions.find(sessionID);
   if(iter != sessions.end() )
   { // session exists => decrease refCount
      SessionReferencer* sessionRefer = iter->second;
      sessionRefer->release();
   }
   
   mutexLock.unlock();
}

bool SessionStore::removeSession(std::string sessionID)
{
   bool delErr = true;
   
   SafeMutexLock mutexLock(&mutex);
   
   SessionMapIter iter = sessions.find(sessionID);
   if(iter != sessions.end() )
   {
      SessionReferencer* sessionRefer = iter->second;
      
      if(sessionRefer->getRefCount() )
         delErr = true;
      else
      { // no references => delete
         sessions.erase(sessionID);
         delete(sessionRefer);
         delErr = false;
      }
   }
   
   mutexLock.unlock();
   
   return !delErr;
}

/**
 * @return NULL if session is referenced, otherwise the sesion must be cleaned up by the caller
 */
Session* SessionStore::removeSessionUnlocked(std::string sessionID)
{
   Session* retVal = NULL;

   SessionMapIter iter = sessions.find(sessionID);
   if(iter != sessions.end() )
   {
      SessionReferencer* sessionRefer = iter->second;
      
      if(!sessionRefer->getRefCount() )
      { // no references => return session to caller
         retVal = sessionRefer->getReferencedObject();
         
         sessionRefer->setOwnReferencedObject(false);
         delete(sessionRefer);
         sessions.erase(sessionID);
      }
   }
   
   return retVal;
}

/**
 * @param masterList must be ordered; contained nodes will be removed and may no longer be
 * accessed after calling this method.
 * @param outRemovedSessions contained sessions must be cleaned up by the caller
 * @param outUnremovableSesssions contains sessions that would have been removed but are currently
 * referenced
 */
void SessionStore::syncSessions(NodeList* masterList, SessionList* outRemovedSessions,
   StringList* outUnremovableSesssions)
{
   SafeMutexLock mutexLock(&mutex);
   
   SessionMapIter sessionIter = sessions.begin();
   NodeListIter masterIter = masterList->begin();
   
   while( (sessionIter != sessions.end() ) && (masterIter != masterList->end() ) )
   {
      std::string currentSession(sessionIter->first);
      std::string currentMaster( (*masterIter)->getID() );

      if(currentMaster < currentSession)
      { // session is added
         // note: we add sessions only on demand, so we just delete the node object here
         delete(*masterIter);
         
         masterList->pop_front();
         masterIter = masterList->begin();
      }
      else
      if(currentSession < currentMaster)
      { // session is removed
         sessionIter++; // (removal invalidates iterator)

         Session* session = removeSessionUnlocked(currentSession);
         if(session)
            outRemovedSessions->push_back(session);
         else
            outUnremovableSesssions->push_back(currentSession); // session was referenced
      }
      else
      { // session unchanged
         delete(*masterIter);

         masterList->pop_front();
         masterIter = masterList->begin();

         sessionIter++;
      }
   }
   
   // remaining masterList entries are new
   while(masterIter != masterList->end() )
   {
      // note: we add sessions only on demand, so we just delete the node object here
      delete(*masterIter);
      
      masterList->pop_front();
      masterIter = masterList->begin();
   }

   // remaining sessions are removed
   while(sessionIter != sessions.end() )
   {
      std::string currentSession(sessionIter->first);
      sessionIter++; // (removal invalidates iterator)

      Session* session = removeSessionUnlocked(currentSession);
      if(session)
         outRemovedSessions->push_back(session);
      else
         outUnremovableSesssions->push_back(currentSession); // session was referenced
   }
   
   
   mutexLock.unlock();
}

/**
 * @return number of sessions
 */
size_t SessionStore::getAllSessionIDs(StringList* outSessionIDs)
{
   SafeMutexLock mutexLock(&mutex);

   size_t retVal = sessions.size();

   for(SessionMapIter iter = sessions.begin(); iter != sessions.end(); iter++)
      outSessionIDs->push_back(iter->first);

   mutexLock.unlock();

   return retVal;
}

size_t SessionStore::getSize()
{
   SafeMutexLock mutexLock(&mutex);
   
   size_t sessionsSize = sessions.size();

   mutexLock.unlock();

   return sessionsSize;
}

unsigned SessionStore::serialize(char* buf)
{
   unsigned elementCount = this->sessions.size();

   size_t bufPos = 0;

   // elem count info field
   bufPos += Serialization::serializeUInt(&buf[bufPos], elementCount);

   // serialize each element
   SessionMapIter iter = this->sessions.begin();

   for ( unsigned i = 0; i < elementCount; i++, iter++ )
   {
      bufPos += Serialization::serializeStr(&buf[bufPos], iter->first.length(),
         iter->first.c_str()); // serialize the key
      bufPos += iter->second->getReferencedObject()->serialize(&buf[bufPos]);
   }

   LOG_DEBUG("SessionStore serialize", Log_DEBUG, "count of serialized Sessions: " +
      StringTk::uintToStr(elementCount));

   return bufPos;
}

bool SessionStore::deserialize(const char* buf, size_t bufLen, unsigned* outLen)
{
   size_t bufPos = 0;
   unsigned elemNumField = 0;

   {
      // elem count info field
      unsigned elemNumFieldLen;

      if (!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &elemNumField,
         &elemNumFieldLen) )
         return false;

      bufPos += elemNumFieldLen;
   }


   {
      // sessions
      for(unsigned i = 0; i < elemNumField; i++)
      {
         //session ID (key)
         unsigned keyLen = 0;
         const char* key = NULL;
         unsigned keyBufLen = 0;

         if(!Serialization::deserializeStr(&buf[bufPos], bufLen-bufPos, &keyLen, &key, &keyBufLen) )
            return false;

         bufPos += keyBufLen;

         // SessionLocalFileReferencer (value)
         Session* session = new Session();

         unsigned sessionLen;

         if (!session->deserialize(&buf[bufPos], bufLen-bufPos, &sessionLen) )
         {
            session->getFiles()->deleteAllSessions();

            delete(session);
            return false;
         }

         SessionMapIter searchResult = this->sessions.find(std::string(key));
         if (searchResult == this->sessions.end() )
         {
            this->sessions.insert(SessionMapVal(std::string(key), new SessionReferencer(session)));
         }
         else
         { // exist so local files will merged
            searchResult->second->getReferencedObject()->mergeSessionFiles(session);
            delete(session);
         }

         bufPos += sessionLen;
      }
   }

   *outLen = bufPos;

   LOG_DEBUG("SessionStore deserialize", Log_DEBUG, "count of deserialized Sessions: " +
         StringTk::uintToStr(elemNumField));

   return true;
}

unsigned SessionStore::serialLen()
{
   unsigned sessionStoreSize = this->sessions.size();

   size_t len = 0;

   len += Serialization::serialLenUInt(); // elem count

   SessionMapIter iter = this->sessions.begin();

   for ( unsigned i = 0; i < sessionStoreSize; i++, iter++)
   {
      len += Serialization::serialLenStr(iter->first.length() );                    // key
      len += iter->second->getReferencedObject()->serialLen();
   }

   return len;
}

bool SessionStore::loadFromFile(const std::string& filePath)
{
   LogContext log("SessionStore (load)");
   log.log(Log_DEBUG,"load sessions from file: " + filePath);

   bool retVal = false;
   char* buf = NULL;
   int readRes;

   struct stat statBuf;
   int retValStat;

   if(!filePath.length() )
      return false;

   SafeMutexLock mutexLock(&mutex);

   int fd = open(filePath.c_str(), O_RDONLY, 0);
   if(fd == -1)
   { // open failed
      log.log(Log_DEBUG, "Unable to open session file: " + filePath + ". " +
         "SysErr: " + System::getErrString() );

      goto err_unlock;
   }

   retValStat = fstat(fd, &statBuf);
   if(retValStat)
   { // stat failed
      log.log(Log_WARNING, "Unable to stat session file: " + filePath + ". " +
         "SysErr: " + System::getErrString() );

      goto err_stat;
   }

   buf = (char*)malloc(statBuf.st_size);
   readRes = read(fd, buf, statBuf.st_size);
   if(readRes <= 0)
   { // reading failed
      log.log(Log_WARNING, "Unable to read session file: " + filePath + ". " +
         "SysErr: " + System::getErrString() );
   }
   else
   { // parse contents
      unsigned outLen;
      retVal = deserialize(buf, readRes, &outLen);
   }

   if (retVal)
      retVal = relinkInodes(*Program::getApp()->getMetaStore());

   if (!retVal)
      log.logErr("Could not deserialize SessionStore from file: " + filePath);

   free(buf);

err_stat:
   close(fd);

err_unlock:
   mutexLock.unlock();

   return retVal;
}

/**
 * Note: setStorePath must be called before using this.
 */
bool SessionStore::saveToFile(const std::string& filePath)
{
   LogContext log("SessionStore (save)");
   log.log(Log_DEBUG,"save sessions to file: " + filePath);

   bool retVal = false;

   char* buf;
   unsigned bufLen;
   ssize_t writeRes;

   if(!filePath.length() )
      return false;

   SafeMutexLock mutexLock(&mutex); // L O C K

   // create/trunc file
   int openFlags = O_CREAT|O_TRUNC|O_WRONLY;

   int fd = open(filePath.c_str(), openFlags, 0666);
   if(fd == -1)
   { // error
      log.logErr("Unable to create session file: " + filePath + ". " +
         "SysErr: " + System::getErrString() );

      goto err_unlock;
   }

   // file created => store data
   buf = (char*)malloc(serialLen() );
   bufLen = serialize(buf);
   writeRes = write(fd, buf, bufLen);
   free(buf);

   if(writeRes != (ssize_t)bufLen)
   {
      log.logErr("Unable to store session file: " + filePath + ". " +
         "SysErr: " + System::getErrString() );

      goto err_closefile;
   }

   retVal = true;

   LOG_DEBUG_CONTEXT(log, Log_DEBUG, "Session file stored: " + filePath);

   // error compensation
err_closefile:
   close(fd);

err_unlock:
   mutexLock.unlock(); // U N L O C K

   return retVal;
}

bool sessionStoreMetaEquals(SessionStore& first, SessionStore& second)
{
   SessionMapIter firstIter = first.sessions.begin();
   SessionMapIter secondIter = second.sessions.begin();

   for( ; (firstIter != first.sessions.end() ) && (secondIter != second.sessions.end() );
      firstIter++, secondIter++ )
   {
      if( (*firstIter).first != (*secondIter).first )
         return false;

      if(!sessionMetaEquals( (*firstIter).second->getReferencedObject(),
         (*secondIter).second->getReferencedObject() ) )
         return false;
   }

   return true;
}
