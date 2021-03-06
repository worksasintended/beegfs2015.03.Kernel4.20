#include "Serialization.h"

//                                                 5 = 4b ipAddr + 1b nicType
#define SERIALIZATION_NICLISTELEM_2012_SIZE       (5+SERIALIZATION_NICLISTELEM_NAME_SIZE)



/**
 * Serialization of a NicList.
 *
 * note: 4 byte aligned.
 *
 * @return 0 on error, used buffer size otherwise
 */
unsigned Serialization::serializeNicList(char* buf, NicAddressList* nicList)
{
   unsigned nicListSize = nicList->size();

   size_t bufPos = 0;

   // elem count info field

   bufPos += serializeUInt(&buf[bufPos], nicListSize);


   // serialize each element of the nicList

   NicAddressListIter iter = nicList->begin();

   for(unsigned i=0; i < nicListSize; i++, iter++)
   {
      const size_t minNameSize = BEEGFS_MIN(sizeof(iter->name), SERIALIZATION_NICLISTELEM_NAME_SIZE);

      // ipAddress
      *(unsigned*)&buf[bufPos] = iter->ipAddr.s_addr;

      bufPos += 4;

      // name
      memcpy(&buf[bufPos], &(iter->name), minNameSize);

      bufPos += SERIALIZATION_NICLISTELEM_NAME_SIZE;

      // nicType
      buf[bufPos] = (char)iter->nicType;

      bufPos += 1;

      // 3 bytes padding (for 4 byte alignment)
      bufPos += 3;
   }

   return bufPos;
}

/**
 * Pre-processes a serialized NicList for deserialization via deserializeNicList().
 *
 * @return false on error or inconsistency
 */
bool Serialization::deserializeNicListPreprocess(const char* buf, size_t bufLen,
   unsigned* outNicListElemNum, const char** outNicListStart, unsigned* outLen)
{
   size_t bufPos = 0;

   unsigned elemNumFieldLen = 0;
   if(unlikely(!deserializeUInt(&buf[bufPos], bufLen-bufPos, outNicListElemNum,
      &elemNumFieldLen) ) )
      return false;
   bufPos += elemNumFieldLen;

   *outLen = bufPos + *outNicListElemNum * SERIALIZATION_NICLISTELEM_SIZE;

   if(unlikely(bufLen < *outLen) )
      return false;

   *outNicListStart = &buf[bufPos];

   return true;
}

/**
 * Deserializes a NicList.
 * (requires pre-processing)
 */
void Serialization::deserializeNicList(unsigned nicListElemNum, const char* nicListStart,
   NicAddressList* outNicList)
{
   const char* currentNicListPos = nicListStart;

   outNicList->clear(); // make sure it is empty

   NicAddress nicAddr;
   const size_t minNameSize = BEEGFS_MIN(sizeof(nicAddr.name), SERIALIZATION_NICLISTELEM_NAME_SIZE);
   memset(&nicAddr, 0, sizeof(nicAddr) ); // clear unused fields

   for(unsigned i=0; i < nicListElemNum; i++)
   {
      // ipAddress
      nicAddr.ipAddr.s_addr = *(unsigned*)currentNicListPos;

      currentNicListPos += 4;

      // name
      memcpy(&nicAddr.name, currentNicListPos, minNameSize);
      nicAddr.name[minNameSize-1] = 0; // make sure the string is zero-terminated

      currentNicListPos += SERIALIZATION_NICLISTELEM_NAME_SIZE;

      // nicType
      nicAddr.nicType = (NicAddrType)*currentNicListPos;

      currentNicListPos += 1;

      // 3 bytes padding (for 4 byte alignment)
      currentNicListPos += 3;


      outNicList->push_back(nicAddr);
   }

}

/**
 * note: 4 byte aligned
 */
unsigned Serialization::serialLenNicList(NicAddressList* nicList)
{
   // elem count info field + elements
   return serialLenUInt() + (nicList->size() * SERIALIZATION_NICLISTELEM_SIZE);
}



/**
 * Pre-processes a serialized NicList in 2012 format for deserialization via deserializeNicList().
 *
 * @return false on error or inconsistency
 */
bool Serialization::deserializeNicListPreprocess2012(const char* buf, size_t bufLen,
   unsigned* outNicListElemNum, const char** outNicListStart, unsigned* outLen)
{
   size_t bufPos = 0;

   unsigned elemNumFieldLen = 0;
   if(unlikely(!deserializeUInt(&buf[bufPos], bufLen-bufPos, outNicListElemNum,
      &elemNumFieldLen) ) )
      return false;
   bufPos += elemNumFieldLen;

   *outLen = bufPos + *outNicListElemNum * SERIALIZATION_NICLISTELEM_2012_SIZE;

   if(unlikely(bufLen < *outLen) )
      return false;

   *outNicListStart = &buf[bufPos];

   return true;
}

/**
 * Deserializes a NicList in 2012 format
 * (requires pre-processing)
 */
void Serialization::deserializeNicList2012(unsigned nicListElemNum, const char* nicListStart,
   NicAddressList* outNicList)
{
   const char* currentNicListPos = nicListStart;

   outNicList->clear();

   NicAddress nicAddr;
   const size_t minNameSize = BEEGFS_MIN(sizeof(nicAddr.name), SERIALIZATION_NICLISTELEM_NAME_SIZE);
   memset(&nicAddr, 0, sizeof(nicAddr) ); // clear unused fields

   for(unsigned i=0; i < nicListElemNum; i++)
   {
      // ipAddress
      nicAddr.ipAddr.s_addr = *(unsigned*)currentNicListPos;

      currentNicListPos += 4;

      // nicType
      nicAddr.nicType = (NicAddrType)*currentNicListPos;

      currentNicListPos += 1;

      // name
      memcpy(&nicAddr.name, currentNicListPos, minNameSize);
      nicAddr.name[minNameSize-1] = 0; // make sure the string is zero-terminated

      currentNicListPos += SERIALIZATION_NICLISTELEM_NAME_SIZE;

      outNicList->push_back(nicAddr);
   }

}

