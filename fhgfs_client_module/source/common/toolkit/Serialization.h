#ifndef SERIALIZATION_H_
#define SERIALIZATION_H_

#include <common/Common.h>
#include <common/net/sock/NetworkInterfaceCard.h>
#include <common/net/sock/NicAddressList.h>
#include <common/toolkit/list/Int64CpyList.h>
#include <common/toolkit/list/Int64CpyListIter.h>
#include <common/toolkit/list/IntCpyList.h>
#include <common/toolkit/list/IntCpyListIter.h>
#include <common/toolkit/list/StrCpyList.h>
#include <common/toolkit/list/StrCpyListIter.h>
#include <common/toolkit/list/UInt8List.h>
#include <common/toolkit/list/UInt8ListIter.h>
#include <common/toolkit/list/UInt16List.h>
#include <common/toolkit/list/UInt16ListIter.h>
#include <common/toolkit/vector/Int64CpyVec.h>
#include <common/toolkit/vector/IntCpyVec.h>
#include <common/toolkit/vector/StrCpyVec.h>
#include <common/toolkit/vector/UInt16Vec.h>
#include <common/toolkit/vector/UInt8Vec.h>
#include <common/toolkit/ListTk.h>
#include <common/storage/Path.h>
#include <common/nodes/Node.h>
#include <common/nodes/NodeList.h>
#include <common/nodes/NodeListIter.h>
#include <os/OsCompat.h>

#include <asm/unaligned.h>


#define SERIALIZATION_NICLISTELEM_NAME_SIZE  (16)
#define SERIALIZATION_NICLISTELEM_SIZE       (8+SERIALIZATION_NICLISTELEM_NAME_SIZE) /*
                                              8 = 4b ipAddr + 1b nicType + 3b alignment padding */


struct PathDeserializationInfo;
typedef struct PathDeserializationInfo PathDeserializationInfo;

struct PathDeserializationInfo
{
   unsigned elemsBufLen;
   unsigned elemNum;
   const char* elemListStart;
   fhgfs_bool isAbsolute;
};

// string (de)serialization
unsigned Serialization_serializeStr(char* buf, unsigned strLen, const char* strStart);
fhgfs_bool Serialization_deserializeStr(const char* buf, size_t bufLen,
   unsigned* outStrLen, const char** outStrStart, unsigned* outLen);
unsigned Serialization_serialLenStr(unsigned strLen);

// string aligned (de)serialization
unsigned Serialization_serializeStrAlign4(char* buf, unsigned strLen, const char* strStart);
fhgfs_bool Serialization_deserializeStrAlign4(const char* buf, size_t bufLen,
   unsigned* outStrLen, const char** outStrStart, unsigned* outLen);
unsigned Serialization_serialLenStrAlign4(unsigned strLen);

// char array (arbitrary binary data) (de)serialization
unsigned Serialization_serializeCharArray(char* buf, unsigned arrLen, const char* arrStart);
fhgfs_bool Serialization_deserializeCharArray(const char* buf, size_t bufLen,
   unsigned* outArrLen, const char** outArrStart, unsigned* outLen);
unsigned Serialization_serialLenCharArray(unsigned arrLen);

// nicList (de)serialization
unsigned Serialization_serializeNicList(char* buf, NicAddressList* nicList);
fhgfs_bool Serialization_deserializeNicListPreprocess(const char* buf, size_t bufLen,
   unsigned* outNicListElemNum, const char** outNicListStart, unsigned* outLen);
void Serialization_deserializeNicList(unsigned nicListElemNum, const char* nicListStart,
   NicAddressList* outNicList);
unsigned Serialization_serialLenNicList(NicAddressList* nicList);

// nodeList (de)serialization
unsigned Serialization_serializeNodeList(char* buf, NodeList* nodeList);
fhgfs_bool Serialization_deserializeNodeListPreprocess(const char* buf, size_t bufLen,
   unsigned* outNodeListElemNum, const char** outNodeListStart, unsigned* outLen);
void Serialization_deserializeNodeList(App* app, unsigned nodeListElemNum,
   const char* nodeListStart, NodeList* outNodeList);
unsigned Serialization_serialLenNodeList(NodeList* nodeList);

// strCpyList (de)serialization
unsigned Serialization_serializeStrCpyList(char* buf, StrCpyList* list);
fhgfs_bool Serialization_deserializeStrCpyListPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen);
fhgfs_bool Serialization_deserializeStrCpyList(unsigned listBufLen, unsigned elemNum,
   const char* listStart, StrCpyList* outList);
unsigned Serialization_serialLenStrCpyList(StrCpyList* list);

