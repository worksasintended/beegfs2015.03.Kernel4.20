#include "Serialization.h"
#include <os/OsDeps.h>

/**
 * @return 0 on error (e.g. strLen is greater than bufLen), used buffer size otherwise
 */
unsigned Serialization_serializeStr(char* buf, unsigned strLen, const char* strStart)
{
   size_t bufPos = 0;

   // write length field
   bufPos += Serialization_serializeUInt(&buf[bufPos], strLen);

   // write raw string
   os_memcpy(&buf[bufPos], strStart, strLen);
   bufPos += strLen;

   // write termination char
   buf[bufPos] = 0;

   return Serialization_serialLenStr(strLen);
}

/**
 * @return fhgfs_false on error (e.g. strLen is greater than bufLen)
 */
fhgfs_bool Serialization_deserializeStr(const char* buf, size_t bufLen,
   unsigned* outStrLen, const char** outStrStart, unsigned* outLen)
{
   size_t bufPos = 0;
   unsigned strLenFieldLen = 0; // "=0" to mute false compiler warning

   // check min length
   if(unlikely(bufLen < Serialization_serialLenStr(0) ) )
      return fhgfs_false;


   // length field
   Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, outStrLen, &strLenFieldLen);
   bufPos += strLenFieldLen;

   // string start
   *outStrStart = &buf[bufPos];
   bufPos += *outStrLen;

   // required outLen
   *outLen = Serialization_serialLenStr(*outStrLen);

   // check length and terminating zero
   if(unlikely( (bufLen < (*outLen) ) || ( (*outStrStart)[*outStrLen] != 0) ) )
      return fhgfs_false;

   return fhgfs_true;
}

unsigned Serialization_serialLenStr(unsigned strLen)
{
   // strLenField + str + terminating zero
   return Serialization_serialLenUInt() + strLen + 1;
}

/**
 * @return 0 on error (e.g. arrLen is greater than bufLen), used buffer size otherwise
 */
unsigned Serialization_serializeCharArray(char* buf, unsigned arrLen, const char* arrStart)
{
   unsigned requiredLen = Serialization_serialLenCharArray(arrLen);

   size_t bufPos = 0;

   // totalBufLen field
   bufPos += Serialization_serializeUInt(&buf[bufPos], requiredLen);

   // elem count info field
   bufPos += Serialization_serializeUInt(&buf[bufPos], arrLen);

   // copy raw array data
   os_memcpy(&buf[bufPos], arrStart, arrLen);
   bufPos += arrLen;

   return requiredLen;
}

/**
 * @return fhgfs_false on error (e.g. arrLen is greater than bufLen)
 */
fhgfs_bool Serialization_deserializeCharArray(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outArrStart, unsigned* outLen)
{
   size_t bufPos = 0;

   unsigned bufLenFieldLen = 0; // "=0" to mute false compiler warning
   unsigned elemNumFieldLen = 0;

   // check min length
   if(unlikely(bufLen < Serialization_serialLenCharArray(0) ) )
      return fhgfs_false;

   // totalBufLen info field
   if(unlikely(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, outLen,
         &bufLenFieldLen) ) )
      return fhgfs_false;
   bufPos += bufLenFieldLen;

   if(unlikely(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, outElemNum,
         &elemNumFieldLen) ) )
      return fhgfs_false;
   bufPos += elemNumFieldLen;

   // array start
   *outArrStart = &buf[bufPos];

   // check length
   if( (*outLen > bufLen) ||
         ( (*outLen - bufPos) != (*outElemNum * Serialization_serialLenChar() ) ) )
      return fhgfs_false;

   return fhgfs_true;
}

unsigned Serialization_serialLenCharArray(unsigned arrLen)
{
   // totalBufLen info field + elemCount info field + buffer
   return Serialization_serialLenUInt() + Serialization_serialLenUInt() + arrLen;
}

/**
 * Note: adds padding to achieve 4 byte alignment
 *
 * @return 0 on error (e.g. strLen is greater than bufLen), used buffer size otherwise
 */
unsigned Serialization_serializeStrAlign4(char* buf, unsigned strLen, const char* strStart)
{
   size_t bufPos = 0;

   // write length field
   bufPos += Serialization_serializeUInt(&buf[bufPos], strLen);

   // write raw string
   os_memcpy(&buf[bufPos], strStart, strLen);
   bufPos += strLen;

   // write termination char
   buf[bufPos] = 0;

   return Serialization_serialLenStrAlign4(strLen);
}

/**
 * @return fhgfs_false on error (e.g. strLen is greater than bufLen)
 */
fhgfs_bool Serialization_deserializeStrAlign4(const char* buf, size_t bufLen,
   unsigned* outStrLen, const char** outStrStart, unsigned* outLen)
{
   size_t bufPos = 0;
   unsigned strLenFieldLen = 0; // "=0" to mute false compiler warning

   // check min length
   if(unlikely(bufLen < Serialization_serialLenStrAlign4(0) ) )
      return fhgfs_false;


   // length field
   Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, outStrLen, &strLenFieldLen);
   bufPos += strLenFieldLen;

   // string start
   *outStrStart = &buf[bufPos];
   bufPos += *outStrLen;

   // required outLen (incl alignment padding)
   *outLen = Serialization_serialLenStrAlign4(*outStrLen);

   // check length and terminating zero
   if(unlikely( (bufLen < (*outLen) ) || ( (*outStrStart)[*outStrLen] != 0) ) )
      return fhgfs_false;

   return fhgfs_true;
}

unsigned Serialization_serialLenStrAlign4(unsigned strLen)
{
   // strLenField + str + terminating zero
   unsigned rawLen = Serialization_serialLenUInt() + strLen + 1;
   unsigned remainder = rawLen % 4;
   unsigned alignedLen = remainder ? (rawLen + 4 - remainder) : rawLen;

   return alignedLen;
}

/**
 * Serialization of a NicList.
 *
 * note: 4 byte aligned.
 *
 * @return 0 on error, used buffer size otherwise
 */
unsigned Serialization_serializeNicList(char* buf, NicAddressList* nicList)
{
   unsigned nicListSize = NicAddressList_length(nicList);
   NicAddressListIter iter;
   unsigned i;

   size_t bufPos = 0;

   // elem count info field

   bufPos += Serialization_serializeUInt(&buf[bufPos], nicListSize);


   // serialize each element of the nicList

   NicAddressListIter_init(&iter, nicList);

   for(i=0; i < nicListSize; i++, NicAddressListIter_next(&iter) )
   {
      NicAddress* nicAddr = NicAddressListIter_value(&iter);
      const size_t minNameSize = MIN(sizeof(nicAddr->name), SERIALIZATION_NICLISTELEM_NAME_SIZE);

      { // ipAddress
         put_unaligned(nicAddr->ipAddr, (unsigned*)&buf[bufPos]);
         bufPos += 4;
      }

      { // name
         os_memcpy(&buf[bufPos], &(nicAddr->name), minNameSize);
         bufPos += SERIALIZATION_NICLISTELEM_NAME_SIZE;
      }

      {  // nicType
         put_unaligned((uint8_t) nicAddr->nicType, (char*)&buf[bufPos]);
         bufPos += 1;
      }

      {  // 3 bytes padding (for 4 byte alignment)
         bufPos += 3;
      }

   }

   NicAddressListIter_uninit(&iter);

   return Serialization_serialLenNicList(nicList);
}

