#ifndef SERIALIZATION_H_
#define SERIALIZATION_H_

#include <common/Common.h>
#include <common/net/sock/NetworkInterfaceCard.h>
#include <common/nodes/Node.h>
#include <common/storage/EntryInfo.h>
#include <common/storage/PathInfo.h>
#include <common/storage/quota/QuotaData.h>
#include <common/storage/StorageTargetInfo.h>
#include <common/toolkit/HighResolutionStats.h>

#include "Byteswap.h"


#define SERIALIZATION_NICLISTELEM_NAME_SIZE  (16)
#define SERIALIZATION_NICLISTELEM_SIZE       (8+SERIALIZATION_NICLISTELEM_NAME_SIZE) /*
                                              8 = 4b ipAddr + 1b nicType + 3b alignment padding */
#define SERIALIZATION_CHUNKINFOLISTELEM_ID_SIZE (96)
#define SERIALIZATION_CHUNKINFOLISTELEM_PATHSTR_SIZE (255)
#define SERIALIZATION_FILEINFOLISTELEM_OWNERNODE_SIZE (255)


struct PathDeserializationInfo
{
   unsigned elemsBufLen;
   unsigned elemNum;
   const char* elemListStart;
   bool isAbsolute;
};

// forward declaration
class Path;

class Serialization
{
   public:
      // string (de)serialization
      static unsigned serializeStr(char* buf, unsigned strLen, const char* strStart);
      static bool deserializeStr(const char* buf, size_t bufLen,
         unsigned* outStrLen, const char** outStrStart, unsigned* outBufLen);
      static bool deserializeStr(const char* buf, size_t bufLen,
         std::string* outStr, unsigned* outBufLen);
      static unsigned serialLenStr(unsigned strLen);

      // string aligned (de)serialization
      static unsigned serializeStrAlign4(char* buf, unsigned strLen, const char* strStart);
      static bool deserializeStrAlign4(const char* buf, size_t bufLen,
         unsigned* outStrLen, const char** outStrStart, unsigned* outBufLen);
      static bool deserializeStrAlign4(const char* buf, size_t bufLen,
         std::string* outStr, unsigned* outBufLen);
      static unsigned serialLenStrAlign4(unsigned strLen);

      // nicList (de)serialization
      static unsigned serializeNicList(char* buf, NicAddressList* nicList);
      static bool deserializeNicListPreprocess(const char* buf, size_t bufLen,
         unsigned* outNicListElemNum, const char** outNicListStart, unsigned* outListBufLen);
      static bool deserializeNicListPreprocess2012(const char* buf, size_t bufLen,
         unsigned* outNicListElemNum, const char** outNicListStart, unsigned* outListBufLen);
      static void deserializeNicList(unsigned nicListElemNum, const char* nicListStart,
         NicAddressList* outNicList);
      static void deserializeNicList2012(unsigned nicListElemNum, const char* nicListStart,
         NicAddressList* outNicList);
      static unsigned serialLenNicList(NicAddressList* nicList);

      // highResStatsList (de)serialization
      static unsigned serializeHighResStatsList(char* buf, HighResStatsList* list);
      static bool deserializeHighResStatsListPreprocess(const char* buf, size_t bufLen,
         unsigned* outListElemNum, const char** outListStart, unsigned* outLen);
      static void deserializeHighResStatsList(unsigned listElemNum, const char* listStart,
         HighResStatsList* outList);
      static unsigned serialLenHighResStatsList(HighResStatsList* list);

      // nodeList (de)serialization
      static unsigned serializeNodeList(char* buf, NodeList* nodeList);
      static bool deserializeNodeListPreprocess(const char* buf, size_t bufLen,
         unsigned* outNodeListElemNum, const char** outNodeListStart, unsigned* outListBufLen);
      static bool deserializeNodeListPreprocess2012(const char* buf, size_t bufLen,
         unsigned* outNodeListElemNum, const char** outNodeListStart, unsigned* outListBufLen);
      static void deserializeNodeList(unsigned nodeListElemNum, const char* nodeListStart,
         NodeList* outNodeList);
      static void deserializeNodeList2012(unsigned nodeListElemNum, const char* nodeListStart,
         NodeList* outNodeList);
      static unsigned serialLenNodeList(NodeList* nodeList);