// strCpyVec (de)serialization
unsigned Serialization_serializeStrCpyVec(char* buf, StrCpyVec* vec);
fhgfs_bool Serialization_deserializeStrCpyVecPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen);
fhgfs_bool Serialization_deserializeStrCpyVec(unsigned listBufLen, unsigned elemNum,
   const char* listStart, StrCpyVec* outVec);
unsigned Serialization_serialLenStrCpyVec(StrCpyVec* vec);

// intList (de)serialization
unsigned Serialization_serializeIntCpyList(char* buf, IntCpyList* list);
fhgfs_bool Serialization_deserializeIntCpyListPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen);
fhgfs_bool Serialization_deserializeIntCpyList(unsigned listBufLen, unsigned elemNum,
   const char* listStart, IntCpyList* outList);
unsigned Serialization_serialLenIntCpyList(IntCpyList* list);

// intVec (de)serialization
unsigned Serialization_serializeIntCpyVec(char* buf, IntCpyVec* vec);
fhgfs_bool Serialization_deserializeIntCpyVecPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen);
fhgfs_bool Serialization_deserializeIntCpyVec(unsigned listBufLen, unsigned elemNum,
   const char* listStart, IntCpyVec* outVec);
unsigned Serialization_serialLenIntCpyVec(IntCpyVec* vec);

// uint8List (de)serialization
unsigned Serialization_serializeUInt8List(char* buf, UInt8List* list);
fhgfs_bool Serialization_deserializeUInt8ListPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen);
fhgfs_bool Serialization_deserializeUInt8List(unsigned listBufLen, unsigned elemNum,
   const char* listStart, UInt8List* outList);
unsigned Serialization_serialLenUInt8List(UInt8List* list);

// uint8Vec (de)serialization
unsigned Serialization_serializeUInt8Vec(char* buf, UInt8Vec* list);
fhgfs_bool Serialization_deserializeUInt8VecPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen);
fhgfs_bool Serialization_deserializeUInt8Vec(unsigned listBufLen, unsigned elemNum,
   const char* listStart, UInt8Vec* outList);
unsigned Serialization_serialLenUInt8Vec(UInt8Vec* list);

// uint16List (de)serialization
unsigned Serialization_serializeUInt16List(char* buf, UInt16List* list);
fhgfs_bool Serialization_deserializeUInt16ListPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen);
fhgfs_bool Serialization_deserializeUInt16List(unsigned listBufLen, unsigned elemNum,
   const char* listStart, UInt16List* outList);
unsigned Serialization_serialLenUInt16List(UInt16List* list);

// uint16Vec (de)serialization
unsigned Serialization_serializeUInt16Vec(char* buf, UInt16Vec* vec);
fhgfs_bool Serialization_deserializeUInt16VecPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen);
fhgfs_bool Serialization_deserializeUInt16Vec(unsigned listBufLen, unsigned elemNum,
   const char* listStart, UInt16Vec* outVec);
unsigned Serialization_serialLenUInt16Vec(UInt16Vec* vec);

// int64CpyList (de)serialization
unsigned Serialization_serializeInt64CpyList(char* buf, Int64CpyList* list);
fhgfs_bool Serialization_deserializeInt64CpyListPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen);
fhgfs_bool Serialization_deserializeInt64CpyList(unsigned listBufLen, unsigned elemNum,
   const char* listStart, Int64CpyList* outList);
unsigned Serialization_serialLenInt64CpyList(Int64CpyList* list);

// uint64Vec (de)serialization
unsigned Serialization_serializeInt64CpyVec(char* buf, Int64CpyVec* vec);
fhgfs_bool Serialization_deserializeInt64CpyVecPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen);
fhgfs_bool Serialization_deserializeInt64CpyVec(unsigned listBufLen, unsigned elemNum,
   const char* listStart, Int64CpyVec* outVec);
unsigned Serialization_serialLenInt64CpyVec(Int64CpyVec* list);

// path (de)serialization
unsigned Serialization_serializePath(char* buf, Path* path);
fhgfs_bool Serialization_deserializePathPreprocess(const char* buf, size_t bufLen,
   struct PathDeserializationInfo* outInfo, unsigned* outLen);
fhgfs_bool Serialization_deserializePath(struct PathDeserializationInfo* info, Path* outPath);
unsigned Serialization_serialLenPath(Path* path);

uint16_t Serialization_cpu_to_be16(uint16_t value);
uint16_t Serialization_be16_to_cpu(uint16_t value);
uint32_t Serialization_cpu_to_be32(uint32_t value);
uint32_t Serialization_be32_to_cpu(uint32_t value);
uint64_t Serialization_cpu_to_be64(uint64_t value);
uint64_t Serialization_be64_to_cpu(uint64_t value);


