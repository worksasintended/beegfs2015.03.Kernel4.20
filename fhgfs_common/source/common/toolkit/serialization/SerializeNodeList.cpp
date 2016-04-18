#include "Serialization.h"

/**
 * Serialization of a NodeList.
 *
 * note: 8 byte aligned.
 *
 * @return 0 on error, used buffer size otherwise
 */
unsigned Serialization::serializeNodeList(char* buf, NodeList* nodeList)
{
   unsigned nodeListSize = nodeList->size();

   size_t bufPos = 0;

   // elem count info field

   bufPos += serializeUInt(&buf[bufPos], nodeListSize);

   // padding for 8 byte alignment

   bufPos += serialLenUInt();


   // serialize each element of the nodeList

   NodeListIter iter = nodeList->begin();

   for(unsigned i=0; i < nodeListSize; i++, iter++)
   {
      unsigned nodeStartBufPos = bufPos; // for 8 byte alignment of this node

      NodeConnPool* connPool = (*iter)->getConnPool();
      const BitStore* nodeFeatureFlags = (*iter)->getNodeFeatures();

      // nodeFeatureFlags (8b aligned)
      bufPos += nodeFeatureFlags->serialize(&buf[bufPos] );

      // nodeID
      std::string nodeID = (*iter)->getID();
      bufPos += serializeStrAlign4(&buf[bufPos], nodeID.length(), nodeID.c_str() );

      // nicList (4b aligned)
      NicAddressList nicList(connPool->getNicList() );
      bufPos += serializeNicList(&buf[bufPos], &nicList);

      // fhgfsVersion
      unsigned fhgfsVersion = (*iter)->getFhgfsVersion();
      bufPos += serializeUInt(&buf[bufPos], fhgfsVersion);

      // nodeNumID
      uint16_t nodeNumID = (*iter)->getNumID();
      bufPos += serializeUShort(&buf[bufPos], nodeNumID);

      // portUDP
      uint16_t portUDP = (*iter)->getPortUDP();
      bufPos += serializeUShort(&buf[bufPos], portUDP);

      // portTCP
      uint16_t portTCP = (*iter)->getPortTCP();
      bufPos += serializeUShort(&buf[bufPos], portTCP);

      // nodeType
      char nodeType = (char) (*iter)->getNodeType();
      bufPos += serializeChar(&buf[bufPos], nodeType);

      // add padding for 8 byte alignment
      unsigned remainder = (bufPos - nodeStartBufPos) % 8;
      if(remainder)
         bufPos += (8 - remainder);
   }

   return bufPos;
}

/**
 * Pre-processes a serialized NodeList for deserialization via deserializeNodeList().
 *
 * @return false on error or inconsistency
 */