/**
 * Pre-processes a serialized NicList for deserialization via deserializeNicList().
 *
 * @return fhgfs_false on error or inconsistency
 */
fhgfs_bool Serialization_deserializeNicListPreprocess(const char* buf, size_t bufLen,
   unsigned* outNicListElemNum, const char** outNicListStart, unsigned* outLen)
{
   size_t bufPos = 0;

   unsigned elemNumFieldLen;
   if(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, outNicListElemNum,
      &elemNumFieldLen) )
   {
      return fhgfs_false;
   }

   bufPos += elemNumFieldLen;

   *outLen = bufPos + *outNicListElemNum * SERIALIZATION_NICLISTELEM_SIZE;

   if(unlikely(bufLen < *outLen) )
      return fhgfs_false;

   *outNicListStart = &buf[bufPos];

   return fhgfs_true;
}

/**
 * Deserializes a NicList.
 * (requires pre-processing via nicListBufToMsg() )
 */
void Serialization_deserializeNicList(unsigned nicListElemNum, const char* nicListStart,
   NicAddressList* outNicList)
{
   const char* currentNicListPos = nicListStart;
   unsigned i;


   for(i=0; i < nicListElemNum; i++)
   {
      NicAddress* nicAddr = os_kmalloc(sizeof(NicAddress) );
      const size_t minNameSize = MIN(sizeof(nicAddr->name), SERIALIZATION_NICLISTELEM_NAME_SIZE);
      os_memset(nicAddr, 0, sizeof(NicAddress) ); // clear unused fields

      { // ipAddress
         nicAddr->ipAddr = get_unaligned((unsigned*)currentNicListPos);
         currentNicListPos += 4;
      }

      { // name
         os_memcpy(&nicAddr->name, currentNicListPos, minNameSize);
         (nicAddr->name)[minNameSize-1] = 0; // make sure the string is zero-terminated
      
         currentNicListPos += SERIALIZATION_NICLISTELEM_NAME_SIZE;
      }
      
      { // nicType
         nicAddr->nicType = (NicAddrType_t) get_unaligned((char*)currentNicListPos);
         currentNicListPos += 1;
      }

      { // 3 bytes padding (for 4 byte alignment)
         currentNicListPos += 3;
      }


      NicAddressList_append(outNicList, nicAddr);
   }

}

/**
 * note: 4 byte aligned
 */
unsigned Serialization_serialLenNicList(NicAddressList* nicList)
{
   // elem count info field + elements
   return Serialization_serialLenUInt() +
      NicAddressList_length(nicList) * SERIALIZATION_NICLISTELEM_SIZE;
}

/**
 * Serialization of a NodeList.
 *
 * note: 8 byte aligned.
 *
 * @return 0 on error, used buffer size otherwise
 */
unsigned Serialization_serializeNodeList(char* buf, NodeList* nodeList)
{
   unsigned nodeListSize = NodeList_length(nodeList);
   NodeListIter iter;
   unsigned i;
   
   size_t bufPos = 0;

   // elem count info field

   bufPos += Serialization_serializeUInt(&buf[bufPos], nodeListSize);

   // padding for 8 byte alignment

   bufPos += Serialization_serialLenUInt();


   // serialize each element of the nodeList

   NodeListIter_init(&iter, nodeList);

   for(i=0; i < nodeListSize; i++, NodeListIter_next(&iter) )
   {
      unsigned nodeStartBufPos = bufPos; // for 8 byte alignment of this node

      Node* node = NodeListIter_value(&iter);

      const BitStore* nodeFeatureFlags = Node_getNodeFeatures(node);
      NodeConnPool* connPool = Node_getConnPool(node);
      const char* nodeID = Node_getID(node);
      NicAddressList* nicList = NodeConnPool_getNicList(connPool);
      unsigned fhgfsVersion = Node_getFhgfsVersion(node);
      uint16_t nodeNumID = Node_getNumID(node);
      uint16_t portUDP = Node_getPortUDP(node);
      uint16_t portTCP = Node_getPortTCP(node);
      char nodeType = (char)Node_getNodeType(node);

      // nodeFeatureFlags (8b aligned)
      bufPos += BitStore_serialize(nodeFeatureFlags, &buf[bufPos] );

      // nodeID
      bufPos += Serialization_serializeStrAlign4(&buf[bufPos], os_strlen(nodeID), nodeID);

      // nicList (4b aligned)
      bufPos += Serialization_serializeNicList(&buf[bufPos], nicList);

      // fhgfsVersion
      bufPos += Serialization_serializeUInt(&buf[bufPos], fhgfsVersion);

      // nodeNumID
      bufPos += Serialization_serializeUShort(&buf[bufPos], nodeNumID);

      // portUDP
      bufPos += Serialization_serializeUShort(&buf[bufPos], portUDP);

      // portTCP
      bufPos += Serialization_serializeUShort(&buf[bufPos], portTCP);

      // nodeType
      bufPos += Serialization_serializeChar(&buf[bufPos], nodeType);


      { // add padding for 8 byte alignment
         unsigned remainder = (bufPos - nodeStartBufPos) % 8;
         if(remainder)
            bufPos += (8 - remainder);
      }
   }

   NodeListIter_uninit(&iter);
   
   return bufPos;
}

/**
 * Pre-processes a serialized NodeList for deserialization via deserializeNodeList().
 *
 * @return fhgfs_false on error or inconsistency
 */