// inliners
static inline unsigned Serialization_serialLenChar(void);
static inline unsigned Serialization_serializeChar(char* buf, char value);
static inline fhgfs_bool Serialization_deserializeChar(const char* buf, size_t bufLen,
   char* outValue, unsigned* outLen);
static inline unsigned Serialization_serialLenBool(void);
static inline unsigned Serialization_serializeBool(char* buf, fhgfs_bool value);
static inline fhgfs_bool Serialization_deserializeBool(const char* buf, size_t bufLen,
   fhgfs_bool* outValue, unsigned* outLen);
static inline unsigned Serialization_serialLenUInt8(void);
static inline unsigned Serialization_serializeUInt8(char* buf, uint8_t value);
static inline fhgfs_bool Serialization_deserializeUInt8(const char* buf, size_t bufLen,
   uint8_t* outValue, unsigned* outLen);
static inline unsigned Serialization_serialLenShort(void);
static inline unsigned Serialization_serializeShort(char* buf, short value);
static inline fhgfs_bool Serialization_deserializeShort(const char* buf, size_t bufLen,
   short* outValue, unsigned* outLen);
static inline unsigned Serialization_serialLenUShort(void);
static inline unsigned Serialization_serializeUShort(char* buf, unsigned short value);
static inline fhgfs_bool Serialization_deserializeUShort(const char* buf, size_t bufLen,
   unsigned short* outValue, unsigned* outLen);
static inline unsigned Serialization_serializeInt(char* buf, int value);
static inline fhgfs_bool Serialization_deserializeInt(const char* buf, size_t bufLen,
   int* outValue, unsigned* outLen);
static inline unsigned Serialization_serialLenInt(void);
static inline unsigned Serialization_serializeUInt(char* buf, unsigned value);
static inline fhgfs_bool Serialization_deserializeUInt(const char* buf, size_t bufLen,
   unsigned* outValue, unsigned* outLen);
static inline unsigned Serialization_serialLenUInt(void);
static inline unsigned Serialization_serializeInt64(char* buf, int64_t value);
static inline fhgfs_bool Serialization_deserializeInt64(const char* buf, size_t bufLen,
   int64_t* outValue, unsigned* outLen);
static inline unsigned Serialization_serialLenInt64(void);
static inline unsigned Serialization_serializeUInt64(char* buf, uint64_t value);
static inline fhgfs_bool Serialization_deserializeUInt64(const char* buf, size_t bufLen,
   uint64_t* outValue, unsigned* outLen);
static inline unsigned Serialization_serialLenUInt64(void);

static inline unsigned Serialization_htonlTrans(unsigned value);
static inline unsigned Serialization_ntohlTrans(unsigned value);
static inline unsigned short Serialization_htonsTrans(unsigned short value);
static inline unsigned short Serialization_ntohsTrans(unsigned short value);
static inline uint64_t Serialization_htonllTrans(uint64_t value);
static inline uint64_t Serialization_ntohllTrans(uint64_t value);


// char (de)serialization
static inline unsigned Serialization_serialLenChar(void)
{
   return sizeof(char);
}

static inline unsigned Serialization_serializeChar(char* buf, char value)
{
   *buf = value;

   return Serialization_serialLenChar();
}

static inline fhgfs_bool Serialization_deserializeChar(const char* buf, size_t bufLen,
   char* outValue, unsigned* outLen)
{
   if(unlikely(bufLen < Serialization_serialLenChar() ) )
      return fhgfs_false;

   *outLen = Serialization_serialLenChar();
   *outValue = *buf;

   return fhgfs_true;
}


// fhgfs_bool (de)serialization
static inline unsigned Serialization_serialLenBool(void)
{
   return Serialization_serialLenChar();
}

static inline unsigned Serialization_serializeBool(char* buf, fhgfs_bool value)
{
   return Serialization_serializeChar(buf, value ? 1 : 0);
}

static inline fhgfs_bool Serialization_deserializeBool(const char* buf, size_t bufLen,
   fhgfs_bool* outValue, unsigned* outLen)
{
   char charVal = 0;

   fhgfs_bool deserRes = Serialization_deserializeChar(buf, bufLen, &charVal, outLen);

   *outValue = (charVal != 0);

   return deserRes;
}

// uint8_t (de)serialization
static inline unsigned Serialization_serialLenUInt8(void)
{
   return Serialization_serialLenChar();
}

static inline unsigned Serialization_serializeUInt8(char* buf, uint8_t value)
{
   return Serialization_serializeChar(buf, value);
}

static inline fhgfs_bool Serialization_deserializeUInt8(const char* buf, size_t bufLen,
   uint8_t* outValue, unsigned* outLen)
{
   return Serialization_deserializeChar(buf, bufLen, (char*)outValue, outLen);
}