bool Serialization::deserializeNodeListPreprocess(const char* buf, size_t bufLen,
   unsigned* outNodeListElemNum, const char** outNodeListStart, unsigned* outLen)
{
   size_t bufPos = 0;

   {  // elem count info field
      unsigned elemNumFieldLen;

      if(unlikely(!deserializeUInt(&buf[bufPos], bufLen-bufPos, outNodeListElemNum,
         &elemNumFieldLen) ) )
         return false;

      bufPos += elemNumFieldLen;
   }

   {  // padding for 8 byte alignment
      bufPos += serialLenUInt();
   }

   // store start pos for later deserialize()

   *outNodeListStart = &buf[bufPos];

   for(unsigned i=0; i < *outNodeListElemNum; i++)
   {
      unsigned nodeStartBufPos = bufPos; // for 8 byte alignment of this node

      { // nodeFeatureFlags (8b aligned)
         unsigned featuresBufLen;
         const char* featuresStart;

         if(unlikely(!BitStore::deserializePreprocess(&buf[bufPos], bufLen-bufPos, &featuresStart,
            &featuresBufLen) ) )
            return false;

         bufPos += featuresBufLen;
      }

      { // nodeID
         unsigned nodeIDLen = 0;
         const char* nodeID = NULL;
         unsigned idBufLen = 0;

         if(unlikely(!Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos,
            &nodeIDLen, &nodeID, &idBufLen) ) )
            return false;

         bufPos += idBufLen;
      }

      { // nicList (4b aligned)
         unsigned nicListElemNum = 0;
         const char* nicListStart = NULL;
         unsigned nicListBufLen = 0;

         if(unlikely(!Serialization::deserializeNicListPreprocess(&buf[bufPos], bufLen-bufPos,
            &nicListElemNum, &nicListStart, &nicListBufLen) ) )
            return false;

         bufPos += nicListBufLen;
      }

      { // fhgfsVersion
         unsigned fhgfsVersion = 0;
         unsigned versionBufLen = 0;

         if(unlikely(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &fhgfsVersion,
            &versionBufLen) ) )
            return false;

         bufPos += versionBufLen;
      }

      { // nodeNumID
         uint16_t nodeNumID = 0;
         unsigned nodeNumIDBufLen = 0;

         if(unlikely(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos, &nodeNumID,
            &nodeNumIDBufLen) ) )
            return false;

         bufPos += nodeNumIDBufLen;
      }

      { // portUDP
         uint16_t portUDP = 0;
         unsigned udpBufLen = 0;

         if(unlikely(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos, &portUDP,
            &udpBufLen) ) )
            return false;

         bufPos += udpBufLen;
      }

      { // portTCP
         uint16_t portTCP = 0;
         unsigned tcpBufLen = 0;

         if(unlikely(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos, &portTCP,
            &tcpBufLen) ) )
            return false;

         bufPos += tcpBufLen;
      }

      { // nodeType
         char nodeType = 0;
         unsigned nodeTypeBufLen = 0;

         if(unlikely(!Serialization::deserializeChar(&buf[bufPos], bufLen-bufPos, &nodeType,
            &nodeTypeBufLen) ) )
            return false;

         bufPos += nodeTypeBufLen;
      }

      // add padding for 8 byte alignment
      unsigned remainder = (bufPos - nodeStartBufPos) % 8;
      if(remainder)
         bufPos += (8 - remainder);
   }

   // check buffer overrun
   if(unlikely(bufLen < bufPos) )
      return false;

   *outLen = bufPos;

   return true;
}

/**
 * Deserializes a NodeList.
 * (requires pre-processing)
 *
 * Note: Nodes will be constructed and need to be deleted later by caller.
 */
void Serialization::deserializeNodeList(unsigned nodeListElemNum, const char* nodeListStart,
   NodeList* outNodeList)
{
   size_t bufPos = 0;
   size_t bufLen = ~0; // fake bufLen to max value (has already been verified during pre-processing)
   const char* buf = nodeListStart;


   for(unsigned i=0; i < nodeListElemNum; i++)
   {
      unsigned nodeStartBufPos = bufPos; // for 8 byte alignment of this node

      NicAddressList nicList;
      char nodeType = 0;
      unsigned fhgfsVersion = 0;
      BitStore nodeFeatureFlags;
      uint16_t portTCP = 0;
      uint16_t portUDP = 0;
      uint16_t nodeNumID = 0;
      const char* nodeID = NULL;


      {  // nodeFeatureFlags
         unsigned featuresBufLen;

         nodeFeatureFlags.deserialize(&buf[bufPos], &featuresBufLen);

         bufPos += featuresBufLen;
      }

      {  // nodeID
         unsigned nodeIDLen = 0;
         unsigned idBufLen = 0;

         Serialization::deserializeStrAlign4(&buf[bufPos], bufLen-bufPos, &nodeIDLen, &nodeID,
            &idBufLen);

         bufPos += idBufLen;
      }

      {  // nicList
         unsigned nicListElemNum = 0;
         const char* nicListStart = NULL;
         unsigned nicListBufLen = 0;

         Serialization::deserializeNicListPreprocess(&buf[bufPos], bufLen-bufPos,
            &nicListElemNum, &nicListStart, &nicListBufLen);

         Serialization::deserializeNicList(nicListElemNum, nicListStart, &nicList);

         bufPos += nicListBufLen;
      }

      {  // fhgfsVersion
         unsigned versionBufLen = 0;

         Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &fhgfsVersion, &versionBufLen);

         bufPos += versionBufLen;
      }

      {  // nodeNumID
         unsigned nodeNumIDBufLen = 0;

         Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos, &nodeNumID, &nodeNumIDBufLen);

         bufPos += nodeNumIDBufLen;
      }

      {  // portUDP
         unsigned udpBufLen = 0;

         Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos, &portUDP, &udpBufLen);

         bufPos += udpBufLen;
      }

      {  // portTCP
         unsigned tcpBufLen = 0;

         Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos, &portTCP, &tcpBufLen);

         bufPos += tcpBufLen;
      }

      {  // nodeType
         unsigned nodeTypeBufLen = 0;

         Serialization::deserializeChar(&buf[bufPos], bufLen-bufPos, &nodeType, &nodeTypeBufLen);

         bufPos += nodeTypeBufLen;
      }

      {  // add padding for 8 byte alignment
         unsigned remainder = (bufPos - nodeStartBufPos) % 8;
         if(remainder)
            bufPos += (8 - remainder);
      }


      // construct node
      Node* node = new Node(nodeID, nodeNumID, portUDP, portTCP, nicList);

      node->setNodeType( (NodeType)nodeType);
      node->setFhgfsVersion(fhgfsVersion);
      node->setFeatureFlags(&nodeFeatureFlags);

      // append node to outList
      outNodeList->push_back(node);
   }
}