fhgfs_bool Serialization_deserializeNodeListPreprocess(const char* buf, size_t bufLen,
   unsigned* outNodeListElemNum, const char** outNodeListStart, unsigned* outLen)
{
   unsigned i;   
   
   size_t bufPos = 0;

   // elem count info field
   unsigned elemNumFieldLen;

   if(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, outNodeListElemNum,
      &elemNumFieldLen) )
      return fhgfs_false;

   bufPos += elemNumFieldLen;

   // padding for 8 byte alignment

   bufPos += Serialization_serialLenUInt();

   // store start pos for later deserialize()

   *outNodeListStart = &buf[bufPos];

   for(i=0; i < *outNodeListElemNum; i++)
   {
      unsigned nodeStartBufPos = bufPos; // for 8 byte alignment of this node

      { // nodeFeatureFlags (8b aligned)
         unsigned featuresBufLen;
         const char* featuresStart;

         if(unlikely(!BitStore_deserializePreprocess(&buf[bufPos], bufLen-bufPos, &featuresStart,
            &featuresBufLen) ) )
            return fhgfs_false;

         bufPos += featuresBufLen;
      }

      {// nodeID
         unsigned nodeIDLen = 0;
         const char* nodeID = NULL;
         unsigned idBufLen = 0;

         if(unlikely(!Serialization_deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
            &nodeIDLen, &nodeID, &idBufLen) ) )
            return fhgfs_false;

         bufPos += idBufLen;
      }

      {// nicList (4b aligned)
         unsigned nicListElemNum = 0;
         const char* nicListStart = NULL;
         unsigned nicListBufLen = 0;

         if(unlikely(!Serialization_deserializeNicListPreprocess(&buf[bufPos], bufLen-bufPos,
            &nicListElemNum, &nicListStart, &nicListBufLen) ) )
            return fhgfs_false;

         bufPos += nicListBufLen;
      }

      {// fhgfsVersion
         unsigned fhgfsVersion = 0;
         unsigned versionBufLen = 0;

         if(unlikely(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, &fhgfsVersion,
            &versionBufLen) ) )
            return fhgfs_false;

         bufPos += versionBufLen;
      }

      {// nodeNumID
         uint16_t nodeNumID = 0;
         unsigned nodeNumIDBufLen = 0;

         if(unlikely(!Serialization_deserializeUShort(&buf[bufPos], bufLen-bufPos, &nodeNumID,
            &nodeNumIDBufLen) ) )
            return fhgfs_false;

         bufPos += nodeNumIDBufLen;
      }

      {// portUDP
         uint16_t portUDP = 0;
         unsigned udpBufLen = 0;

         if(unlikely(!Serialization_deserializeUShort(&buf[bufPos], bufLen-bufPos, &portUDP,
            &udpBufLen) ) )
            return fhgfs_false;

         bufPos += udpBufLen;
      }

      {// portTCP
         uint16_t portTCP = 0;
         unsigned tcpBufLen = 0;

         if(unlikely(!Serialization_deserializeUShort(&buf[bufPos], bufLen-bufPos, &portTCP,
            &tcpBufLen) ) )
            return fhgfs_false;

         bufPos += tcpBufLen;
      }

      {// nodeType
         char nodeType = 0;
         unsigned nodeTypeBufLen = 0;

         if(unlikely(!Serialization_deserializeChar(&buf[bufPos], bufLen-bufPos, &nodeType,
            &nodeTypeBufLen) ) )
            return fhgfs_false;

         bufPos += nodeTypeBufLen;
      }


      { // add padding for 8 byte alignment
         unsigned remainder = (bufPos - nodeStartBufPos) % 8;
         if(remainder)
            bufPos += (8 - remainder);
      }
   }

   // check buffer overrun
   if(unlikely(bufLen < bufPos) )
      return fhgfs_false;


   *outLen = bufPos;

   return fhgfs_true;
}

/**
 * Deserializes a NodeList.
 * (requires pre-processing)
 *
 * Note: Nodes will be constructed and need to be deleted later
 */
void Serialization_deserializeNodeList(App* app, unsigned nodeListElemNum,
   const char* nodeListStart, NodeList* outNodeList)
{
   unsigned i;
   
   size_t bufPos = 0;
   size_t bufLen = ~0; // fake bufLen to max value (has already been verified during pre-processing)
   const char* buf = nodeListStart;


   for(i=0; i < nodeListElemNum; i++)
   {
      unsigned nodeStartBufPos = bufPos; // for 8 byte alignment of this node

      BitStore nodeFeatureFlags;
      NicAddressList nicList;

      const char* nodeID = NULL;
      unsigned fhgfsVersion = 0;
      uint16_t nodeNumID = 0;
      uint16_t portUDP = 0;
      uint16_t portTCP = 0;
      char nodeType = 0;

      
      BitStore_init(&nodeFeatureFlags, fhgfs_false);
      NicAddressList_init(&nicList);


      {//nodeFeatureFlags (8b aligned)
         unsigned featuresBufLen = 0;

         BitStore_deserialize(&nodeFeatureFlags, &buf[bufPos], &featuresBufLen);

         bufPos += featuresBufLen;
      }

      {// nodeID
         unsigned idBufLen = 0;
         unsigned nodeIDLen = 0;

         Serialization_deserializeStrAlign4(&buf[bufPos], bufLen-bufPos, &nodeIDLen, &nodeID,
            &idBufLen);

         bufPos += idBufLen;
      }

      {// nicList (4b aligned)
         unsigned nicListBufLen = 0;
         unsigned nicListElemNum = 0;
         const char* nicListStart = NULL;

         Serialization_deserializeNicListPreprocess(&buf[bufPos], bufLen-bufPos,
            &nicListElemNum, &nicListStart, &nicListBufLen);

         bufPos += nicListBufLen;

         Serialization_deserializeNicList(nicListElemNum, nicListStart, &nicList);
      }

      {// fhgfsVersion
         unsigned versionBufLen = 0;

         Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, &fhgfsVersion, &versionBufLen);

         bufPos += versionBufLen;
      }

      {// nodeNumID
         unsigned nodeNumIDBufLen = 0;

         Serialization_deserializeUShort(&buf[bufPos], bufLen-bufPos, &nodeNumID, &nodeNumIDBufLen);

         bufPos += nodeNumIDBufLen;
      }

      {// portUDP
         unsigned udpBufLen = 0;

         Serialization_deserializeUShort(&buf[bufPos], bufLen-bufPos, &portUDP, &udpBufLen);

         bufPos += udpBufLen;
      }

      {// portTCP
         unsigned tcpBufLen = 0;

         Serialization_deserializeUShort(&buf[bufPos], bufLen-bufPos, &portTCP, &tcpBufLen);

         bufPos += tcpBufLen;
      }

      {// nodeType
         unsigned nodeTypeBufLen = 0;

         Serialization_deserializeChar(&buf[bufPos], bufLen-bufPos, &nodeType, &nodeTypeBufLen);

         bufPos += nodeTypeBufLen;
      }

      { // add padding for 8 byte alignment
         unsigned remainder = (bufPos - nodeStartBufPos) % 8;
         if(remainder)
            bufPos += (8 - remainder);
      }

      {// construct node
         Node* node = Node_construct(app, nodeID, nodeNumID, portUDP, portTCP, &nicList);

         Node_setNodeType(node, (NodeType)nodeType);
         Node_setFhgfsVersion(node, fhgfsVersion);
         Node_setFeatureFlags(node, &nodeFeatureFlags);

         // append node to outList
         NodeList_append(outNodeList, node);
      }

      // cleanup
      ListTk_kfreeNicAddressListElems(&nicList);
      NicAddressList_uninit(&nicList);
      BitStore_uninit(&nodeFeatureFlags);
   }

}

