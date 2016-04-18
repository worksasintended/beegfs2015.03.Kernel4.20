#include "FsckModificationEventMsg.h"

bool FsckModificationEventMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   {
      // modificationEventTypeList
      if ( !Serialization::deserializeUInt8ListPreprocess(&buf[bufPos], bufLen - bufPos,
         &eventTypeListElemNum, &eventTypeListStart, &eventTypeListBufLen) )
         return false;

      bufPos += eventTypeListBufLen;
   }

   {
      // entryIDList
      if ( !Serialization::deserializeStringListPreprocess(&buf[bufPos], bufLen - bufPos,
         &entryIDListElemNum, &entryIDListStart, &entryIDListBufLen) )
         return false;

      bufPos += entryIDListBufLen;
   }

   {
      // eventsMissed
      unsigned eventsMissedBufLen;

      if ( !Serialization::deserializeBool(&buf[bufPos], bufLen - bufPos,
         &(this->eventsMissed), &eventsMissedBufLen) )
         return false;

      bufPos += eventsMissedBufLen;
   }

   { // ackID
      unsigned ackBufLen;

      if(!Serialization::deserializeStr(&buf[bufPos], bufLen-bufPos,
         &ackIDLen, &ackID, &ackBufLen) )
         return false;

      bufPos += ackBufLen;
   }

   return true;
}

void FsckModificationEventMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // modificationEventTypeList
   bufPos += Serialization::serializeUInt8List(&buf[bufPos], this->modificationEventTypeList);

   // entryIDList
   bufPos += Serialization::serializeStringList(&buf[bufPos], this->entryIDList);

   // eventsMissed
   bufPos += Serialization::serializeBool(&buf[bufPos], this->eventsMissed);

   // ackID
   bufPos += Serialization::serializeStr(&buf[bufPos], ackIDLen, ackID);
}