      // stringList (de)serialization
      static unsigned serializeStringList(char* buf, const StringList* list);
      static bool deserializeStringListPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outListStart, unsigned* outListBufLen);
      static bool deserializeStringList(unsigned listBufLen, unsigned elemNum,
         const char* listStart, StringList* outList);
      static unsigned serialLenStringList(const StringList* list);

      // StringVector (de)serialization
      static unsigned serializeStringVec(char* buf, StringVector* vec);
      static bool deserializeStringVecPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outListStart, unsigned* outVecBufLen);
      static bool deserializeStringVec(unsigned listBufLen, unsigned elemNum,
         const char* listStart, StringVector* outVec);
      static unsigned serialLenStringVec(StringVector* vec);

      // stringSet (de)serialization
      static unsigned serializeStringSet(char* buf, const StringSet* set);
      static bool deserializeStringSetPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outSetStart, unsigned* outSetBufLen);
      static bool deserializeStringSet(unsigned setBufLen, unsigned elemNum,
         const char* setStart, StringSet* outSet);
      static unsigned serialLenStringSet(const StringSet* set);

      // UIntList (de)serialization
      static unsigned serializeUIntList(char* buf, UIntList* list);
      static bool deserializeUIntListPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outListStart, unsigned* outListBufLen);
      static bool deserializeUIntList(unsigned listBufLen, unsigned elemNum,
         const char* listStart, UIntList* outList);
      static unsigned serialLenUIntList(UIntList* list);
      static unsigned serialLenUIntList(size_t size);

      // UInt8List (/ushort) (de)serialization
      static unsigned serializeUInt8List(char* buf, UInt8List* list);
      static bool deserializeUInt8ListPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outListStart, unsigned* outListBufLen);
      static bool deserializeUInt8List(unsigned listBufLen, unsigned elemNum,
         const char* listStart, UInt8List* outList);
      static unsigned serialLenUInt8List(UInt8List* list);
      static unsigned serialLenUInt8List(size_t size);

      // UInt16List (/ushort) (de)serialization
      static unsigned serializeUInt16List(char* buf, UInt16List* list);
      static bool deserializeUInt16ListPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outListStart, unsigned* outListBufLen);
      static bool deserializeUInt16List(unsigned listBufLen, unsigned elemNum,
         const char* listStart, UInt16List* outList);
      static unsigned serialLenUInt16List(UInt16List* list);
      static unsigned serialLenUInt16List(size_t size);
      // UShortVector (de)serialization
      static unsigned serializeUShortVector(char* buf, UShortVector* vec);
      static bool deserializeUShortVectorPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outVecStart, unsigned* outVecBufLen);
      static bool deserializeUShortVector(unsigned vecBufLen, unsigned elemNum,
         const char* outVecStart, UShortVector* outVec);
      static unsigned serialLenUShortVector(UShortVector* vec);

      // CharVector (de)serialization
      static unsigned serializeCharVector(char* buf, CharVector* vec);
      static bool deserializeCharVectorPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outVecStart, unsigned* outVecBufLen);
      static bool deserializeCharVector(unsigned vecBufLen, unsigned elemNum,
         const char* outVecStart, CharVector* outVec);
      static unsigned serialLenCharVector(CharVector* vec);



      // UIntVector (de)serialization
      static unsigned serializeUIntVector(char* buf, UIntVector* vec);
      static bool deserializeUIntVectorPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outListStart, unsigned* outVecBufLen);
      static bool deserializeUIntVector(unsigned vecBufLen, unsigned elemNum,
         const char* outVecStart, UIntVector* outVec);
      static unsigned serialLenUIntVector(UIntVector* vec);
      static unsigned serialLenUIntVector(size_t size);

      // UInt16Vector (/ushort) (de)serialization
      static unsigned serializeUInt16Vector(char* buf, UInt16Vector* vec);
      static bool deserializeUInt16VectorPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outListStart, unsigned* outVecBufLen);
      static bool deserializeUInt16Vector(unsigned vecBufLen, unsigned elemNum,
         const char* outVecStart, UInt16Vector* outVec);
      static unsigned serialLenUInt16Vector(UInt16Vector* vec);
      static unsigned serialLenUInt16Vector(size_t size);

      // IntList (de)serialization
      static unsigned serializeIntList(char* buf, IntList* list);
      static bool deserializeIntListPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outListStart, unsigned* outListBufLen);
      static bool deserializeIntList(unsigned listBufLen, unsigned elemNum,
         const char* listStart, IntList* outList);
      static unsigned serialLenIntList(IntList* list);

      // IntVector (de)serialization
      static unsigned serializeIntVector(char* buf, IntVector* vec);
      static bool deserializeIntVectorPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outVecStart, unsigned* outVecBufLen);
      static bool deserializeIntVector(unsigned vecBufLen, unsigned elemNum,
         const char* outVecStart, IntVector* outVec);
      static unsigned serialLenIntVector(IntVector* vec);

      // Int64List (de)serialization
      static unsigned serializeInt64List(char* buf, Int64List* list);
      static bool deserializeInt64ListPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outListStart, unsigned* outListBufLen);
      static bool deserializeInt64List(unsigned listBufLen, unsigned elemNum,
         const char* listStart, Int64List* outList);
      static unsigned serialLenInt64List(Int64List* list);
      static unsigned serialLenInt64List(size_t size);

      // Int64Vector (de)serialization
      static unsigned serializeInt64Vector(char* buf, Int64Vector* vec);
      static bool deserializeInt64VectorPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outVecStart, unsigned* outVecBufLen);
      static bool deserializeInt64Vector(unsigned vecBufLen, unsigned elemNum,
         const char* outVecStart, Int64Vector* outVec);
      static unsigned serialLenInt64Vector(Int64Vector* vec);

      // UInt64List (de)serialization
      static unsigned serializeUInt64List(char* buf, const UInt64List* list);
      static bool deserializeUInt64ListPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outListStart, unsigned* outListBufLen);
      static bool deserializeUInt64List(unsigned listBufLen, unsigned elemNum,
         const char* listStart, UInt64List* outList);
      static unsigned serialLenUInt64List(const UInt64List* list);
      static unsigned serialLenUInt64List(size_t size);

      // UInt64Vector (de)serialization
      static unsigned serializeUInt64Vector(char* buf, const UInt64Vector* vec);
      static bool deserializeUInt64VectorPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outListStart, unsigned* outVecBufLen);
      static bool deserializeUInt64Vector(unsigned vecBufLen, unsigned elemNum,
         const char* outVecStart, UInt64Vector* outVec);
      static unsigned serialLenUInt64Vector(const UInt64Vector* vec);
      static unsigned serialLenUInt64Vector(size_t size);

      // BoolList (de)serialization
      static unsigned serializeBoolList(char* buf, BoolList* list);
      static bool deserializeBoolListPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outListStart, unsigned* outListBufLen);
      static bool deserializeBoolList(unsigned listBufLen, unsigned elemNum,
         const char* listStart, BoolList* outList);
      static unsigned serialLenBoolList(BoolList* list);
      static unsigned serialLenBoolList(size_t size);

      // path (de)serialization
      static unsigned serializePath(char* buf, Path* path);
      static bool deserializePathPreprocess(const char* buf, size_t bufLen,
         struct PathDeserializationInfo* outInfo, unsigned* outBufLen);
      static bool deserializePath(struct PathDeserializationInfo& info, Path* outPath);
      static unsigned serialLenPath(Path* path);

      // storageTargetInfo list (de)serialization
      static unsigned serializeStorageTargetInfoList(char* buf,
         StorageTargetInfoList *storageTargetInfoList);
      static unsigned serialLenStorageTargetInfoList(StorageTargetInfoList* storageTargetInfoList);
      static bool deserializeStorageTargetInfoListPreprocess(const char* buf, size_t bufLen,
         const char** outInfoStart, unsigned *outElemNum, unsigned* outLen);
      static bool deserializeStorageTargetInfoList(unsigned storageTargetInfoListElemNum,
         const char* storageTargetInfoListStart, StorageTargetInfoList* outList);

      // EntryInfoList
      static unsigned serializeEntryInfoList(char* buf, EntryInfoList* entryInfoList);
      static unsigned serialLenEntryInfoList(EntryInfoList* entryInfoList);
      static bool deserializeEntryInfoListPreprocess(const char* buf, size_t bufLen,
         unsigned *outElemNum, const char** outInfoStart, unsigned* outLen);
      static void deserializeEntryInfoList(unsigned entryInfoListLen,
         unsigned entryInfoListElemNum, const char* entryInfoListStart,
         EntryInfoList *outEntryInfoList);

      // PathInfoList
      static unsigned serializePathInfoList(char* buf, PathInfoList* pathInfoList);
      static unsigned serialLenPathInfoList(PathInfoList* pathInfoList);
      static bool deserializePathInfoListPreprocess(const char* buf, size_t bufLen,
         unsigned *outElemNum, const char** outInfoStart, unsigned* outLen);
      static void deserializePathInfoList(unsigned pathInfoListLen,
         unsigned pathInfoListElemNum, const char* pathInfoListStart,
         PathInfoList *outPathInfoList);

      // QuotaDataList
      static unsigned serializeQuotaDataList(char* buf, QuotaDataList* quotaDataList);
      static bool deserializeQuotaDataListPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outListStart, unsigned* outListBufLen);
      static void deserializeQuotaDataList(unsigned listBufLen, unsigned elemNum,
         const char* listStart, QuotaDataList* outList);
      static unsigned serialLenQuotaDataList(QuotaDataList* quotaDataList);

      // QuotaDataMap
      static unsigned serializeQuotaDataMap(char* buf, QuotaDataMap* quotaDataMap);
      static bool deserializeQuotaDataMapPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outMapStart, unsigned* outMapBufLen);
      static bool deserializeQuotaDataMap(unsigned mapBufLen, unsigned elemNum,
         const char* mapStart, QuotaDataMap* outQuotaDataMap);
      static unsigned serialLenQuotaDataMap(QuotaDataMap* quotaDataMap);

      // QuotaDataMapForTarget
      static unsigned serializeQuotaDataMapForTarget(char* buf,
         QuotaDataMapForTarget* quotaDataMapForTarget);
      static bool deserializeQuotaDataMapForTargetPreprocess(const char* buf, size_t bufLen,
         unsigned* outElemNum, const char** outMapStart, unsigned* outMapBufLen);
      static bool deserializeQuotaDataMapForTarget(unsigned mapBufLen, unsigned elemNum,
         const char* mapStart, QuotaDataMapForTarget* outQuotaDataMapForTarget);
      static unsigned serialLenQuotaDataMapForTarget(QuotaDataMapForTarget* quotaDataMap);

      // TargetStateInfoMap
      static unsigned serializeTargetStateInfoMap(char* buf,
            TargetStateInfoMap* targetStateInfoMap);
      static unsigned serialLenTargetStateInfoMap(
            TargetStateInfoMap* targetStateInfoMap);
      static bool deserializeTargetStateInfoMapPreprocess(const char* buf,
            size_t bufLen, const char** outInfoStart, unsigned *outElemNum, unsigned* outLen);
      static bool deserializeTargetStateInfoMap(unsigned targetStateInfoMapElemNum,
            const char* targetStateInfoMapStart, TargetStateInfoMap* outMap);
      static unsigned serializeTargetStateInfoMapElement(char* buf,
            const std::pair<uint16_t, TargetStateInfo>& value);
      static bool deserializeTargetStateInfoMapElement(const char* buf,
            size_t bufLen, std::pair<uint16_t, TargetStateInfo>* outValue, unsigned* outLen);
      static unsigned serialLenTargetStateInfoMapElem();

   private:
      Serialization() {}


   public:
      // inliners

      // char (de)serialization
      static unsigned serializeChar(char* buf, char value)
      {
         *buf = value;

         return serialLenChar();
      }

      static bool deserializeChar(const char* buf, size_t bufLen,
         char* outValue, unsigned* outLen)
      {
         if(unlikely(bufLen < serialLenChar() ) )
            return false;

         *outLen = serialLenChar();
         *outValue = *buf;

         return true;
      }

      static unsigned serialLenChar()
      {
         return sizeof(char);
      }

      // bool (de)serialization
      static unsigned serializeBool(char* buf, bool value)
      {
         return serializeChar(buf, value ? 1 : 0);
      }

      static bool deserializeBool(const char* buf, size_t bufLen,
         bool* outValue, unsigned* outLen)
      {
         char charVal = 0;

         bool deserRes = deserializeChar(buf, bufLen, &charVal, outLen);

         *outValue = (charVal != 0);

         return deserRes;
      }

      static unsigned serialLenBool()
      {
         return serialLenChar();
      }

      // short (de)serialization
      static inline unsigned serializeShort(char* buf, short value)
      {
         return serializeUShort(buf, value);
      }

      static inline bool deserializeShort(const char* buf, size_t bufLen,
         short* outValue, unsigned* outLen)
      {
         return deserializeUShort(buf, bufLen, (unsigned short*)outValue, outLen);
      }

      static inline unsigned serialLenShort()
      {
         return serialLenUShort();
      }

      // ushort (de)serialization
      static inline unsigned serializeUShort(char* buf, unsigned short value)
      {
         return serializeUInt16(buf, value);
      }

      static inline bool deserializeUShort(const char* buf, size_t bufLen,
         unsigned short* outValue, unsigned* outLen)
      {
         return deserializeUInt16(buf, bufLen, outValue, outLen);
      }

      static inline unsigned serialLenUShort()
      {
         return serialLenUInt16();
      }
      
      // int (de)serialization
      static inline unsigned serializeInt(char* buf, int value)
      {
         return serializeUInt(buf, value);
      }

      static inline bool deserializeInt(const char* buf, size_t bufLen,
         int* outValue, unsigned* outLen)
      {
         return deserializeUInt(buf, bufLen, (unsigned*)outValue, outLen);
      }

      static inline unsigned serialLenInt()
      {
         return serialLenUInt();
      }

      // uint (de)serialization
      static inline unsigned serializeUInt(char* buf, unsigned value)
      {
         HOST_TO_LITTLE_ENDIAN_32(value, *(unsigned*)buf);

         return serialLenUInt();
      }

      static inline bool deserializeUInt(const char* buf, size_t bufLen,
         unsigned* outValue, unsigned* outLen)
      {
         if(unlikely(bufLen < serialLenUInt() ) )
            return false;

         *outLen = serialLenUInt();
         LITTLE_ENDIAN_TO_HOST_32(*(const unsigned*)buf, *outValue);

         return true;
      }

      static inline unsigned serialLenUInt()
      {
         return sizeof(unsigned);
      }

      // uint8_t (de)serialization
      static inline unsigned serializeUInt8(char* buf, uint8_t value)
      {
         return serializeChar(buf, value);
      }

      static inline bool deserializeUInt8(const char* buf, size_t bufLen,
         uint8_t* outValue, unsigned* outLen)
      {
         return deserializeChar(buf, bufLen, (char* )outValue, outLen);
      }

      static inline unsigned serialLenUInt8()
      {
         return serialLenChar();
      }


      // uint16_t (de)serialization
      static inline unsigned serializeUInt16(char* buf, uint16_t value)
      {
         HOST_TO_LITTLE_ENDIAN_16(value, *(unsigned short*)buf);

         return serialLenUInt16();
      }

      static inline bool deserializeUInt16(const char* buf, size_t bufLen,
         uint16_t* outValue, unsigned* outLen)
      {
         if(unlikely(bufLen < serialLenUInt16() ) )
            return false;

         *outLen = serialLenUInt16();
         LITTLE_ENDIAN_TO_HOST_16(*(const unsigned short*)buf, *outValue);

         return true;
      }

      static inline unsigned serialLenUInt16()
      {
         return sizeof(uint16_t);
      }



      // int64 (de)serialization
      static inline unsigned serializeInt64(char* buf, int64_t value)
      {
         return serializeUInt64(buf, value);
      }

      static inline bool deserializeInt64(const char* buf, size_t bufLen,
         int64_t* outValue, unsigned* outLen)
      {
         return deserializeUInt64(buf, bufLen, (uint64_t*)outValue, outLen);
      }

      static inline unsigned serialLenInt64()
      {
         return serialLenUInt64();
      }

      // uint64 (de)serialization
      static inline unsigned serializeUInt64(char* buf, uint64_t value)
      {
         HOST_TO_LITTLE_ENDIAN_64(value, *(uint64_t*)buf);

         return serialLenUInt64();
      }

      static inline bool deserializeUInt64(const char* buf, size_t bufLen,
         uint64_t* outValue, unsigned* outLen)
      {
         if(unlikely(bufLen < serialLenUInt64() ) )
            return false;

         *outLen = serialLenUInt64();
         LITTLE_ENDIAN_TO_HOST_64(*(const uint64_t*)buf, *outValue);

         return true;
      }

      static inline unsigned serialLenUInt64()
      {
         return sizeof(uint64_t);
      }

      static inline uint32_t htonlTrans(uint32_t value)
      {
         return htonl(value);
      }

      static inline uint32_t ntohlTrans(uint32_t value)
      {
         return ntohl(value);
      }

      static inline uint16_t htonsTrans(uint16_t value)
      {
         return htons(value);
      }

      static inline uint16_t ntohsTrans(uint16_t value)
      {
         return ntohs(value);
      }

      // host to network byte order (big endian) transformation for 64-bit types

      #if BYTE_ORDER == BIG_ENDIAN // BIG_ENDIAN

         static inline uint64_t htonllTrans(int64_t value)
         {
            return value;
         }

      #else // LITTLE_ENDIAN

         static inline uint64_t htonllTrans(int64_t value)
         {
            return byteswap64(value);
         }

      #endif


      static inline uint64_t ntohllTrans(int64_t value)
      {
         return htonllTrans(value);
      }

      static inline unsigned short byteswap16(unsigned short x)
      {
         char* xChars = (char*)&x;

         unsigned short retVal;
         char* retValChars = (char*)&retVal;

         retValChars[0] = xChars[1];
         retValChars[1] = xChars[0];

         return retVal;
      }

      static inline unsigned byteswap32(unsigned x)
      {
         char* xChars = (char*)&x;

         unsigned retVal;
         char* retValChars = (char*)&retVal;

         retValChars[0] = xChars[3];
         retValChars[1] = xChars[2];
         retValChars[2] = xChars[1];
         retValChars[3] = xChars[0];

         return retVal;
      }

      static inline uint64_t byteswap64(uint64_t x)
      {
         char* xChars = (char*)&x;

         uint64_t retVal;
         char* retValChars = (char*)&retVal;

         retValChars[0] = xChars[7];
         retValChars[1] = xChars[6];
         retValChars[2] = xChars[5];
         retValChars[3] = xChars[4];
         retValChars[4] = xChars[3];
         retValChars[5] = xChars[2];
         retValChars[6] = xChars[1];
         retValChars[7] = xChars[0];

         return retVal;
      }

};

#endif /*SERIALIZATION_H_*/