/**
 * note: 8 byte aligned.
 */
unsigned Serialization_serialLenNodeList(NodeList* nodeList)
{
   unsigned nodeListSize = NodeList_length(nodeList);
   NodeListIter iter;
   unsigned i;
   
   unsigned bufPos = 0;

   // elem count info field

   bufPos += Serialization_serialLenUInt();

   // padding for 8 byte alignment

   bufPos += Serialization_serialLenUInt();


   // serialize each element of the nodeList

   NodeListIter_init(&iter, nodeList);

   for(i=0; i < nodeListSize; i++, NodeListIter_next(&iter) )
   {
      unsigned nodeStartBufPos = bufPos; // for 8 byte alignment of this node

      Node* node = NodeListIter_value(&iter);

      const BitStore* nodeFeatureFlags = Node_getNodeFeatures(node);
      const char* nodeID = Node_getID(node);
      NodeConnPool* connPool = Node_getConnPool(node);
      NicAddressList* nicList = NodeConnPool_getNicList(connPool);

      // nodeFeatureFlags (8b aligned)
      bufPos += BitStore_serialLen(nodeFeatureFlags);

      // nodeID
      bufPos += Serialization_serialLenStrAlign4(os_strlen(nodeID) );

      // nicList (4b algined)
      bufPos += Serialization_serialLenNicList(nicList);

      // fhgfsVersion
      bufPos += Serialization_serialLenUInt();

      // nodeNumID
      bufPos += Serialization_serialLenUShort();

      // portUDP
      bufPos += Serialization_serialLenUShort();

      // portTCP
      bufPos += Serialization_serialLenUShort();

      // nodeType
      bufPos += Serialization_serialLenChar();


      { // add padding for 8 byte alignment
         unsigned remainder = (bufPos - nodeStartBufPos) % 8;
         if(remainder)
            bufPos += (8 - remainder);
      }
   }
   
   NodeListIter_uninit(&iter);

   return bufPos;
}



/**
 * Serialization of a StrCpyList.
 *
 * Note: We keep the serialiazation format compatible with string vec serialization
 */
unsigned Serialization_serializeStrCpyList(char* buf, StrCpyList* list)
{
   StrCpyListIter iter;

   size_t requiredLen = Serialization_serialLenStrCpyList(list);
   size_t listSize = StrCpyList_length(list);
   size_t i;

   size_t bufPos = 0;

   // totalBufLen info field

   bufPos += Serialization_serializeUInt(&buf[bufPos], (unsigned)requiredLen);

   // elem count info field

   bufPos += Serialization_serializeUInt(&buf[bufPos], listSize);


   // store each element of the list as a raw zero-terminated string

   StrCpyListIter_init(&iter, list);

   for(i=0; i < listSize; i++, StrCpyListIter_next(&iter) )
   {
      char* currentElem = StrCpyListIter_value(&iter);

      size_t serialElemLen = os_strlen(currentElem) + 1; // +1 for the terminating zero

      os_memcpy(&buf[bufPos], currentElem, serialElemLen);

      bufPos += serialElemLen;
   }

   StrCpyListIter_uninit(&iter);

   return requiredLen;
}

/**
 * Pre-processes a serialized StrCpyList.
 *
 * @return fhgfs_false on error or inconsistency
 */
fhgfs_bool Serialization_deserializeStrCpyListPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen)
{
   size_t bufPos = 0;

   const char* outListEnd;

   unsigned bufLenFieldLen = 0; // "=0" to mute false compiler warning
   unsigned elemNumFieldLen = 0; // "=0" to mute false compiler warning

   // totalBufLen info field
   if(unlikely(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, outLen,
      &bufLenFieldLen) ) )
      return fhgfs_false;
   bufPos += bufLenFieldLen;

   // elem count field
   if(unlikely(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, outElemNum,
      &elemNumFieldLen) ) )
      return fhgfs_false;
   bufPos += elemNumFieldLen;

   *outListStart = &buf[bufPos];

   outListEnd = &buf[*outLen] - 1;

   if(unlikely(
       (*outLen > bufLen) ||
       (*outLen < (2 * Serialization_serialLenUInt() ) ) || // bufLenField + numElemsField
       (*outElemNum && (*outListEnd != 0) ) ) )
      return fhgfs_false;

   return fhgfs_true;
}

/**
 * Deserializes a StrCpyList.
 * (requires pre-processing)
 *
 * @return fhgfs_false on error or inconsistency
 */
fhgfs_bool Serialization_deserializeStrCpyList(unsigned listBufLen, unsigned elemNum,
   const char* listStart, StrCpyList* outList)
{
   const char* currentPos = listStart;
   unsigned elemsBufLen;
   const char* listEndPos;
   unsigned lastElemLen;
   unsigned i;

   elemsBufLen =
      listBufLen - Serialization_serialLenUInt() -
      Serialization_serialLenUInt(); // bufLenField & numElemsField

   listEndPos = listStart + elemsBufLen - 1;


   // read each list element as a raw zero-terminated string
   // and make sure that we do not read beyond the specified end position

   for(i=0; (i < elemNum) && (currentPos <= listEndPos); i++, currentPos += lastElemLen)
   {
      const char* currentElem = currentPos;

      StrCpyList_append(outList, currentElem);

      lastElemLen = os_strlen(currentElem) + 1; // +1 for the terminating zero
   }

   // check whether all of the elements were read (=consistency)
   return ( (i==elemNum) && (currentPos > listEndPos) );
}



unsigned Serialization_serialLenStrCpyList(StrCpyList* list)
{
   StrCpyListIter iter;

   // bufLen-field + numElems-field
   unsigned requiredLen = Serialization_serialLenUInt() + Serialization_serialLenUInt();

   StrCpyListIter_init(&iter, list);

   for( ; !StrCpyListIter_end(&iter); StrCpyListIter_next(&iter) )
   {
      char* currentElem = StrCpyListIter_value(&iter);

      requiredLen += os_strlen(currentElem) + 1; // +1 for the terminating zero
   }

   StrCpyListIter_uninit(&iter);

   return requiredLen;
}

/**
 * Serialization of a StrCpyVec.
 *
 * Note: We keep the serialiazation format compatible with string list serialization
 */