/**
 * note: 8 byte aligned.
 */
unsigned Serialization::serialLenNodeList(NodeList* nodeList)
{
   unsigned nodeListSize = nodeList->size();

   unsigned bufPos = 0;

   // elem count info field

   bufPos += serialLenUInt();

   // padding for 8 byte alignment

   bufPos += serialLenUInt();


   // serialize each element of the nodeList

   NodeListIter iter = nodeList->begin();

   for(unsigned i=0; i < nodeListSize; i++, iter++)
   {
      unsigned nodeStartBufPos = bufPos; // for 8 byte alignment of this node

      NodeConnPool* connPool = (*iter)->getConnPool();
      const BitStore* nodeFeatureFlags = (*iter)->getNodeFeatures();

      // nodeFeatureFlags (8b aligned)
      bufPos += nodeFeatureFlags->serialLen();

      // nodeID
      std::string nodeID = (*iter)->getID();
      bufPos += serialLenStrAlign4(nodeID.length() );

      // nicList (4b aligned)
      NicAddressList nicList(connPool->getNicList() );
      bufPos += serialLenNicList(&nicList);

      // fhgfsVersion
      bufPos += serialLenUInt();

      // nodeNumID
      bufPos += serialLenUShort();

      // portUDP
      bufPos += serialLenUShort();

      // portTCP
      bufPos += serialLenUShort();

      // nodeType
      bufPos += serialLenChar();

      // add padding for 8 byte alignment
      unsigned remainder = (bufPos - nodeStartBufPos) % 8;
      if(remainder)
         bufPos += (8 - remainder);
   }

   return bufPos;
}


/**
 * Pre-processes a serialized NodeList of the 2012.10 format for deserialization via
 * deserializeNodeList2012().
 *
 * @return false on error or inconsistency
 */
bool Serialization::deserializeNodeListPreprocess2012(const char* buf, size_t bufLen,
   unsigned* outNodeListElemNum, const char** outNodeListStart, unsigned* outLen)
{
   // Note: This is a fairly inefficient implementation (requiring redundant pre-processing),
   //    but efficiency doesn't matter for the typical use-cases of this method

   size_t bufPos = 0;

   {  // elem count info field
      unsigned elemNumFieldLen;

      if(unlikely(!deserializeUInt(&buf[bufPos], bufLen-bufPos, outNodeListElemNum,
         &elemNumFieldLen) ) )
         return false;

      bufPos += elemNumFieldLen;
   }


   *outNodeListStart = &buf[bufPos];

   for(unsigned i=0; i < *outNodeListElemNum; i++)
   {
      {  // nodeID
         unsigned nodeIDLen = 0;
         const char* nodeID = NULL;
         unsigned idBufLen = 0;

         if(unlikely(!Serialization::deserializeStr(&buf[bufPos], bufLen-bufPos,
            &nodeIDLen, &nodeID, &idBufLen) ) )
            return false;

         bufPos += idBufLen;
      }

      {  // nodeNumID
         uint16_t nodeNumID = 0;
         unsigned nodeNumIDBufLen = 0;

         if(unlikely(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos, &nodeNumID,
            &nodeNumIDBufLen) ) )
            return false;

         bufPos += nodeNumIDBufLen;
      }

      {  // portUDP
         uint16_t portUDP = 0;
         unsigned udpBufLen = 0;

         if(unlikely(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos, &portUDP,
            &udpBufLen) ) )
            return false;

         bufPos += udpBufLen;
      }

      {  // portTCP
         uint16_t portTCP = 0;
         unsigned tcpBufLen = 0;

         if(unlikely(!Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos, &portTCP,
            &tcpBufLen) ) )
            return false;

         bufPos += tcpBufLen;
      }

      {  // nicList
         unsigned nicListElemNum = 0;
         const char* nicListStart = NULL;
         unsigned nicListBufLen = 0;

         if(unlikely(!Serialization::deserializeNicListPreprocess2012(&buf[bufPos], bufLen-bufPos,
            &nicListElemNum, &nicListStart, &nicListBufLen) ) )
            return false;

         bufPos += nicListBufLen;
      }

      {  // nodeType
         char nodeType = 0;
         unsigned nodeTypeBufLen = 0;

         if(unlikely(!Serialization::deserializeChar(&buf[bufPos], bufLen-bufPos, &nodeType,
            &nodeTypeBufLen) ) )
            return false;

         bufPos += nodeTypeBufLen;
      }

      {  // fhgfsVersion
         unsigned fhgfsVersion = 0;
         unsigned versionBufLen = 0;

         if(unlikely(!Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &fhgfsVersion,
            &versionBufLen) ) )
            return false;

         bufPos += versionBufLen;
      }
   }

   *outLen = bufPos;

   return true;
}

