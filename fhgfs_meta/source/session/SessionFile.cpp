#include <program/Program.h>
#include <storage/MetaStore.h>

#include "SessionFile.h"



unsigned SessionFile::serialize(char* buf)
{
   size_t bufPos = 0;

   // accessFlags
   bufPos += Serialization::serializeUInt(&buf[bufPos], this->accessFlags);

   // sessionID
   bufPos += Serialization::serializeUInt(&buf[bufPos], this->sessionID);

   // entryInfo
   bufPos += this->entryInfo.serialize(&buf[bufPos]);

   // useAsyncCleanup
   bufPos += Serialization::serializeBool(&buf[bufPos], this->useAsyncCleanup);

   return bufPos;
}

bool SessionFile::deserialize(const char* buf, size_t bufLen, unsigned* outLen)
{
   size_t bufPos = 0;

   {
      // accessFlags
      unsigned accessFlagsLen;

      if (!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &this->accessFlags,
         &accessFlagsLen) )
         return false;

      bufPos += accessFlagsLen;
   }

   {
      // sessionID
      unsigned sessionIDLen;

      if (!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &this->sessionID,
         &sessionIDLen) )
         return false;

      bufPos += sessionIDLen;
   }

   {
      // entryInfo
      unsigned entryInfoLen;

      if (!entryInfo.deserialize(&buf[bufPos], bufLen-bufPos, &entryInfoLen) )
         return false;

      bufPos += entryInfoLen;
   }

   {
      // useAsyncCleanup
      unsigned useAsyncCleanupLen;

      if (!Serialization::deserializeBool(&buf[bufPos], bufLen-bufPos, &this->useAsyncCleanup,
         &useAsyncCleanupLen) )
         return false;

      bufPos += useAsyncCleanupLen;
   }

   *outLen = bufPos;

   return true;
}

unsigned SessionFile::serialLen()
{
   unsigned len = 0;

   len += Serialization::serialLenUInt();             // accessFlags
   len += Serialization::serialLenUInt();             // sessionID
   len += this->entryInfo.serialLen();                // entryInfo
   len += Serialization::serialLenBool();             // useAsyncCleanup

   return len;
}

bool sessionFileEquals(const SessionFile* first, const SessionFile* second, bool disableInodeCheck)
{
   if(!first || !second)
      return false;

   // accessFlags;
   if(first->accessFlags != second->accessFlags)
      return false;

   // sessionID;
   if(first->sessionID != second->sessionID)
      return false;

   // entryInfo;
   if(!first->entryInfo.compare(&(second->entryInfo) ) )
      return false;

   // useAsyncCleanup;
   if(first->useAsyncCleanup != second->useAsyncCleanup)
      return false;

   if(!disableInodeCheck)
   {
      // inode;
      if(!fileInodeEquals(first->inode, second->inode) )
         return false;
   }

   return true;
}

bool SessionFile::relinkInode(MetaStore& store)
{
   int openRes = store.openFile(&entryInfo, accessFlags, &inode);

   if (openRes == FhgfsOpsErr_SUCCESS)
      return true;

   Program::getApp()->getLogger()->logErr(__func__, "Could not relink session for inode "
         + entryInfo.getParentEntryID() + "/" + entryInfo.getEntryID());
   return false;
}
