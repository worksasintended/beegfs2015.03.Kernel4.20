#ifndef SESSIONLOCALFILESTORE_H_
#define SESSIONLOCALFILESTORE_H_

#include <common/toolkit/ObjectReferencer.h>
#include <common/threading/Mutex.h>
#include <common/Common.h>
#include "SessionLocalFile.h"



typedef std::list<SessionLocalFile*> SessionLocalFileList;
typedef SessionLocalFileList::iterator SessionLocalFileListIter;
typedef ObjectReferencer<SessionLocalFile*> SessionLocalFileReferencer;
typedef std::map<std::string, SessionLocalFileReferencer*> SessionLocalFileMap;
typedef SessionLocalFileMap::iterator SessionLocalFileMapIter;
typedef SessionLocalFileMap::value_type SessionLocalFileMapVal;


class SessionLocalFileStore
{
   public:
      SessionLocalFileStore() {}

      SessionLocalFile* addAndReferenceSession(SessionLocalFile* session);
      SessionLocalFile* referenceSession(std::string fileHandleID, uint16_t targetID,
         bool isMirrorSession);
      void releaseSession(SessionLocalFile* session);
      bool removeSession(std::string fileHandleID, uint16_t targetID, bool isMirrorSession,
         SessionLocalFile** outSessionLocalFile);
      void removeAllSessions(SessionLocalFileList* outRemovedSessions,
         StringList* outReferencedSessions);
      void removeAllMirrorSessions(uint16_t targetID);
      void deleteAllSessions();
      void mergeSessionLocalFiles(SessionLocalFileStore*
         sessionLocalFileStore);

      size_t getSize();

      unsigned serializeForTarget(char* buf, uint16_t targetID);
      bool deserializeForTarget(const char* buf, size_t bufLen, unsigned* outLen,
         uint16_t targetID);
      unsigned serialLenForTarget(uint16_t targetID);

   protected:
      SessionLocalFileMap* getSessionMap();

   private:
      SessionLocalFileMap sessions; // key: <fileHandleID><...>@<targetID>, value: <session_ref>

      Mutex mutex;

      std::string generateMapKey(const std::string fileHandleID, uint16_t targetID,
         bool isMirrorSession);

      uint16_t getTargetIDFromKey(std::string key);
};

#endif /*SESSIONLOCALFILESTORE_H_*/
