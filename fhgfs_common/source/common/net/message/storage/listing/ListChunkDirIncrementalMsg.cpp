#include "ListChunkDirIncrementalMsg.h"

bool ListChunkDirIncrementalMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   { // relativeDir
      unsigned relativeDirBufLen;

      if ( !Serialization::deserializeStrAlign4(&buf[bufPos], bufLen - bufPos, &relativeDir,
         &relativeDirBufLen) )
         return false;

      bufPos += relativeDirBufLen;
   }

   { // targetID
      unsigned targetIDBufLen;

      if ( !Serialization::deserializeUInt16(&buf[bufPos], bufLen - bufPos, &targetID,
         &targetIDBufLen) )
         return false;

      bufPos += targetIDBufLen;
   }

   {
      // isMirror
      unsigned isMirrorBufLen;

      if ( !Serialization::deserializeBool(&buf[bufPos], bufLen - bufPos, &isMirror,
         &isMirrorBufLen) )
         return false;

      bufPos += isMirror;
   }

   { // offset
      unsigned offsetBufLen;

      if ( !Serialization::deserializeInt64(&buf[bufPos], bufLen - bufPos, &offset, &offsetBufLen) )
         return false;

      bufPos += offsetBufLen;
   }

   { // maxOutEntries
      unsigned maxOutEntriesBufLen;

      if ( !Serialization::deserializeUInt(&buf[bufPos], bufLen - bufPos, &maxOutEntries,
         &maxOutEntriesBufLen) )
         return false;

      bufPos += maxOutEntriesBufLen;
   }

   {
      // onlyFiles
      unsigned onlyFilesBufLen;

      if ( !Serialization::deserializeBool(&buf[bufPos], bufLen - bufPos, &onlyFiles,
         &onlyFilesBufLen) )
         return false;

      bufPos += onlyFiles;
   }

   return true;
}

void ListChunkDirIncrementalMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;
   
   // relativeDir
   bufPos += Serialization::serializeStrAlign4(&buf[bufPos], relativeDir.length(),
      relativeDir.c_str());

   // targetID
   bufPos += Serialization::serializeUInt16(&buf[bufPos], targetID);

   // isMirror
   bufPos += Serialization::serializeBool(&buf[bufPos], isMirror);

   // offset
   bufPos += Serialization::serializeInt64(&buf[bufPos], offset);
   
   // maxOutEntries
   bufPos += Serialization::serializeUInt(&buf[bufPos], maxOutEntries);

   //onlyFiles
   bufPos += Serialization::serializeBool(&buf[bufPos], onlyFiles);
}