// short (de)serialization
static inline unsigned Serialization_serialLenShort(void)
{
   return Serialization_serialLenUShort();
}

static inline unsigned Serialization_serializeShort(char* buf, short value)
{
   return Serialization_serializeUShort(buf, value);
}

static inline fhgfs_bool Serialization_deserializeShort(const char* buf, size_t bufLen,
   short* outValue, unsigned* outLen)
{
   return Serialization_deserializeUShort(buf, bufLen, (unsigned short*)outValue, outLen);
}

// ushort (de)serialization
static inline unsigned Serialization_serialLenUShort(void)
{
   return sizeof(unsigned short);
}

static inline unsigned Serialization_serializeUShort(char* buf, unsigned short value)
{
   put_unaligned_le16(value, buf);

   return Serialization_serialLenUShort();
}

static inline fhgfs_bool Serialization_deserializeUShort(const char* buf, size_t bufLen,
   unsigned short* outValue, unsigned* outLen)
{
   if(unlikely(bufLen < Serialization_serialLenUShort() ) )
      return fhgfs_false;

   *outLen = Serialization_serialLenUShort();

   *outValue = get_unaligned_le16(buf);

   return fhgfs_true;
}

// int (de)serialization
static inline unsigned Serialization_serialLenInt(void)
{
   return Serialization_serialLenUInt();
}

static inline unsigned Serialization_serializeInt(char* buf, int value)
{
   return Serialization_serializeUInt(buf, value);
}

static inline fhgfs_bool Serialization_deserializeInt(const char* buf, size_t bufLen,
   int* outValue, unsigned* outLen)
{
   return Serialization_deserializeUInt(buf, bufLen, (unsigned*)outValue, outLen);
}


// uint (de)serialization
static inline unsigned Serialization_serializeUInt(char* buf, unsigned value)
{
   put_unaligned_le32(value, buf);

   return Serialization_serialLenUInt();
}

static inline fhgfs_bool Serialization_deserializeUInt(const char* buf, size_t bufLen,
   unsigned* outValue, unsigned* outLen)
{
   if(unlikely(bufLen < Serialization_serialLenUInt() ) )
      return fhgfs_false;

   *outLen = Serialization_serialLenUInt();
   *outValue = get_unaligned_le32(buf);

   return fhgfs_true;
}

static inline unsigned Serialization_serialLenUInt(void)
{
   return sizeof(unsigned);
}

// int64 (de)serialization
static inline unsigned Serialization_serializeInt64(char* buf, int64_t value)
{
   return Serialization_serializeUInt64(buf, value);
}

static inline fhgfs_bool Serialization_deserializeInt64(const char* buf, size_t bufLen,
   int64_t* outValue, unsigned* outLen)
{
   return Serialization_deserializeUInt64(buf, bufLen, (uint64_t*)outValue, outLen);
}

static inline unsigned Serialization_serialLenInt64(void)
{
   return Serialization_serialLenUInt64();
}

// uint64 (de)serialization
static inline unsigned Serialization_serializeUInt64(char* buf, uint64_t value)
{
   put_unaligned_le64(value, buf);

   return Serialization_serialLenUInt64();
}

static inline fhgfs_bool Serialization_deserializeUInt64(const char* buf, size_t bufLen,
   uint64_t* outValue, unsigned* outLen)
{
   if(unlikely(bufLen < Serialization_serialLenUInt64() ) )
      return fhgfs_false;

   *outLen = Serialization_serialLenUInt64();
   *outValue = get_unaligned_le64(buf);

   return fhgfs_true;
}

static inline unsigned Serialization_serialLenUInt64(void)
{
   return sizeof(uint64_t);
}

// byte-order transformation

unsigned Serialization_htonlTrans(unsigned value)
{
   return Serialization_cpu_to_be32(value);
   //return htonl(value);
}

unsigned Serialization_ntohlTrans(unsigned value)
{
   return Serialization_be32_to_cpu(value);
   //return ntohl(value);
}

unsigned short Serialization_htonsTrans(unsigned short value)
{
   return Serialization_cpu_to_be16(value);
   //return htons(value);
}

unsigned short Serialization_ntohsTrans(unsigned short value)
{
   return Serialization_be16_to_cpu(value);
   //return ntohs(value);
}

uint64_t Serialization_htonllTrans(uint64_t value)
{
   return Serialization_cpu_to_be64(value);
   //return (((int64_t)(Serialization_ntohlTrans((int)((value << 32) >> 32))) << 32) |
   //            (unsigned int)Serialization_ntohlTrans(((int)(value >> 32))) );
}

uint64_t Serialization_ntohllTrans(uint64_t value)
{
   return Serialization_be64_to_cpu(value);
   //return Serialization_htonllTrans(value);
}

#endif /*SERIALIZATION_H_*/
