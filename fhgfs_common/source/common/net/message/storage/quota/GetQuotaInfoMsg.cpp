#include "GetQuotaInfoMsg.h"

void GetQuotaInfoMsg::serializePayload(char* buf)
{
   size_t bufPos = 0;

   // queryType
   bufPos += Serialization::serializeInt(&buf[bufPos], this->queryType);

   // type
   bufPos += Serialization::serializeInt(&buf[bufPos], this->type);

   if(queryType == GetQuotaInfo_QUERY_TYPE_ID_RANGE)
   {
      // idRangeStart
      bufPos += Serialization::serializeUInt(&buf[bufPos], this->idRangeStart);

      // idRangeEnd
      bufPos += Serialization::serializeUInt(&buf[bufPos], this->idRangeEnd);
   }

   if(queryType == GetQuotaInfo_QUERY_TYPE_ID_LIST)
   {
      //idList
      bufPos += Serialization::serializeUIntList(&buf[bufPos], &this->idList);
   }

   if(queryType == GetQuotaInfo_QUERY_TYPE_SINGLE_ID)
   {
      // idRangeStart
      bufPos += Serialization::serializeUInt(&buf[bufPos], this->idRangeStart);
   }

   // targetSelection
   bufPos += Serialization::serializeUInt(&buf[bufPos], this->targetSelection);

   // targetNumID
   bufPos += Serialization::serializeUInt16(&buf[bufPos], this->targetNumID);
}

bool GetQuotaInfoMsg::deserializePayload(const char* buf, size_t bufLen)
{
   size_t bufPos = 0;

   // queryType

   unsigned queryTypeBufLen;

   if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos,
      &this->queryType, &queryTypeBufLen) )
      return false;

   bufPos += queryTypeBufLen;


   // type

   unsigned typeBufLen;

   if(!Serialization::deserializeInt(&buf[bufPos], bufLen-bufPos,
      &this->type, &typeBufLen) )
      return false;

   bufPos += typeBufLen;


   if(queryType == GetQuotaInfo_QUERY_TYPE_ID_RANGE)
   {
      // idRangeStart

      unsigned idRangeStartBufLen;

      if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
         &this->idRangeStart, &idRangeStartBufLen) )
         return false;

      bufPos += idRangeStartBufLen;


      // idRangeEnd

      unsigned idRangeEndBufLen;

      if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
         &this->idRangeEnd, &idRangeEndBufLen) )
         return false;

      bufPos += idRangeEndBufLen;
   }


   if(queryType == GetQuotaInfo_QUERY_TYPE_ID_LIST)
   {
      // idList

      if(!Serialization::deserializeUIntListPreprocess(&buf[bufPos], bufLen-bufPos,
         &this->idListElemNum, &this->idListListStart, &this->idListBufLen) )
         return false;

      bufPos += this->idListBufLen;
   }


   if(queryType == GetQuotaInfo_QUERY_TYPE_SINGLE_ID)
   {
      // idRangeStart

      unsigned idRangeStartBufLen;

      if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
         &this->idRangeStart, &idRangeStartBufLen) )
         return false;

      bufPos += idRangeStartBufLen;
   }


   // targetSelection

   unsigned targetSelectionBufLen;

   if(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos,
      &this->targetSelection, &targetSelectionBufLen) )
      return false;

   bufPos += targetSelectionBufLen;


   // targetNumID

   unsigned targetNumIDBufLen;

   if(!Serialization::deserializeUInt16(&buf[bufPos], bufLen-bufPos,
      &this->targetNumID, &targetNumIDBufLen) )
      return false;

   bufPos += targetNumIDBufLen;

   return true;
}