unsigned Serialization_serializeStrCpyVec(char* buf, StrCpyVec* vec)
{
   size_t requiredLen = Serialization_serialLenStrCpyVec(vec);
   size_t listSize = StrCpyVec_length(vec);
   size_t i;

   size_t bufPos = 0;

   // totalBufLen info field

   bufPos += Serialization_serializeUInt(&buf[bufPos], (unsigned)requiredLen);

   // elem count info field

   bufPos += Serialization_serializeUInt(&buf[bufPos], listSize);


   // store each element of the list as a raw zero-terminated string

   for(i=0; i < listSize; i++)
   {
      char* currentElem = StrCpyVec_at(vec, i);

      size_t serialElemLen = os_strlen(currentElem) + 1; // +1 for the terminating zero

      os_memcpy(&buf[bufPos], currentElem, serialElemLen);

      bufPos += serialElemLen;
   }

   return requiredLen;
}

/**
 * Pre-processes a serialized StrCpyVec.
 *
 * Note: We keep the serialization format compatible to the string list format.
 *
 * @return fhgfs_false on error or inconsistency
 */
fhgfs_bool Serialization_deserializeStrCpyVecPreprocess(const char* buf,
   size_t bufLen, unsigned* outElemNum, const char** outListStart, unsigned* outLen)
{
   // serialization format is 100% compatible to the list version, so we can just use the list
      // version code here...

   return Serialization_deserializeStrCpyListPreprocess(buf, bufLen,
      outElemNum, outListStart, outLen);
}

/**
 * Deserializes a StrCpyVec.
 * (requires pre-processing)
 *
 * @return fhgfs_false on error or inconsistency
 */
fhgfs_bool Serialization_deserializeStrCpyVec(unsigned listBufLen, unsigned elemNum,
   const char* listStart, StrCpyVec* outVec)
{
   const char* currentPos = listStart;
   unsigned elemsBufLen;
   const char* listEndPos;
   unsigned lastElemLen;
   unsigned i;

   elemsBufLen =
      listBufLen - Serialization_serialLenUInt() -
      Serialization_serialLenUInt(); // bufLenField & numElemsField

   listEndPos = listStart + elemsBufLen - 1;


   // read each list element as a raw zero-terminated string
   // and make sure that we do not read beyond the specified end position

   for(i=0; (i < elemNum) && (currentPos <= listEndPos); i++, currentPos += lastElemLen)
   {
      const char* currentElem = currentPos;

      StrCpyVec_append(outVec, currentElem);

      lastElemLen = os_strlen(currentElem) + 1; // +1 for the terminating zero
   }

   // check whether all of the elements were read (=consistency)
   return ( (i==elemNum) && (currentPos > listEndPos) );
}



unsigned Serialization_serialLenStrCpyVec(StrCpyVec* vec)
{
   size_t i = 0;
   size_t vecLen = StrCpyVec_length(vec);

   // bufLen-field + numElems-field
   unsigned requiredLen = Serialization_serialLenUInt() + Serialization_serialLenUInt();

   for( ; i < vecLen; i++)
   {
      char* currentElem = StrCpyVec_at(vec, i);

      requiredLen += os_strlen(currentElem) + 1; // +1 for the terminating zero
   }

   return requiredLen;
}


/**
 * Serialization of a IntList.
 *
 * Note: We keep the serialization format compatible with int vec serialization
 */
unsigned Serialization_serializeIntCpyList(char* buf, IntCpyList* list)
{
   IntCpyListIter iter;
   unsigned i;

   unsigned requiredLen = Serialization_serialLenIntCpyList(list);
   unsigned listSize = IntCpyList_length(list);

   size_t bufPos = 0;

   // totalBufLen info field

   bufPos += Serialization_serializeUInt(&buf[bufPos], requiredLen);

   // elem count info field

   bufPos += Serialization_serializeUInt(&buf[bufPos], listSize);


   // store each element of the list

   IntCpyListIter_init(&iter, list);

   for(i=0; i < listSize; i++, IntCpyListIter_next(&iter) )
   {
      int currentValue = IntCpyListIter_value(&iter);

      bufPos += Serialization_serializeInt(&buf[bufPos], currentValue);
   }

   IntCpyListIter_uninit(&iter);

   return requiredLen;
}

/**
 * Pre-processes a serialized IntList().
 *
 * @return fhgfs_false on error or inconsistency
 */
fhgfs_bool Serialization_deserializeIntCpyListPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen)
{
   size_t bufPos = 0;

   unsigned bufLenFieldLen = 0; // "=0" to mute false compiler warning
   unsigned elemNumFieldLen = 0; // "=0" to mute false compiler warning

   // totalBufLen info field
   if(unlikely(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, outLen,
      &bufLenFieldLen) ) )
      return fhgfs_false;
   bufPos += bufLenFieldLen;

   // elem count field
   if(unlikely(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, outElemNum,
      &elemNumFieldLen) ) )
      return fhgfs_false;
   bufPos += elemNumFieldLen;

   *outListStart = &buf[bufPos];

   if( (*outLen > bufLen) ||
      ( (*outLen - bufPos) != (*outElemNum * Serialization_serialLenInt() ) ) )
      return fhgfs_false;

   return fhgfs_true;
}

/**
 * Deserializes a IntList.
 * (requires pre-processing)
 *
 * @return fhgfs_false on error or inconsistency
 */
fhgfs_bool Serialization_deserializeIntCpyList(unsigned listBufLen, unsigned elemNum,
   const char* listStart, IntCpyList* outList)
{
   unsigned i;

   unsigned elemsBufLen =
      listBufLen - Serialization_serialLenUInt() -
      Serialization_serialLenUInt(); // bufLenField & numElemsField

   size_t bufPos = 0;

   // read each list element

   for(i=0; i < elemNum; i++)
   {
      int value;
      unsigned valueLen;

      if(!Serialization_deserializeInt(&listStart[bufPos], elemsBufLen-bufPos, &value, &valueLen) )
         return fhgfs_false;

      bufPos += valueLen;

      IntCpyList_append(outList, value);
   }

   return fhgfs_true;
}



unsigned Serialization_serialLenIntCpyList(IntCpyList* list)
{
   // bufLen-field + numElems-field + numElems*elemSize
   unsigned requiredLen = Serialization_serialLenUInt() + Serialization_serialLenUInt() +
         IntCpyList_length(list)*Serialization_serialLenInt();

   return requiredLen;
}

/**
 * Note: We keep the serialiazation format compatible with string list serialization
 */
unsigned Serialization_serializeIntCpyVec(char* buf, IntCpyVec* vec)
{
   // IntCpyVecs are derived from ListCpyLists, so we can just do a cast here and use the list
      // version code...

   return Serialization_serializeIntCpyList(buf, (IntCpyList*)vec);
}

/**
 * Pre-processes a serialized IntVec().
 *
 * @return fhgfs_false on error or inconsistency
 */