/**
 * Deserializes a NodeList of the 2012 format
 * (requires pre-processing)
 *
 * Note: Nodes will be constructed and need to be deleted later
 */
void Serialization::deserializeNodeList2012(unsigned nodeListElemNum, const char* nodeListStart,
   NodeList* outNodeList)
{
   // Note: This is a fairly inefficient implementation (containing redundant pre-processing),
   //    but efficiency doesn't matter for the typical use-cases of this method

   size_t bufPos = 0;
   size_t bufLen = ~0; // fake bufLen to max value (has already been verified during pre-processing)
   const char* buf = nodeListStart;

   for(unsigned i=0; i < nodeListElemNum; i++)
   {
      const char* nodeID = NULL;
      uint16_t nodeNumID = 0;
      uint16_t portUDP = 0;
      uint16_t portTCP = 0;
      char nodeType = 0;
      unsigned fhgfsVersion = 0;
      NicAddressList nicList;

      {  // nodeID
         unsigned nodeIDLen = 0;
         unsigned idBufLen = 0;

         Serialization::deserializeStr(&buf[bufPos], bufLen-bufPos, &nodeIDLen, &nodeID, &idBufLen);

         bufPos += idBufLen;
      }

      {  // nodeNumID
         unsigned nodeNumIDBufLen = 0;

         Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos, &nodeNumID, &nodeNumIDBufLen);

         bufPos += nodeNumIDBufLen;
      }

      {  // portUDP
         unsigned udpBufLen = 0;

         Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos, &portUDP, &udpBufLen);

         bufPos += udpBufLen;
      }

      {  // portTCP
         unsigned tcpBufLen = 0;

         Serialization::deserializeUShort(&buf[bufPos], bufLen-bufPos, &portTCP, &tcpBufLen);

         bufPos += tcpBufLen;
      }

      {  // nicList
         unsigned nicListElemNum = 0;
         const char* nicListStart = NULL;
         unsigned nicListBufLen = 0;

         Serialization::deserializeNicListPreprocess2012(&buf[bufPos], bufLen-bufPos,
            &nicListElemNum, &nicListStart, &nicListBufLen);

         bufPos += nicListBufLen;

         Serialization::deserializeNicList2012(nicListElemNum, nicListStart, &nicList);
      }

      {  // nodeType
         unsigned nodeTypeBufLen = 0;

         Serialization::deserializeChar(&buf[bufPos], bufLen-bufPos, &nodeType, &nodeTypeBufLen);

         bufPos += nodeTypeBufLen;
      }

      {  // fhgfsVersion
         unsigned versionBufLen = 0;

         Serialization::deserializeUInt(&buf[bufPos], bufLen-bufPos, &fhgfsVersion, &versionBufLen);

         bufPos += versionBufLen;
      }


      // construct node
      Node* node = new Node(nodeID, nodeNumID, portUDP, portTCP, nicList);

      node->setNodeType( (NodeType)nodeType);
      node->setFhgfsVersion(fhgfsVersion);

      // append node to outList
      outNodeList->push_back(node);
   }
}