fhgfs_bool Serialization_deserializeIntCpyVecPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen)
{
   // serialization format is 100% compatible to the list version, so we can just use the list
      // version code here...

   return Serialization_deserializeIntCpyListPreprocess(buf, bufLen,
      outElemNum, outListStart, outLen);
}

/**
 * Deserializes a IntVec.
 * (requires pre-processing)
 *
 * @return fhgfs_false on error or inconsistency
 */
fhgfs_bool Serialization_deserializeIntCpyVec(unsigned listBufLen, unsigned elemNum,
   const char* listStart, IntCpyVec* outVec)
{
   unsigned i;

   unsigned elemsBufLen =
      listBufLen - Serialization_serialLenUInt() -
      Serialization_serialLenUInt(); // bufLenField & numElemsField

   size_t bufPos = 0;

   // read each list element

   for(i=0; i < elemNum; i++)
   {
      int value;
      unsigned valueLen;

      if(!Serialization_deserializeInt(&listStart[bufPos], elemsBufLen-bufPos, &value, &valueLen) )
         return fhgfs_false;

      bufPos += valueLen;

      IntCpyVec_append(outVec, value);
   }

   return fhgfs_true;
}

unsigned Serialization_serialLenIntCpyVec(IntCpyVec* vec)
{
   // IntCpyVecs are derived from IntCpyLists and serialization format for both is 100% compatible,
      // so we can just do a cast here and use the list version code...

   return Serialization_serialLenIntCpyList( (IntCpyList*)vec);
}

/**
 * Serialization of a UInt8List.
 *
 * Note: We keep the serialization format compatible with int vec serialization
 */
unsigned Serialization_serializeUInt8List(char* buf, UInt8List* list)
{
   UInt8ListIter iter;
   unsigned i;

   unsigned requiredLen = Serialization_serialLenUInt8List(list);
   unsigned listSize = UInt8List_length(list);

   size_t bufPos = 0;

   // totalBufLen info field

   bufPos += Serialization_serializeUInt(&buf[bufPos], requiredLen);

   // elem count info field

   bufPos += Serialization_serializeUInt(&buf[bufPos], listSize);


   // store each element of the list

   UInt8ListIter_init(&iter, list);

   for(i=0; i < listSize; i++, UInt8ListIter_next(&iter) )
   {
      uint8_t currentValue = UInt8ListIter_value(&iter);

      bufPos += Serialization_serializeUInt8(&buf[bufPos], currentValue);
   }

   UInt8ListIter_uninit(&iter);

   return requiredLen;
}

/**
 * Pre-processes a serialized UInt8List().
 *
 * @return fhgfs_false on error or inconsistency
 */
fhgfs_bool Serialization_deserializeUInt8ListPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen)
{
   size_t bufPos = 0;

   unsigned bufLenFieldLen = 0; // "=0" to mute false compiler warning
   unsigned elemNumFieldLen = 0; // "=0" to mute false compiler warning

   // totalBufLen info field
   if(unlikely(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, outLen,
      &bufLenFieldLen) ) )
      return fhgfs_false;
   bufPos += bufLenFieldLen;

   // elem count field
   if(unlikely(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, outElemNum,
      &elemNumFieldLen) ) )
      return fhgfs_false;
   bufPos += elemNumFieldLen;

   *outListStart = &buf[bufPos];

   if( (*outLen > bufLen) ||
      ( (*outLen - bufPos) != (*outElemNum * Serialization_serialLenUInt8() ) ) )
      return fhgfs_false;

   return fhgfs_true;
}

/**
 * Deserializes a UInt8List.
 * (requires pre-processing)
 *
 * @return fhgfs_false on error or inconsistency
 */
fhgfs_bool Serialization_deserializeUInt8List(unsigned listBufLen, unsigned elemNum,
   const char* listStart, UInt8List* outList)
{
   unsigned i;

   unsigned elemsBufLen =
      listBufLen - Serialization_serialLenUInt() -
      Serialization_serialLenUInt(); // bufLenField & numElemsField

   size_t bufPos = 0;

   // read each list element

   for(i=0; i < elemNum; i++)
   {
      uint8_t value;
      unsigned valueLen;

      if(!Serialization_deserializeUInt8(&listStart[bufPos], elemsBufLen-bufPos,
         &value, &valueLen) )
         return fhgfs_false;

      bufPos += valueLen;

      UInt8List_append(outList, value);
   }

   return fhgfs_true;
}



unsigned Serialization_serialLenUInt8List(UInt8List* list)
{
   // bufLen-field + numElems-field + numElems*elemSize
   unsigned requiredLen = Serialization_serialLenUInt() + Serialization_serialLenUInt() +
         UInt8List_length(list)*Serialization_serialLenUInt8();

   return requiredLen;
}


/**
 * Note: We keep the serialiazation format compatible with string list serialization
 */
unsigned Serialization_serializeUInt8Vec(char* buf, UInt8Vec* vec)
{
   // UInt8Vecs are derived from UInt8Lists, so we can just do a cast here and use the list
      // version code...

   return Serialization_serializeUInt8List(buf, (UInt8List*)vec);
}

/**
 * Pre-processes a serialized UInt8Vec().
 *
 * @return fhgfs_false on error or inconsistency
 */
fhgfs_bool Serialization_deserializeUInt8VecPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen)
{
   // serialization format is 100% compatible to the list version, so we can just use the list
      // version code here...

   return Serialization_deserializeUInt8ListPreprocess(buf, bufLen,
      outElemNum, outListStart, outLen);
}

/**
 * Deserializes a UInt8Vec.
 * (requires pre-processing)
 *
 * @return fhgfs_false on error or inconsistency
 */
fhgfs_bool Serialization_deserializeUInt8Vec(unsigned listBufLen, unsigned elemNum,
   const char* listStart, UInt8Vec* outVec)
{
   unsigned i;

   unsigned elemsBufLen =
      listBufLen - Serialization_serialLenUInt() -
      Serialization_serialLenUInt(); // bufLenField & numElemsField

   size_t bufPos = 0;

   // read each list element

   for(i=0; i < elemNum; i++)
   {
      uint8_t value;
      unsigned valueLen;

      if(!Serialization_deserializeUInt8(&listStart[bufPos], elemsBufLen-bufPos,
         &value, &valueLen) )
         return fhgfs_false;

      bufPos += valueLen;

      UInt8Vec_append(outVec, value);
   }

   return fhgfs_true;
}

unsigned Serialization_serialLenUInt8Vec(UInt8Vec* vec)
{
   // UInt8Vecs are derived from UInt8Lists and serialization format for both is 100% compatible,
      // so we can just do a cast here and use the list version code...

   return Serialization_serialLenUInt8List( (UInt8List*)vec);
}


/**
 * Serialization of a UInt16List.
 *
 * Note: We keep the serialization format compatible with int vec serialization
 */
unsigned Serialization_serializeUInt16List(char* buf, UInt16List* list)
{
   UInt16ListIter iter;
   unsigned i;

   unsigned requiredLen = Serialization_serialLenUInt16List(list);
   unsigned listSize = UInt16List_length(list);

   size_t bufPos = 0;

   // totalBufLen info field

   bufPos += Serialization_serializeUInt(&buf[bufPos], requiredLen);

   // elem count info field

   bufPos += Serialization_serializeUInt(&buf[bufPos], listSize);


   // store each element of the list

   UInt16ListIter_init(&iter, list);

   for(i=0; i < listSize; i++, UInt16ListIter_next(&iter) )
   {
      uint16_t currentValue = UInt16ListIter_value(&iter);

      bufPos += Serialization_serializeUShort(&buf[bufPos], currentValue);
   }

   UInt16ListIter_uninit(&iter);

   return requiredLen;
}

/**
 * Pre-processes a serialized UInt16List().
 *
 * @return fhgfs_false on error or inconsistency
 */
fhgfs_bool Serialization_deserializeUInt16ListPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen)
{
   size_t bufPos = 0;

   unsigned bufLenFieldLen = 0; // "=0" to mute false compiler warning
   unsigned elemNumFieldLen = 0; // "=0" to mute false compiler warning

   // totalBufLen info field
   if(unlikely(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, outLen,
      &bufLenFieldLen) ) )
      return fhgfs_false;
   bufPos += bufLenFieldLen;

   // elem count field
   if(unlikely(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, outElemNum,
      &elemNumFieldLen) ) )
      return fhgfs_false;
   bufPos += elemNumFieldLen;

   *outListStart = &buf[bufPos];

   if( (*outLen > bufLen) ||
      ( (*outLen - bufPos) != (*outElemNum * Serialization_serialLenUShort() ) ) )
      return fhgfs_false;

   return fhgfs_true;
}

/**
 * Deserializes a UInt16List.
 * (requires pre-processing)
 *
 * @return fhgfs_false on error or inconsistency
 */
fhgfs_bool Serialization_deserializeUInt16List(unsigned listBufLen, unsigned elemNum,
   const char* listStart, UInt16List* outList)
{
   unsigned i;

   unsigned elemsBufLen =
      listBufLen - Serialization_serialLenUInt() -
      Serialization_serialLenUInt(); // bufLenField & numElemsField

   size_t bufPos = 0;

   // read each list element

   for(i=0; i < elemNum; i++)
   {
      uint16_t value;
      unsigned valueLen;

      if(!Serialization_deserializeUShort(&listStart[bufPos], elemsBufLen-bufPos,
         &value, &valueLen) )
         return fhgfs_false;

      bufPos += valueLen;

      UInt16List_append(outList, value);
   }

   return fhgfs_true;
}



unsigned Serialization_serialLenUInt16List(UInt16List* list)
{
   // bufLen-field + numElems-field + numElems*elemSize
   unsigned requiredLen = Serialization_serialLenUInt() + Serialization_serialLenUInt() +
         UInt16List_length(list)*Serialization_serialLenUShort();

   return requiredLen;
}

/**
 * Note: We keep the serialiazation format compatible with string list serialization
 */
unsigned Serialization_serializeUInt16Vec(char* buf, UInt16Vec* vec)
{
   // UInt16Vecs are derived from UInt16Lists, so we can just do a cast here and use the list
      // version code...

   return Serialization_serializeUInt16List(buf, (UInt16List*)vec);
}

/**
 * Pre-processes a serialized UInt16Vec().
 *
 * @return fhgfs_false on error or inconsistency
 */
fhgfs_bool Serialization_deserializeUInt16VecPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen)
{
   // serialization format is 100% compatible to the list version, so we can just use the list
      // version code here...

   return Serialization_deserializeUInt16ListPreprocess(buf, bufLen,
      outElemNum, outListStart, outLen);
}

/**
 * Deserializes a UInt16Vec.
 * (requires pre-processing)
 *
 * @return fhgfs_false on error or inconsistency
 */
fhgfs_bool Serialization_deserializeUInt16Vec(unsigned listBufLen, unsigned elemNum,
   const char* listStart, UInt16Vec* outVec)
{
   unsigned i;

   unsigned elemsBufLen =
      listBufLen - Serialization_serialLenUInt() -
      Serialization_serialLenUInt(); // bufLenField & numElemsField

   size_t bufPos = 0;

   // read each list element

   for(i=0; i < elemNum; i++)
   {
      uint16_t value;
      unsigned valueLen;

      if(!Serialization_deserializeUShort(&listStart[bufPos], elemsBufLen-bufPos,
         &value, &valueLen) )
         return fhgfs_false;

      bufPos += valueLen;

      UInt16Vec_append(outVec, value);
   }

   return fhgfs_true;
}

unsigned Serialization_serialLenUInt16Vec(UInt16Vec* vec)
{
   // UInt16Vecs are derived from UInt16Lists and serialization format for both is 100% compatible,
      // so we can just do a cast here and use the list version code...

   return Serialization_serialLenUInt16List( (UInt16List*)vec);
}

/**
 * Serialization of a Int64CpyList.
 *
 * Note: We keep the serialization format compatible with int vec serialization
 */
unsigned Serialization_serializeInt64CpyList(char* buf, Int64CpyList* list)
{
   Int64CpyListIter iter;
   unsigned i;

   unsigned requiredLen = Serialization_serialLenInt64CpyList(list);
   unsigned listSize = Int64CpyList_length(list);

   size_t bufPos = 0;

   // totalBufLen info field

   bufPos += Serialization_serializeUInt(&buf[bufPos], requiredLen);

   // elem count info field

   bufPos += Serialization_serializeUInt(&buf[bufPos], listSize);


   // store each element of the list

   Int64CpyListIter_init(&iter, list);

   for(i=0; i < listSize; i++, Int64CpyListIter_next(&iter) )
   {
      int64_t currentValue = Int64CpyListIter_value(&iter);

      bufPos += Serialization_serializeInt64(&buf[bufPos], currentValue);
   }

   Int64CpyListIter_uninit(&iter);

   return requiredLen;
}

/**
 * Pre-processes a serialized Int64CpyList().
 *
 * @return fhgfs_false on error or inconsistency
 */
fhgfs_bool Serialization_deserializeInt64CpyListPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen)
{
   size_t bufPos = 0;

   unsigned bufLenFieldLen = 0; // "=0" to mute false compiler warning
   unsigned elemNumFieldLen = 0; // "=0" to mute false compiler warning

   // totalBufLen info field
   if(unlikely(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, outLen,
      &bufLenFieldLen) ) )
      return fhgfs_false;
   bufPos += bufLenFieldLen;

   // elem count field
   if(unlikely(!Serialization_deserializeUInt(&buf[bufPos], bufLen-bufPos, outElemNum,
      &elemNumFieldLen) ) )
      return fhgfs_false;
   bufPos += elemNumFieldLen;

   *outListStart = &buf[bufPos];

   if( (*outLen > bufLen) ||
      ( (*outLen - bufPos) != (*outElemNum * Serialization_serialLenInt64() ) ) )
      return fhgfs_false;

   return fhgfs_true;
}

/**
 * Deserializes a Int64CpyList.
 * (requires pre-processing)
 *
 * @return fhgfs_false on error or inconsistency
 */
fhgfs_bool Serialization_deserializeInt64CpyList(unsigned listBufLen, unsigned elemNum,
   const char* listStart, Int64CpyList* outList)
{
   unsigned i;

   unsigned elemsBufLen =
      listBufLen - Serialization_serialLenUInt() -
      Serialization_serialLenUInt(); // bufLenField & numElemsField

   size_t bufPos = 0;

   // read each list element

   for(i=0; i < elemNum; i++)
   {
      int64_t value;
      unsigned valueLen;

      if(!Serialization_deserializeInt64(&listStart[bufPos], elemsBufLen-bufPos,
         &value, &valueLen) )
         return fhgfs_false;

      bufPos += valueLen;

      Int64CpyList_append(outList, value);
   }

   return fhgfs_true;
}



unsigned Serialization_serialLenInt64CpyList(Int64CpyList* list)
{
   // bufLen-field + numElems-field + numElems*elemSize
   unsigned requiredLen = Serialization_serialLenUInt() + Serialization_serialLenUInt() +
         Int64CpyList_length(list)*Serialization_serialLenInt64();

   return requiredLen;
}


/**
 * Note: We keep the serialiazation format compatible with string list serialization
 */
unsigned Serialization_serializeInt64CpyVec(char* buf, Int64CpyVec* vec)
{
   // Int64CpyVecs are derived from Int64CpyLists, so we can just do a cast here and use the list
      // version code...

   return Serialization_serializeInt64CpyList(buf, (Int64CpyList*)vec);
}

/**
 * Pre-processes a serialized Int64CpyVec().
 *
 * @return fhgfs_false on error or inconsistency
 */
fhgfs_bool Serialization_deserializeInt64CpyVecPreprocess(const char* buf, size_t bufLen,
   unsigned* outElemNum, const char** outListStart, unsigned* outLen)
{
   // serialization format is 100% compatible to the list version, so we can just use the list
      // version code here...

   return Serialization_deserializeInt64CpyListPreprocess(buf, bufLen,
      outElemNum, outListStart, outLen);
}

/**
 * Deserializes a Int64CpyVec.
 * (requires pre-processing)
 *
 * @return fhgfs_false on error or inconsistency
 */
fhgfs_bool Serialization_deserializeInt64CpyVec(unsigned listBufLen, unsigned elemNum,
   const char* listStart, Int64CpyVec* outVec)
{
   unsigned i;

   unsigned elemsBufLen =
      listBufLen - Serialization_serialLenUInt() -
      Serialization_serialLenUInt(); // bufLenField & numElemsField

   size_t bufPos = 0;

   // read each list element

   for(i=0; i < elemNum; i++)
   {
      int64_t value;
      unsigned valueLen;

      if(!Serialization_deserializeInt64(&listStart[bufPos], elemsBufLen-bufPos,
         &value, &valueLen) )
         return fhgfs_false;

      bufPos += valueLen;

      Int64CpyVec_append(outVec, value);
   }

   return fhgfs_true;
}

unsigned Serialization_serialLenInt64CpyVec(Int64CpyVec* vec)
{
   /* Int64CpyVecs are derived from Int64CpyLists and serialization format for both is 100%
      compatible, so we can just do a cast here and use the list version code... */

   return Serialization_serialLenInt64CpyList( (Int64CpyList*)vec);
}



unsigned Serialization_serializePath(char* buf, Path* path)
{
   size_t bufPos = 0;

   // StrCpyList
   bufPos += Serialization_serializeStrCpyList(&buf[bufPos], Path_getPathElems(path) );

   // isAbsolute
   bufPos += Serialization_serializeBool(&buf[bufPos], Path_isAbsolute(path) );

   return bufPos;
}

fhgfs_bool Serialization_deserializePathPreprocess(const char* buf, size_t bufLen,
   struct PathDeserializationInfo* outInfo, unsigned* outLen)
{
   size_t bufPos = 0;

   unsigned absFieldLen = 0; // "=0" to mute false compiler warning
   unsigned elemsBufLen = 0; // "=0" to mute false compiler warning

   // StrCpyList
   if(unlikely(!Serialization_deserializeStrCpyListPreprocess(&buf[bufPos], bufLen-bufPos,
      &outInfo->elemNum, &outInfo->elemListStart, &elemsBufLen) ) )
      return fhgfs_false;

   outInfo->elemsBufLen = elemsBufLen;

   bufPos += elemsBufLen;

   // isAbsolute
   if(unlikely(!Serialization_deserializeBool(&buf[bufPos], bufLen-bufPos, &outInfo->isAbsolute,
      &absFieldLen) ) )
      return fhgfs_false;

   bufPos += absFieldLen;

   *outLen = bufPos;

   return fhgfs_true;
}

fhgfs_bool Serialization_deserializePath(struct PathDeserializationInfo* info, Path* outPath)
{
   // StrCpyList
   if(unlikely(!Serialization_deserializeStrCpyList(
      info->elemsBufLen, info->elemNum, info->elemListStart, Path_getPathElems(outPath) ) ) )
      return fhgfs_false;

   // isAbsolute
   Path_setAbsolute(outPath, info->isAbsolute);

   return fhgfs_true;
}

unsigned Serialization_serialLenPath(Path* path)
{
   // elemsList + isAbsolute
   return Serialization_serialLenStrCpyList(Path_getPathElems(path) ) +
      Serialization_serialLenBool();
}

uint16_t Serialization_cpu_to_be16(uint16_t value)
{
   return __cpu_to_be16(value);
}

uint16_t Serialization_be16_to_cpu(uint16_t value)
{
   return __be16_to_cpu(value);
}

uint32_t Serialization_cpu_to_be32(uint32_t value)
{
   return __cpu_to_be32(value);
}

uint32_t Serialization_be32_to_cpu(uint32_t value)
{
   return __be32_to_cpu(value);
}

uint64_t Serialization_cpu_to_be64(uint64_t value)
{
   return __cpu_to_be64(value);
}

uint64_t Serialization_be64_to_cpu(uint64_t value)
{
   return __be64_to_cpu(value);
}

